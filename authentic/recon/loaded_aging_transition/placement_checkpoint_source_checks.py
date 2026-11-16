"""Read-only wiring and manifest checks; supplements actual helper tests."""
from pathlib import Path
import hashlib
import json
import re
r = Path(__file__).resolve().parent
manifest = (r / 'TUTORIAL_RELOAD_FIXTURE_IDENTITY.json').read_bytes()
assert hashlib.sha256(manifest).hexdigest() == '25dbaa34ee02113ac32520c0f28482fa0502e6985d715b9c51cb94bdb5de64a6'
files = json.loads(manifest)['files']
rows = re.findall(r'^\{"(.*)","([a-f0-9]{64})"\},$', (r / 'placement_checkpoint_saved_files.inc').read_text(), re.M)
assert len(rows) == 93 and dict(rows) == files and len(dict(rows)) == 93
s = (r / 'fixture_placement_bindings.inc').read_text()
# Remove ONLY our four explicitly guarded additions: reproduce prior worker SHA.
s = re.sub(r'^#ifdef OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION\n.*?^#endif\n', '', s, flags=re.M | re.S)
assert hashlib.sha256(s.encode()).hexdigest() == '6ae79221688ba855ddd4d830f7d77cbe7fbaa5c6457eee0c72286c3907e36711'
s = (r / 'placement_checkpoint_fragment.inc').read_text()
assert s.count('>(0x100046036)(') == 1
assert s.index('if(!consume_save())') < s.index('>(0x100046036)(')
assert 'tc::prepared_file' not in s and 'tc::neighbors(' not in s
assert s.index('complete=run();') < s.index('end(complete,reason);')
assert 'job=new fb::Job(true)' in s and 'const Positive* positive' in s
print('PASS prior worker preserved byte-exactly after removing four guarded hooks; all 93 saved hashes equal pinned manifest; save wiring')
