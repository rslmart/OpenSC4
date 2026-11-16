"""Read-only scope/delta checks; never generate or overwrite implementation."""
from pathlib import Path
import re
import subprocess
p = 'authentic/recon/loaded_aging_transition/fixture_placement_bindings.inc'
old = subprocess.check_output(['git', 'show', 'c8a6faf:' + p], text=True)
new = Path(p).read_text()
def between(s, a, b):
    return s[s.index(a):s.index(b, s.index(a))]
assert between(old, 'static void late_exec(', 'static bool late_transfer(') == between(new, 'static void late_exec(', 'static bool late_transfer(')
assert between(old, 'struct Job {', '} // namespace') == between(new, 'struct Job {', '} // namespace')
# Original calls, log format strings, slot guards and object domains unchanged.
for pattern in [r'original<[^\n]*?>\(0x[0-9a-f]+\)', r'logf_\("([^"\n]*)"',
                r'slot\([^\n]*?0x[0-9a-f]+\)', r'object\([^\n]*?0x[0-9a-f]+\)']:
    before, after = re.findall(pattern, old), re.findall(pattern, new)
    assert before and before == after, pattern
assert 'c.deps={identity,services,inventory}' in new
assert 'static bool inventory(void* opaque,const State& s,const char* phase)' in new
assert 'static bool services(void* opaque,const State& s,uint64_t control)' in new
print('placement receipt source checks: PASS; native call/log/guard sequences and ordinary Job paths unchanged')
