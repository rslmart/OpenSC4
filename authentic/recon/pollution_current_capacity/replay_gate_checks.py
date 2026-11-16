"""Regression for incomplete captures that previously returned success."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from prepare_native_replay import prepare

capture = Path(__file__).with_name('live_capacity_output.txt').read_text()
valid = prepare(capture)
checks = 1


def reject(name, data):
    global checks
    try:
        prepare(data)
    except (ValueError, KeyError, IndexError):
        checks += 1
        return
    raise RuntimeError(f'false pass: {name}')


for tag in ('CURVE-RECORD', 'CAP-SKIP', 'CAP-TRACE', 'CAP-TOTAL', 'CURVE-TOTAL',
            'CURVE-EMPTY', 'CURVE-EMPTY-EVAL', 'CAP-RAW'):
    reject('missing '+tag, '\n'.join(x for x in capture.splitlines() if f' {tag} ' not in x))
reject('review demonstrated combined deletion', '\n'.join(
    x for x in capture.splitlines() if not any(f' {t} ' in x for t in ('CURVE-RECORD', 'CAP-SKIP', 'CAP-TRACE'))))
reject('wrong total', capture.replace('compared=2285', 'compared=2284'))
reject('mismatched trace', capture.replace('kernel=HCcUPHR', 'kernel=HCcUR', 1))
reject('both traces incorrect', capture.replace('HCcUPHR', 'HCcUR'))
reject('misassociated record', capture.replace('CURVE-RECORD id=0 index=0', 'CURVE-RECORD id=1 index=0', 1))
reject('duplicate record replacing missing index', capture.replace('CURVE-RECORD id=0 index=1', 'CURVE-RECORD id=0 index=0', 1))
reject('duplicate capacity ID', capture.replace('CAP id=1 ', 'CAP id=0 ', 1))
reject('misassociated raw payload', capture.replace('CAP-RAW id=177 ', 'CAP-RAW id=176 ', 1))
reject('wrong exclusion ID', capture.replace('CAP-SKIP id=1632 ', 'CAP-SKIP id=0 ', 1))
reject('missing evaluation with duplicate replacement', capture.replace('CURVE-EVAL id=0 input=80000000', 'CURVE-EVAL id=0 input=00000000', 1))
reject('invalid scalar count', capture.replace('CAP-RAW id=176 count=4', 'CAP-RAW id=176 count=9', 1))
if len(sys.argv) > 1:
    with tempfile.TemporaryDirectory(prefix='opensc4-replay-negative-') as tmp:
        path = Path(tmp)/'replay.txt'
        variants = {
            'valid': valid,
            'no-records': '\n'.join(x for x in valid.splitlines() if not x.startswith('R ')),
            'record-wrong-id': valid.replace('R 0 0 ', 'R 1 0 ', 1),
            'record-duplicate': valid.replace('R 0 1 ', 'R 0 0 ', 1),
            'evaluation-wrong-id': valid.replace('E 0 ', 'E 1 ', 1),
            'duplicate-capacity': re.sub(r'^C 1 ', 'C 0 ', valid, count=1, flags=re.M),
        }
        for name, data in variants.items():
            path.write_text(data)
            result = subprocess.run([sys.argv[1], str(path)], capture_output=True, text=True, timeout=10)
            if (result.returncode == 0) != (name == 'valid'):
                raise RuntimeError(f'bad replay exit for {name}: {result.returncode}\n{result.stdout}')
            checks += 1
print(f'{checks} evidence-gate checks passed (valid capture plus deliberate corruptions)')
