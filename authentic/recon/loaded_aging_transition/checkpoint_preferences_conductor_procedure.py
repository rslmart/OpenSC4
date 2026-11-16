"""Conductor-only commands for this fixed diagnostic; no game calls."""
from pathlib import Path
import os,sys,json,re,hashlib,stat,shutil,subprocess,tempfile,base64
root=Path('/Users/russellmartin/Documents/OpenSC4Fixtures/aging')
region=Path('/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114')
pid=int(sys.argv[2]); folder=root/f'tutorial_checkpoint_{pid}'
def sha(path):
    with open(path,'rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def canonical(p,absent=False):
    p=Path(p); assert p.is_absolute() and str(p)==str(p.resolve(strict=not absent))
    for part in [p,*p.parents]: assert not part.is_symlink()
    return p
def attrs(p):
    names=subprocess.check_output(['/usr/bin/xattr',str(p)],text=True).splitlines()
    result={}
    for n in names:
        assert re.fullmatch(r'[A-Za-z][A-Za-z0-9_.-]*',n), 'unsupported xattr name'
        raw=subprocess.check_output(['/usr/bin/xattr','-p','-x',n,str(p)],text=True)
        result[n]=base64.b64encode(bytes.fromhex(raw)).decode()
    return result
def set_attr(p,n,data):
    subprocess.run(['/usr/bin/xattr','-w','-x',n,data.hex(),str(p)],check=True)
def del_attr(p,n):subprocess.run(['/usr/bin/xattr','-d',n,str(p)],check=True)
def acl_none(p):
    out=subprocess.check_output(['/bin/ls','-lde',str(p)],text=True)
    assert len(out.splitlines())==1, 'unsupported preferences ACL; stop before save'
def eligible(p):
    canonical(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and s.st_nlink==1 and s.st_size<=128*1024*1024
    assert s.st_uid==os.geteuid() and s.st_gid in {os.getegid(),*os.getgroups()}
    assert s.st_flags==0,'unsupported flags; stop before save'
    acl_none(p);return s
def logical(p,r):
    s=p.stat();return (stat.S_IMODE(s.st_mode),s.st_uid,s.st_gid,s.st_mtime_ns,attrs(p))==(r['mode'],r['uid'],r['gid'],r['mtime_ns'],r['xattrs'])
def apply_metadata(p,r):
    os.chown(p,r['uid'],r['gid']);os.chmod(p,r['mode'])
    for n in attrs(p):
        if n not in r['xattrs']:del_attr(p,n)
    for n,v in r['xattrs'].items():
        if attrs(p).get(n)!=v:set_attr(p,n,base64.b64decode(v))
    os.utime(p,ns=(r['atime_ns'],r['mtime_ns']))
if sys.argv[1]=='backup':
    os.kill(pid,0)
    raw=(Path.home()/'.opensc4-oracle/oracle.log').read_text()
    assert f'ORACLE-UP pid={pid} ' in raw and raw.count('TUTORIAL_CHECKPOINT READY ')==1
    assert 'TUTORIAL_CHECKPOINT INSPECT_END complete=1' in raw
    assert 'TUTORIAL_OPEN BEGIN' not in raw and 'TUTORIAL_FINISH BEGIN' not in raw
    m=re.search(r'TUTORIAL_CHECKPOINT READY pid=(\d+) .*? path=(.*?) prefpath=(.*?) presence=([01]) sha256=([0-9a-f]*) size=(-?\d+) mode=([0-7]+) uid=(\d+) gid=(\d+) mtime=(\d+)\.(\d{9})',raw);assert m
    assert int(m[1])==pid and Path(m[2])==region/'City - Rush Hour Tutorial.sc4'
    p=canonical(m[3],True)
    assert p.is_relative_to(Path.home()) and not p.is_relative_to(region) and not p.is_relative_to(root)
    assert not p.is_relative_to(Path.home()/'Library/Application Support/Steam/steamapps/common/SimCity 4 Deluxe')
    present=m[4]=='1';canonical(root);folder.mkdir(mode=0o700)
    r={'pid':pid,'path':str(p),'present':present,'coveredBaseline':'READY before checkpoint SaveCity; startup before READY excluded','identityRestoration':'inode/ctime/birthtime not claimed','originalReady':m[0]}
    if present:
        s=eligible(p);assert sha(p)==m[5] and s.st_size==int(m[6]) and s.st_mode==int(m[7],8) and s.st_uid==int(m[8]) and s.st_gid==int(m[9])
        assert s.st_mtime_ns==int(m[10])*10**9+int(m[11])
        n=re.search(r'TUTORIAL_CHECKPOINT PREF_METADATA dev=(\d+) ino=(\d+) nlink=(\d+) atime=(\d+)\.(\d{9}) ctime=(\d+)\.(\d{9})',raw);assert n
        assert (s.st_dev,s.st_ino,s.st_nlink,s.st_ctime_ns)==(int(n[1]),int(n[2]),int(n[3]),int(n[6])*10**9+int(n[7]))
        r.update(sha256=m[5],mode=stat.S_IMODE(s.st_mode),uid=s.st_uid,gid=s.st_gid,mtime_ns=s.st_mtime_ns,atime_ns=int(n[4])*10**9+int(n[5]),xattrs=attrs(p),dev=s.st_dev,ino=s.st_ino,size=s.st_size)
        backup=folder/'preferences.original';shutil.copy2(p,backup);apply_metadata(backup,r)
        assert eligible(backup).st_ino!=s.st_ino and sha(backup)==r['sha256'] and logical(backup,r)
        fd,tmp=tempfile.mkstemp(prefix='.opensc4-preference-restore-test-',dir=p.parent);os.close(fd);test=Path(tmp)
        try:
            shutil.copyfile(backup,test);apply_metadata(test,r)
            assert sha(test)==r['sha256'] and logical(test,r)
            os.utime(test,ns=(r['atime_ns'],r['mtime_ns']));assert test.stat().st_atime_ns==r['atime_ns'];acl_none(test)
        finally:test.unlink(missing_ok=True)
        assert sha(p)==r['sha256'] and logical(p,r) and p.stat().st_ino==r['ino'] and p.stat().st_ctime_ns==s.st_ctime_ns
    else:
        assert not p.exists() and not p.is_symlink() and p.parent.is_dir()
        marker=b'OpenSC4 tutorial checkpoint: preferences originally absent\n'
        assert hashlib.sha256(marker).hexdigest()=='4e29f417049ee870db30826c5ec73027fe6fcff44f94d60665fbca3f767f5cf7'
        (folder/'preferences.absent').write_bytes(marker)
        fd,tmp=tempfile.mkstemp(prefix='.opensc4-preference-restore-test-',dir=p.parent);os.close(fd);os.unlink(tmp)
    (folder/'inspection.log').write_text(raw)
    r['restorationCapabilityTested']=True
    (folder/'preferences_receipt.json').write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({'backup':str(folder),'preferences':str(p),'present':present,'sha256':r.get('sha256'),'restorationCapabilityTested':True}))
elif sys.argv[1]=='restore':
    try:os.kill(pid,0)
    except ProcessLookupError:pass
    else:raise RuntimeError('game still alive; restoration forbidden')
    r=json.loads((folder/'preferences_receipt.json').read_text());assert r['pid']==pid and r['restorationCapabilityTested']
    p=canonical(r['path'],True)
    if p.exists():
        s=eligible(p);current_hash=sha(p)
        after=folder/'preferences.after';assert not after.exists();shutil.copy2(p,after)
        (folder/'preferences_after_metadata.json').write_text(json.dumps({'sha256':current_hash,'stat':list(s),'xattrs':attrs(p)},indent=2)+'\n')
        assert p.stat().st_ino==s.st_ino and sha(p)==current_hash
    else:current_hash=None
    if r['present']:
        original=folder/'preferences.original';assert sha(original)==r['sha256'] and logical(original,r)
        fd,tmp=tempfile.mkstemp(prefix='.opensc4-preference-restore-',dir=p.parent);os.close(fd);target=Path(tmp)
        try:
            shutil.copyfile(original,target);apply_metadata(target,r)
            assert sha(target)==r['sha256'] and logical(target,r);acl_none(target)
            if current_hash is None:assert not p.exists()
            else:assert sha(p)==current_hash and p.stat().st_ino==s.st_ino
            os.replace(target,p)
        finally:target.unlink(missing_ok=True)
        assert sha(p)==r['sha256'] and logical(p,r);acl_none(p)
        os.utime(p,ns=(r['atime_ns'],r['mtime_ns']));assert p.stat().st_atime_ns==r['atime_ns']
    else:
        if current_hash is not None:assert sha(p)==current_hash;p.unlink()
        assert not p.exists() and not p.is_symlink()
    result={'restored':True,'present':r['present'],'originalSHA256':r.get('sha256'),'postGameSHA256':current_hash,'logicalMetadataVerified':r['present'],'excludedMetadata':'inode/ctime/birthtime; startup before READY'}
    (folder/'preferences_restoration_result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
elif sys.argv[1]=='selftest':
    with tempfile.TemporaryDirectory(prefix='preference_procedure_test_',dir=root) as t:
        a=Path(t)/'original';b=Path(t)/'restored';a.write_bytes(b'preserve-test\x00\xff');os.chmod(a,0o600)
        set_attr(a,'com.opensc4.test',b'\x00\xffbinary\n');set_attr(a,'com.opensc4.empty',b'')
        observed=attrs(a);assert observed['com.opensc4.empty']=='' and observed['com.opensc4.test']==base64.b64encode(b'\x00\xffbinary\n').decode()
        st=a.stat();r={'mode':stat.S_IMODE(st.st_mode),'uid':st.st_uid,'gid':st.st_gid,'atime_ns':st.st_atime_ns,'mtime_ns':st.st_mtime_ns,'xattrs':attrs(a)}
        shutil.copyfile(a,b);apply_metadata(b,r);assert sha(a)==sha(b) and logical(b,r);eligible(b)
        del_attr(b,'com.opensc4.test');assert 'com.opensc4.test' not in attrs(b)
        apply_metadata(b,r);assert logical(b,r)
        print('PASS macOS xattr binary/empty/list/delete; copy/restore logical metadata; ACL/flags domain')
else:raise ValueError('only backup, restore or selftest')
