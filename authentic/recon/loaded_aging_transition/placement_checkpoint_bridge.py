#!/usr/bin/env python3
"""Fixed operator trust bridge. No launch, native calls, settings or backup writes.
Only creates a NEW placement_checkpoint_PID directory after external prelaunch.
Restoration remains /tmp/opensc4_checkpoint_preferences.py restore NONCE after death.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess

ROOT = Path('/Users/russellmartin/Documents/OpenSC4Fixtures/aging')
PREF = Path('/Users/russellmartin/Documents/SimCity 4/Sim City 4 Deluxe Edition.cfg')
LOG = Path('/Users/russellmartin/.opensc4-oracle/oracle.log')


def number(s):
    if not re.fullmatch(r'[1-9][0-9]{0,9}', s) or int(s) > 2147483647:
        raise ValueError('positive canonical int32 required')
    return int(s)


def wire(pid, nonce, original, receipt):
    number(str(pid)); number(str(nonce))
    if not all(re.fullmatch('[0-9a-f]{64}', h) for h in (original, receipt)):
        raise ValueError('invalid digest')
    return f'OPENSC4_PLACEMENT_BRIDGE_V1\n{pid}\n{nonce}\n{original}\n{receipt}\n'.encode('ascii')


def canonical(p):
    if not p.is_absolute() or p.resolve(strict=True) != p:
        raise ValueError('noncanonical path')
    for c in (p, *p.parents):
        if c.is_symlink():
            raise ValueError('symlink component')
    return p


def stamp(s):
    return (s.st_dev, s.st_ino, s.st_size, s.st_mode, s.st_uid, s.st_gid,
            s.st_nlink, s.st_mtime_ns, s.st_ctime_ns)


def snapshot(p, limit=128*1024*1024):
    canonical(p)
    a = p.lstat()
    if not stat.S_ISREG(a.st_mode) or a.st_nlink != 1 or a.st_uid != os.geteuid() or not 0 <= a.st_size <= limit:
        raise ValueError('unsupported file identity/ownership/size')
    fd = os.open(p, os.O_RDONLY | os.O_NOFOLLOW)
    with os.fdopen(fd, 'rb') as f:
        if stamp(os.fstat(f.fileno())) != stamp(a):
            raise ValueError('open race')
        data = f.read(limit + 1)
        if len(data) != a.st_size or stamp(os.fstat(f.fileno())) != stamp(a):
            raise ValueError('read race/truncation')
    if stamp(p.lstat()) != stamp(a):
        raise ValueError('path changed')
    return data, a, hashlib.sha256(data).hexdigest()


def attrs(p):
    result = {}
    for name in subprocess.check_output(['/usr/bin/xattr', str(p)], text=True).splitlines():
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_.-]*', name):
            raise ValueError('unsupported xattr')
        value = subprocess.check_output(['/usr/bin/xattr', '-p', '-x', name, str(p)], text=True)
        result[name] = base64.b64encode(bytes.fromhex(value)).decode()
    return result


def logical(p, s, r):
    if getattr(s, 'st_flags', 0) or s.st_gid not in {os.getegid(), *os.getgroups()}:
        raise ValueError('unsupported flags/group')
    if len(subprocess.check_output(['/bin/ls', '-lde', str(p)], text=True).splitlines()) != 1:
        raise ValueError('unsupported ACL')
    if r is not None and (stat.S_IMODE(s.st_mode), s.st_uid, s.st_gid, s.st_mtime_ns, attrs(p)) != (
            r['mode'], r['uid'], r['gid'], r['mtime_ns'], r['xattrs']):
        raise ValueError('original logical metadata mismatch')


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON field')
        result[key] = value
    return result


def create(pid, nonce):
    canonical(ROOT)
    os.kill(pid, 0)
    log, _, _ = snapshot(LOG, 16*1024*1024)
    ups = re.findall(rb'ORACLE-UP pid=([0-9]+) ', log)
    if not ups or int(ups[-1]) != pid:
        raise ValueError('latest ORACLE-UP PID mismatch')
    if b'PLACEMENT_CHECKPOINT BEGIN' in log[log.rfind(b'ORACLE-UP pid='):]:
        raise ValueError('checkpoint already began')
    folder = ROOT / f'tutorial_checkpoint_{nonce}'
    canonical(folder)
    receipt, receipt_st, receipt_sha = snapshot(folder / 'preferences_receipt.json', 65536)
    r = json.loads(receipt, object_pairs_hook=unique_object)
    # Existing prelaunch protocol names the nonce "pid"; accept optional nonce
    # only when it agrees, never confuse it with the actual process PID.
    if type(r.get('pid')) is not int or r['pid'] != nonce or ('nonce' in r and r['nonce'] != nonce):
        raise ValueError('receipt nonce mismatch')
    if r.get('path') != str(PREF) or r.get('present') is not True or r.get('restorationCapabilityTested') is not True:
        raise ValueError('unsupported absent/unprotected preferences')
    if not r.get('coveredBaseline', '').startswith('before original reload launch'):
        raise ValueError('requires prelaunch receipt')
    original, original_st, original_sha = snapshot(folder / 'preferences.original')
    if original_sha != r.get('sha256'):
        raise ValueError('original checksum mismatch')
    logical(folder / 'preferences.original', original_st, r)
    _, runtime_st, _ = snapshot(PREF)
    logical(PREF, runtime_st, None)
    if (original_st.st_dev, original_st.st_ino) == (runtime_st.st_dev, runtime_st.st_ino):
        raise ValueError('backup aliases runtime')
    # Startup runtime bytes may differ; NEVER replace the restoration baseline.
    payload = wire(pid, nonce, original_sha, receipt_sha)
    for p, old in ((folder / 'preferences.original', original_st), (folder / 'preferences_receipt.json', receipt_st), (PREF, runtime_st)):
        if stamp(p.lstat()) != stamp(old):
            raise ValueError('baseline changed')
    os.kill(pid, 0)
    dest = ROOT / f'placement_checkpoint_{pid}'
    dest.mkdir(mode=0o700)  # exclusive: no existing PID directory or bridge
    with (dest / 'protection.bridge').open('xb') as f:
        os.chmod(f.fileno(), 0o600)
        f.write(payload)
        f.flush(); os.fsync(f.fileno())
    print(dest / 'protection.bridge')


def self_test():
    import tempfile
    for bad in ('0', '-1', '+1', '01', '2147483648', '1\n', '1\x00', ''):
        try:
            number(bad)
        except ValueError:
            pass
        else:
            raise AssertionError(bad)
    assert wire(1, 2, 'a'*64, 'b'*64).endswith(b'\n')
    with tempfile.TemporaryDirectory() as t:
        p = Path(t).resolve() / 'file'
        p.write_bytes(b'original\x00'); a = snapshot(p)
        p.write_bytes(b'runtime different'); b = snapshot(p)
        assert a[2] != b[2]
        alias = p.parent / 'alias'; alias.symlink_to(p)
        try:
            snapshot(alias)
        except ValueError:
            pass
        else:
            raise AssertionError('symlink')
    print('PASS bridge pure self-test; no real preferences/bridge touched')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--pid', type=number)
    parser.add_argument('--nonce', type=number)
    args = parser.parse_args()
    if args.self_test:
        if args.pid or args.nonce:
            parser.error('self-test takes no PID/nonce')
        self_test()
    elif args.pid and args.nonce:
        create(args.pid, args.nonce)
    else:
        parser.error('--pid and --nonce required')
