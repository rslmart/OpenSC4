"""Focused offline validation only. No oracle build, game, global verifier or prefs."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent
flags=['xcrun','clang++','-std=c++17','-arch','x86_64','-isysroot','/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk','-ffp-model=strict','-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-undefined-internal']
def run(name, command):
    p=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    (r/f'placement_checkpoint_{name}.log').write_text(p.stdout)
    print(name, p.returncode, p.stdout.strip())
    if p.returncode:
        raise SystemExit(p.returncode)
run('syntax',flags+['-fsyntax-only',str(r/'placement_checkpoint_syntax_stub.cpp')])
for name,source in [('checks','placement_checkpoint_checks.cpp'),('receipt_regression','placement_receipt_checks.cpp'),('late_regression','placement_late_checks.cpp')]:
    binary=r/f'placement_checkpoint_{name}.bin'
    run(name+'_build',flags+['-O2','-Wl,-dead_strip',str(r/source),'-o',str(binary)])
    run(name,[str(binary)])
run('bridge_selftest',['python3',str(r/'placement_checkpoint_bridge.py'),'--self-test'])
run('source_checks',['python3',str(r/'placement_checkpoint_source_checks.py')])
run('guardrail',['python3','/Users/russellmartin/DecompBench/tools/static_guardrail.py','--mode','opensc4_live','--files',str(r/'placement_checkpoint_fragment.inc'),str(r/'fixture_placement_bindings.inc')])
run('diffcheck',['git','diff','--check','--',str(r/'fixture_placement_bindings.inc')])
