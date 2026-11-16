#include "opensc4/dbpf.h"
#include "opensc4/sc4_subfiles.h"
#include "opensc4/civic_simulators.h"
#include <cstdio>
#include <unordered_map>
using namespace opensc4;
int main(int argc,char**argv){
 if(argc!=2)return 2;cDBPFArchive a;if(!a.Open(argv[1]))return 3;
 std::vector<Uint8> b;PollutionSimulatorSaveRecord p;
 if(!a.ReadFirstByType(kCLSID_cSC4PollutionSimulator,b)||!p.ReadFromSC4Record(b))return 4;
 std::vector<BuildingOccupantSaveRecord> buildings;
 if(!a.ReadFirstByType(kCLSID_cSC4BuildingOccupant,b)||!DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(b,buildings))return 5;
 std::unordered_map<Uint32,const BuildingOccupantSaveRecord*> index;
 for(auto& x:buildings)if(!index.emplace(x.memAddr,&x).second)return 6;
 printf("buildings=%zu\n",buildings.size());
 const OccupantRefList* lists[]={&p.listC0,&p.listD8,&p.listF0,&p.list108};const char* names[]={"C0","D8","F0","108"};
 for(unsigned i=0;i<4;++i){unsigned matches=0,missing=0,other=0;printf("list=%s count=%zu\n",names[i],lists[i]->size());
 for(auto r:*lists[i]){if(r.clsid!=kCLSID_cSC4BuildingOccupant){++other;printf("other clsid=%08x addr=%08x\n",r.clsid,r.memAddr);continue;}
 auto it=index.find(r.memAddr);if(it==index.end()){++missing;continue;}++matches;
 const auto& h=it->second->base.exemplarHolder;printf("match addr=%08x exemplar=%08x:%08x:%08x saved_properties=%zu\n",r.memAddr,h.exemplarKey.type,h.exemplarKey.group,h.exemplarKey.instance,h.properties.properties.size());
 for(auto& e:h.properties.properties){printf(" entry holder=%08x property=%08x variant=%08x dbtype=%08x count=%u bytes=",e.holderPropId,e.propertyId,e.variantTag,e.dbType,e.arrayCount);for(auto v:e.rawValueBytes)printf("%02x",v);puts("");}
 }printf("summary list=%s matches=%u missing=%u other=%u\n",names[i],matches,missing,other);}
}
