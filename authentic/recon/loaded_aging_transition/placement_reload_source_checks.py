from pathlib import Path
import hashlib,re,subprocess
r=Path(__file__).resolve().parent
b=(r/'fixture_placement_bindings.inc').read_text(); m=(r/'placement_reload_fragment.inc').read_text()
old=subprocess.check_output(['git','show','9172457:authentic/recon/loaded_aging_transition/fixture_placement_bindings.inc'],text=True)
def body(s,n):
 start=s.index('static ',s.rfind('\n',0,s.index(n+'('))+1); i=s.index('{',start)+1; depth=1
 while depth: depth+=(s[i]=='{')-(s[i]=='}'); i+=1
 return s[start:i]
# Unmodified runtime, native service/query/collector and mutator functions byte-identical.
changed={'observe_owner','fixture_profile','loaded_path','identity','aging_fields'}
names=re.findall(r'^static (?:[\w:<>]+\s+)+([a-zA-Z_]\w*)\(',old,re.M)
for n in names:
 if n not in changed: assert body(b,n)==body(old,n),n
# Native call sequence stays identical in the altered identity path.
for n in changed:
 assert re.findall(r'original<.*',body(b,n))==re.findall(r'original<.*',body(old,n)),n
assert 'bool diagnostic=false' in body(b,'fixture_profile')
assert 'bool diagnostic=false' in body(b,'loaded_path')
assert 'diagnostic=o.reload_allowed()' in body(b,'identity')
assert 'o.kind!=kind' in body(b,'identity')
assert 'private:\n    bool diagnostic_reload=false;' in b
assert 'kind!=FixtureKind::SavedInactive' in body(b,'placement_candidate_matches')
assert 'loaded_path(s,path,regional,&kind);' in b # ordinary exec stays closed
pc=(r/'placement_checkpoint_fragment.inc').read_text()
assert 'receipt->kind==fb::FixtureKind::SavedInactive' in body(pc,'command_preflight')
assert m.count('dispatch_after_f(')==1 and m.count('dispatch_async_f(')==1
assert 'if(!readable(context->initial)) { end(false,"initial_incomplete"); return; }' in m
assert m.count('PLACEMENT_RELOAD END ')==1
for prohibited in ['receipt.reset','pc::ready','SaveCity','PickTerrain','dispatch_sync','busy.store','delete context','tc::prepared_file','pc::fresh','pc::validate','tc::neighbors']:
 assert prohibited not in re.sub(r'//[^\n]*','',m),prohibited
for file,expected in [('PLACEMENT_RELOAD_FIXTURE_IDENTITY.json','7b608df3746f52afb94563a1b3e66aa3139e276319b761ae473a3ec84c36bbce')]:
 assert hashlib.sha256((r/file).read_bytes()).hexdigest()==expected
assert '&s.profile,c.owner.reload_allowed()' in m
for f in [r/'fixture_placement_bindings.inc',r/'PLACEMENT_RELOAD_HANDOFF.md',*r.glob('placement_reload_*')]:
 if f.suffix in ('.cpp','.inc','.py','.md'):
  assert all(line==line.rstrip() for line in f.read_text().splitlines()),f
print(f'source checks passed: {len(names)-len(changed)} unchanged helper bodies; altered native call sequences unchanged; default/mutator closure and fixed identity checked; no live proof')
