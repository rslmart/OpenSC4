from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent
flags=['xcrun','clang++','-std=c++17','-arch','x86_64','-isysroot','/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk','-ffp-model=strict','-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-undefined-internal']
def run(n,c):
 p=subprocess.run(c,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 (r/f'aging_input_capture_{n}.log').write_text(p.stdout)
 print(n,p.returncode,p.stdout.strip())
 if p.returncode: raise SystemExit(p.returncode)
def guard():
 run('guardrail',['python3','/Users/russellmartin/DecompBench/tools/static_guardrail.py','--mode','opensc4_live','--files',str(r/'aging_input_capture_fragment.inc'),str(r/'placement_reload_fragment.inc')])
guard()
run('syntax',flags+['-fsyntax-only',str(r/'aging_input_capture_syntax_stub.cpp')])
guard()
run('checks_build',flags+['-O2','-Wl,-dead_strip',str(r/'aging_input_capture_checks.cpp'),'-o',str(r/'aging_input_capture_checks.bin')])
run('checks',[str(r/'aging_input_capture_checks.bin')])
run('diffcheck',['git','diff','--check','--',str(r/'placement_reload_fragment.inc')])

run('mock_generate',['python3',str(r/'aging_input_capture_mock_checks.py')])
for name,source in [('mock','aging_input_capture_mock_checks.cpp'),('old','placement_reload_checks.cpp')]:
 guard()
 run(name+'_build',flags+['-O2','-Wl,-dead_strip',str(r/source),'-o',str(r/f'aging_input_capture_{name}.bin')])
 run(name,[str(r/f'aging_input_capture_{name}.bin')])
