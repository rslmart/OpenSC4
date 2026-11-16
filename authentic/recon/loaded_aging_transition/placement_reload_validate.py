from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent
flags=['xcrun','clang++','-std=c++17','-arch','x86_64','-isysroot','/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk','-ffp-model=strict','-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-undefined-internal']
def run(n,c):
 p=subprocess.run(c,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT); (r/f'placement_reload_{n}.log').write_text(p.stdout); print(n,p.returncode,p.stdout.strip());
 if p.returncode: raise SystemExit(p.returncode)
run('syntax',flags+['-fsyntax-only',str(r/'placement_reload_syntax_stub.cpp')])
run('generate',['python3',str(r/'placement_reload_checks.py')])
for n,s in [('checks','placement_reload_checks.cpp'),('receipt','placement_receipt_checks.cpp'),('lifecycle','placement_late_checks.cpp')]:
 binary=r/f'placement_reload_{n}.bin'; run(n+'_build',flags+['-O2','-Wl,-dead_strip',str(r/s),'-o',str(binary)]); run(n,[str(binary)])
run('source_checks',['python3',str(r/'placement_reload_source_checks.py')])
run('guardrail',['python3','/Users/russellmartin/DecompBench/tools/static_guardrail.py','--mode','opensc4_live','--files',str(r/'placement_reload_fragment.inc'),str(r/'fixture_placement_bindings.inc')])
run('diffcheck',['git','diff','--check','--',str(r/'fixture_placement_bindings.inc')])
