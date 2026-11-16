from pathlib import Path
import json, hashlib, re
root=Path(__file__).parent
source=root/'AGING_INPUT_CAPTURE_DECODED.json'
raw=source.read_bytes(); d=json.loads(raw)
sha=hashlib.sha256(raw).hexdigest()
# Pin source below; raw addresses are used ONLY as opaque alias evidence keys.
expected='0c8a1cdad080bb930c3cee6142be2c73af8264c5cd24a2e9289d3c48dcee2c42'
assert sha==expected, (sha,expected)
objects={}; variants={}; rows=[]
def number(s): return '0x'+s.removeprefix('0x')+'u'
def obj(p):
    if p.get('missing'): return 0
    key=p['property']
    if key in objects: return objects[key]
    identity=len(objects)+1; objects[key]=identity
    variant=variants.setdefault(p['variant'],len(variants)+1)
    arr=p['words'] if int(p['type'],16)&0x8000 else []
    bits='00000000' if arr else p['words'][0]
    rows.append('{%d,%d,{%s,%s},{%s}}'%(identity,variant,number(p['type']),number(bits),','.join(map(number,arr))))
    return identity
local={}; defaults={}
for p in d['properties']:
    for name,h in [('local',local),('default',defaults)]:
        identity=obj(p[name])
        if identity: h[p['id']]=identity
    assert objects[p['effective']['property']]==(local.get(p['id']) or defaults.get(p['id']))
r=d['occupantResponseCurve']; identity=obj(r)
assert dict(d['defaultVector']['entries'])['8a1e07ee']==r['property']
defaults['8a1e07ee']=identity
for name,h in [('localVector',local),('defaultVector',defaults)]:
    for key,evidence in d[name]['entries']: h.setdefault(key,0) # explicit unsupported, not invented content
lines=['#pragma once','#include "opensc4/loaded_aging_transition.h"','// Generated from accepted decoded capture SHA256 '+sha,'// raw log SHA256 '+d['rawLogSHA256'],'inline opensc4::loaded_aging_transition::Snapshot LocalAgeFixture() {','    opensc4::loaded_aging_transition::Snapshot s;','    s.properties={'+','.join(rows)+'};']
for name,h in [('local',local),('defaults',defaults)]:
    lines.append('    s.%s={%s};'%(name,','.join('{%s,%d}'%(number(k),v) for k,v in h.items())))
for name,curve in zip(['fundingCurve','factorCurve'],d['simulatorCurves']):
    assert curve['count']==len(curve['records'])
    lines.append('    s.%s={%s};'%(name,','.join('{'+','.join(map(number,r))+'}' for r in curve['records'])))
for name,value in [('fundingBits',d['funding']['bits']),('percentageZeroBits',d['runtimePercentageZeroBits']),('scaleBits',d['currentCapacity']['scaleBits']),('lotFlags',d['currentCapacity']['flagsCD'])]:
    lines.append('    s.%s=%s;'%(name,number(value)))
log=(root/'aging_input_capture_window_full.log').read_bytes()
assert hashlib.sha256(log).hexdigest()==d['rawLogSHA256']
text=log.decode()
ids=[int(v,16) for i,v in re.findall(r'AGING_INPUT_CAPTURE ID phase=INITIAL index=(\d+) raw=([0-9a-f]+)',text)]
assert len(ids)==15
row=re.search(r'TARGET_ROW phase=INITIAL .*state=(\d+) condition=(\d+) readable=1',text)
config=re.search(r'FIXTURE DESCRIPTOR .* config=([0-9a-f]+) control=',text)
assert row and config
lines.append('    s.lotRectangle={'+','.join(hex(v)+'u' for v in ids[11:15])+'};')
lines.append('    s.occupantTGI={'+','.join(hex(v)+'u' for v in ids[8:11])+'};')
lines.append('    s.lotConfig='+hex(int(config[1],16))+'u;')
lines.append('    s.lotState='+row[1]+'; s.lotCondition='+row[2]+';')
lines+=['    s.lotExists=true;','    return s;','}']
(root/'local_age_adapter_fixture.h').write_text('\n'.join(lines)+'\n')
print('generated',sha,'objects',len(objects),'variants',len(variants))
