"""Narrow source invariants, supplementary to compiled behavioral checks."""
from pathlib import Path
p=Path(__file__).resolve().parent
h=(p/'placement_record_snapshot.h').read_text();probe=(p/'placement_record_probe.cpp').read_text()
checks={
 'full64 matching': 'identity==target.identity' in h and 'target.identity==0' in h,
 'validated metadata42': 'mc.get(4)!=0x42' in h and 'md.bytes.size()!=4' in h,
 'owned body': 'std::vector<U8> body' in h and 's.body.assign(body.begin(),body.end())' in h,
 'atomic selected publication': h.index('r.selected=std::move(pending)')>h.index('if(!pending)'),
 'existing CRC seam': 'native_pollution::recordCRC(c.bytes)' in h,
 'hash before parse': probe.index('digest!=expected')<probe.index('archive.OpenFromMemory'),
 'safe read EOF and error': all(s in probe for s in ['f.bad()','!f.eof()','f.gcount()!=0','incomplete file read']),
 'unique required keys': 'metadataKey,pollutionKey,tutorialKey,buildingKey' in probe and 'uniqueExact(keys,k)' in probe,
 'no asset writing or object construction API': not any(s in h+probe for s in ['ofstream','OpenFromFile','WriteByTGI','LoadClassObjects(','reinterpret_cast<opensc4','system(','popen(']),
 'explicit diagnostic limits': 'maxRecords=100000' in h and 'maxStreamBytes=64*1024*1024' in h,
 'scope qualification': all(s in probe for s in ['HEADER_RESOLUTION_ONLY','exemplarTGI/lot=UNPROVEN','runtime_END_complete=0'])
}
for name,ok in checks.items():print(('PASS ' if ok else 'FAIL ')+name)
assert all(checks.values())
print('PASS',len(checks),'source invariants (supplementary, not semantic review)')
