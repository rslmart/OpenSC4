from pathlib import Path
p=Path(__file__).resolve().parents[2]
files=[p/'src/loaded_aging_transition.cpp',p/'include/opensc4/loaded_aging_transition.h']
s='\n'.join(f.read_text() for f in files)
for forbidden in ['reinterpret_cast','const_cast','dlopen','dlsym','SaveCity','0x38e8fe5b','system(']:
    assert forbidden not in s, forbidden
for required in ['aging_math::factor','aging_math::update_age','aging_math::select_request','pollution_capacity::get_current_capacity','TryGetOutputValue','ReadSint32','ReadUint32','ReadFloat32Bits']:
    assert required in s,required
assert s.index('WriteFloat(0xc910b4cf,r.afterBits)') < s.index('push_back(Event::Threshold)')
print('PASS narrow static guard: no native calls/casts/prediction shortcut; composed production kernels; write before threshold trace')
