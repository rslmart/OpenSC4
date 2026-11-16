"""Narrow wiring checks complement executed actual-helper lifecycle tests; no game proof."""
from pathlib import Path
import subprocess
p=Path(__file__).with_name('fixture_placement_bindings.inc')
s=p.read_text()
base=subprocess.check_output(['git','show','228874c:authentic/recon/loaded_aging_transition/fixture_placement_bindings.inc'],text=True)
# Byte-preserve existing identity, original service/descriptor and grid guards.
for start,end in [('static bool loaded_path(', 'static bool property('), ('static bool grid(', 'static bool occupant(')]:
    old=base[base.index(start):base.index(end)]
    # Only inserted helpers/comments occupy the tail, not the original blocks.
    assert old.rstrip() in s
command=s[s.index('static void command_exec('):s.index('// Returns queue acceptance only.')]
assert command.count('fixture_placement::exec(&c);')==1
assert command.index('fixture_placement::exec(&c);')<command.index('late_transfer(')
assert 'late_scope(j->attempt,j->context.click_calls,ownedKind)' in command
late=s[s.index('static void late_exec('):s.index('static void command_exec(')]
assert late.count('dispatch_after_f(')==1
assert late.count('services(receipt.get(),j->late_second,0)')==1
assert late.count('inventory(receipt.get(),j->late_second,"LATER_AFTER_CLICK")')==1
late_code='\n'.join(line.split('//')[0] for line in late.splitlines())
assert 'active(' not in late_code and 'new ' not in late_code and 'Release' not in late_code
assert late.index('late_context=j.get()')<late.index('j.release()')<late.index('dispatch_after_f(')
prop=s[s.index('static bool property(uint64_t'):s.index('static bool grid(')]
assert prop.index('log_property_domain(')<prop.index('else if(type!=6')
assert 'count>256' in prop and 'n*4' in prop
assert '0x029244db' not in s
print('placement late wiring checks: passed; original identity/services/grid preserved')
