// Concrete original-game oracle bindings; include AFTER the base fragment.
// C++17 x86_64; host supplies ScopedRZString/get_app/logf_/g_slide.
#ifndef OPENSC4_FIXTURE_BINDINGS_INC
#define OPENSC4_FIXTURE_BINDINGS_INC
#include <dispatch/dispatch.h>
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
namespace placement_reload { struct Context; }
namespace fixture_placement_bindings {
using namespace fixture_placement;
// clean-room fallback (not authentic): all limits and rejection policies below
// are diagnostic policy. Only the called original methods supply game values.
static constexpr char region[] = "/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114";
static constexpr char manifest[] = "/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/DISPOSABLE_REGION_IDENTITY.json";
static constexpr char manifest_hash[] = "e59b83e27a5339ecd9ad2399daaff16e45cd93e49b111a6f6e2421281c9e7a92";
// Fixed diagnostic domain, captured in TUTORIAL_RELOAD_FIXTURE_IDENTITY.json
// and TUTORIAL_RELOAD_WINDOW_RESULT / CHECKPOINT_RELOAD_ACCEPTANCE; not game policy.
enum class FixtureKind { Unbound, OriginalPrepared, SavedInactive, DiagnosticPlaced };
static constexpr char saved_manifest[] = "/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/TUTORIAL_RELOAD_FIXTURE_IDENTITY.json";
static constexpr char saved_manifest_hash[] = "25dbaa34ee02113ac32520c0f28482fa0502e6985d715b9c51cb94bdb5de64a6";
static constexpr char saved_path[] = "/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114/City - Rush Hour Tutorial.sc4";
static constexpr char saved_sha[] = "01f91c3f091ddbb8bdeed9b0c48e60e10a8832f077355bce2a61386c2d8ae3a0";
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
static constexpr char reload_manifest[] = "/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/PLACEMENT_RELOAD_FIXTURE_IDENTITY.json";
static constexpr char reload_manifest_hash[] = "7b608df3746f52afb94563a1b3e66aa3139e276319b761ae473a3ec84c36bbce";
static constexpr char reload_sha[] = "097fed5a2730480538d25a2fae2dee2005d044cabf41be9721d8fa3f337592fe";
#endif
struct Prepared { const char* name; const char* sha; };
static constexpr Prepared prepared[] = {
{"City - New City (25).sc4","671f300d795b470e7a71d3b1529951916a1f57414eb85765a596fa429ed0517a"},
{"City - New City (72).sc4","1078ab177034caccccdfa2d23368d70a4817bda96f40e38166d38e6b254c2131"},
{"City - New City (64).sc4","cb3939c0b5ee0868fdc5d34970377a0894fd3d7d170e953191ddc9838bafa8f6"},
{"City - New City (33).sc4","bf120d8ca2dde02bbbe32d29546945650c250cf997d77400019202ee48588c91"},
{"City - New City (48).sc4","aca6cb8e0745b311d8abb79bef6bf5357fbcfa12ed14f9a861ccb8f160af6d5f"},
{"City - New City (29).sc4","b9d2c8e6835061542257558e23c0dc229c83efc424fb893ab2286a84241b39e6"},
{"City - New City (91).sc4","d1a857642597293aff6f2bdef312d8f7749b67a143b4e582827fbd203e6151da"},
{"City - New City (68).sc4","f655b29386f7d8c8992143761072b7c2fb5e5d069d4f828eb5df591689f44e6e"},
{"City - New City (87).sc4","71c3f979cb545eaf9118348b4a9dcb4f7e3ebc28e4bb6978db0f564729dab3eb"},
{"City - New City (44).sc4","863497e736ce97e241895e0091d76010be178c0e9107895999d8d2ed24a36ee0"},
{"City - New City (2).sc4","53eb2644efd48fb1a930b15359448a66eba669f826da7b04070730f5254b2997"},
{"City - New City (13).sc4","d608368afe659da47ee91a1f4bd0fda004fece3bb4b9b0545be2dfb05cf6cd7d"},
{"City - New City (52).sc4","bf927fbb1d88bbe749e095de8e71087e77310f31cdfdf5bc4bb387b78365a3bb"},
{"City - New City (53).sc4","331d32cc5997f414d85fbb03807da021a5075f1b6d597b8c1360993ef34d4e39"},
{"City - New City.sc4","f86aa11c8ef238cf907c2eb27126f0e3e97ab94a774eae5b2b4ba71f75621639"},
{"City - New City (12).sc4","191117ea899077b1e0de8e5d29bc1c12d5fe64fc66bf08d00e647083aacb6051"},
{"City - New City (3).sc4","10ed014f083785e98520e967c3d8061073d9c80fb7adf2d2e8517f31ee4a0f06"},
{"City - New City (69).sc4","2e1fefdf202bf1d6fbabe6cd0f8c81ef8a6fb707a6f378804c82e8986c560149"},
{"City - New City (86).sc4","6bf696a8a7d183e8296caeec18b057d0c35d1806c96ffe7094906fcb95c5eb51"},
{"City - New City (90).sc4","010227cea5754040256341ec7355e6f79ab26330d642a6e6c431bda953cbe874"},
{"City - New City (28).sc4","1b998434c1e42c38e25a693967f6ccef218fe986933b3c1f502e3b176605d993"},
{"City - New City (49).sc4","c6bc973dc6b7f82a70340d769c98afb5a33283c1e1b99d0d7b4f8e34bc8ef6b6"},
{"City - New City (32).sc4","d13729a6d23aa0be1f417ed7f3ce034bd9e27100d031c5db25ee2492bb4273db"},
{"City - New City (65).sc4","c9422321455ae821909ffc0135f0f1dc3551b3f16ad70a75bcd0a25cf44a643a"},
{"City - New City (73).sc4","56bdd09a155834f6c59ab1d6498c2f32150e6d26dd52f5fb9592d3d6b4f27329"},
{"City - New City (24).sc4","43f21463561522b73252343a0d054064f2e8d66acddbd93807acc2d5cf3ef2c6"},
{"City - Making Money Tutorial.sc4","4a9f064c0c27932b525d8c34dc1db653f41831ad4a9d491811518ab445933a79"},
{"City - New City (78).sc4","150e2461f7ade4491a5ae04fe8a6856d927c73acb57f3d44a265385682d7a494"},
{"City - New City (81).sc4","c22b0c4ce902f9d14caed2cd41fbcf05ffb408da0db82b348ec996651b1f2a1f"},
{"City - New City (39).sc4","87c8c101ad44ca87276108a086d4ea4d84f118368c07e732eeb14a85cb788180"},
{"City - New City (42).sc4","6c8ed0473570fa67208f184e1a7811202b5f2e225d469b37dc2823beb207a8eb"},
{"City - New City (15).sc4","2b446f861733cebf6fc61865f3f55ce11bbac3269661c11464bd1292e04166af"},
{"City - Big City Tutorial.sc4","adc2e09ef041f169db69d5d8343786790baa2d96a52f3aeb338e7719c17fc38f"},
{"City - New City (54).sc4","c7fd4c698244e246f6956b2db2415ee41728d50a22859a922db5ec2ce6fe0a15"},
{"City - New City (23).sc4","0452360d6d94a564e9657d71baaa2e40a284e4817796158e77e379d19d0f7bb6"},
{"City - New City (74).sc4","9de9aa4e4a4d1ff22fb08a3c9766fb46469a6866dfeeee116719427da2cd1d7b"},
{"City - New City (62).sc4","14d008a8e843fc7cbdd7b951ee75d7f77fa16f283bc10dbb9944e24d7ab573a9"},
{"City - New City (35).sc4","9352be7ced5865638b4c280620da6240d42bf024e0dce783d42cb8516519223b"},
{"City - New City (8).sc4","837b2c5a8a6e215af23d0aea2bc5d9204aaedddbc8a6ce72537cadd1f4f4e16b"},
{"City - New City (19).sc4","c2dc10bc2eb76b0937ace8e0573570db0372d3776da2a87d685269668a75f37b"},
{"City - New City (58).sc4","4363cd3aaac0d48dc3b9fa4e7bdc820d89d29c4ab33c880be8d0a5a9545e3e62"},
{"City - New City (59).sc4","29658bfcaacc8b55c6cb02a31a4eac396c726d42c9228c2d529ff1ee9bfdef62"},
{"City - Getting Started Tutorial.sc4","cb2405a2bf30e51f223e5d13bd6f77bca2ab63be4b5cae40b6757b5cdba58dc2"},
{"City - New City (18).sc4","d9a379b99cf0510fa0f563c59efec8d94dba46123ef55d27964e723b6106341c"},
{"City - New City (9).sc4","65bbc7c728936821de4249f4c6663f485bb1518565823e2260a6215699c92f9e"},
{"City - New City (34).sc4","592213eda3b6f25ef2558e54dcbc1024bf5c6d735c17ae4c7f2b1816ec3e4a86"},
{"City - Terraforming Tutorial.sc4","d441383b914b505d1b3dfabf5fc9d3a45d96e986ae535f55d83563d55b6f57f4"},
{"City - New City (63).sc4","e730993c378819a278382f0f4381f87246d7507f749df05b600e3b071036f04e"},
{"City - New City (75).sc4","35f5a02e138fbdc5b3a888d36d55b1d8e3a1d6623b158033c2bf0bcd2d05c423"},
{"City - New City (22).sc4","d1bf49a683b9b8afd84ac93d30108b05325dbaccc897e1603c7a49606f91210a"},
{"City - New City (55).sc4","9710a0c84fcbc1ab0e62cfda620345d602464da7c155b2a34257994aa02b2e6e"},
{"City - New City (14).sc4","a93a142cb244ba8e05fecdc7b416e4fe99e0cbacef38245a076b921af6175cfc"},
{"City - New City (43).sc4","0510b9ee34ed0256188e402a29aa028aa5133ab91509538bf01c781288b87929"},
{"City - New City (38).sc4","7fdeb42a2b4b2d6263970016fcd44286e5a75584add6a458282cd14a117fd6c4"},
{"City - New City (80).sc4","46333c9fefbc4e42a7d54abeec6163afa7b0f9a42502847a7d0c1efd14c335cb"},
{"City - New City (79).sc4","3de3224ff921321bc83827bd704fccd957aa7266ba73dd0a73ceba23e7da0071"},
{"City - New City (56).sc4","e52e63344e09f3cd1ccf61ed2944c63d90483396e6bd0a71507f0f7a253229f4"},
{"City - New City (17).sc4","3cecf8dfd864faef432b194e61e157c3f5327be4b12d89d18bde47a1a86f364b"},
{"City - New City (6).sc4","422566ae74faa6cae1bdb7119ed4f048b574638df348e09bf0e285a522800bb9"},
{"City - New City (40).sc4","2be101da707d54f90afdb82f7d6ab9be56ec892b9f9a843125ab746c3b70b274"},
{"City - New City (83).sc4","831b54bd1c745da571440368a2db11af6c2284373d028bf256a90f4c05baac87"},
{"City - New City (37).sc4","fd342a3df220d6c0efd195ec90bb23d09d3fcc370ccbdcf781a7795abf78b838"},
{"City - New City (60).sc4","5f21a546aac0aedae4d364ca0ad4452cf6ce02d5670e0c4a0cae67b05546346b"},
{"City - New City (76).sc4","fb1e05a71f658754071408ea0d6afa64fb978358148b91e77cb51e7c9c002aa5"},
{"City - New City (21).sc4","ea9af0ee641e77324a67b1c356a3f1baf2ecba1aaf313211b05dfdc6e15a07e2"},
{"City - New City (20).sc4","d3f22211d4b187334f365406c8ee53444e26702ee797dc92262c71f8fa775244"},
{"City - New City (77).sc4","b3f6b86751b26c496089f742c86aeafb71b0ec6cfef6aad17bf131f2ed09281b"},
{"City - New City (61).sc4","001c33d8118eb69464227abbc6dd24fc49183dd5e7e442b0ebf40290da7241be"},
{"City - New City (36).sc4","dc0a6912766814ed93f31386bb372ea18bf2203b3a879f6533a2fcca050ab56c"},
{"City - New City (82).sc4","c4dea44a03fcc64ec85181046c73e302c7c3eb58944dcc565758448e906eb70e"},
{"City - New City (41).sc4","4be1165da1d46ce644ac415aca60641b927e76933c59ce28a1f48fc6a093f91c"},
{"City - New City (7).sc4","6cf644400edd9b0ea56d3d1cd6bcf7be52561aaf0c1f2ef0265fa94f558a7af2"},
{"City - New City (16).sc4","fab3d7157f521109b4ac0b03ef49d68cf36e21c6477ea07c6d0bc9892a0bb2c0"},
{"City - Rush Hour Tutorial.sc4","81ee61200118484b05f1e17ba00545e99c9db7780e147b4e049ce5d5fb2b40c1"},
{"City - New City (57).sc4","b8da934bfc7b81cfee1a521c6c9b90e98e35f817cef7dfe8278ec50a064a0152"},
{"City - New City (89).sc4","e4302a25fca318002fa84f17ab2772e8eabbae6271c0e8618b7e7fc43cf075cb"},
{"City - New City (66).sc4","1f5b7b7b865ba470bc0b0c5b4ffe65048f70b901cd0bea48fa8fe904f20e8619"},
{"City - New City (70).sc4","896f667ce89981fa41c5c8003f69b8e3b48ccee9231f9adb72650cd813b75a12"},
{"City - New City (27).sc4","c8b0dc6eba4728165abccd2de907d7cd8f48504a5ccb165fc19ec68b4458b0d2"},
{"City - New City (50).sc4","64f92c3e3520851f675dfd9934a966181d9fd7bb77a681bdec9d02658ac67cf7"},
{"City - New City (11).sc4","fd54d0381302931a1ffbdf4f654a9d930a50eb57fe5bfafadb5ff65c70f2e86b"},
{"City - New City (46).sc4","2e7b3ab00e784702844a7468b6d5f7330cae4ac2aaa967c6850b184def699236"},
{"City - New City (85).sc4","999ebdb7c91f51179351c5df53d6328b72c433cc0448f53eb822f8fb97207cdb"},
{"City - New City (84).sc4","6de32c1d0211a4c5351bde57cf9c8881aec0ae1a5ea70907b5d787b593627d51"},
{"City - New City (47).sc4","14334c8d2ba4f3b3ebff27023db200c239287f351a9030a80dd531946ceba1a0"},
{"City - New City (10).sc4","3020f5d2e87d001c6521d93a105826868902672dddf7b21764110ea73848fa93"},
{"City - New City (51).sc4","9d921486c8043fa14bc1ccc30e2414660810a3c4f616be798389ae2e4c00e99f"},
{"City - New City (26).sc4","a0f6bda65638a5ec2e55ec2f1eef515aea942a09f1e02727c04faa384e0d2eab"},
{"City - New City (71).sc4","acc47736228b3bcd57098bdabe2b4cecd89d0ab90abe646837ba3e9eeb751d4d"},
{"City - New City (88).sc4","e46e627558db9a87af8aafec74a715f332be145c2f86fab97e858aabf450f3fa"},
{"City - New City (67).sc4","500022f86abb2f35124398e41a9ea71d05ef8ded5eaf2315c71aa2ffea9ea522"},
{"City - New City (30).sc4","f414d39067a43664a103889f7c742730427ea130cdf51b6abc1eddd7e61347f5"},
};
static constexpr Entry binding_entries[] = {
{0x10001709a,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xfb,0x48,0x8b,0x03,0xff,0x90,0x80,0x00}},
{0x100402fba,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x0c,0x5d,0xc3,0x90,0x55,0x48,0x89,0xe5,0x48,0x8d}},
{0x10015387a,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x87,0xd8,0x00,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48}},
{0x10018399e,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x58,0x5d,0xc3,0x55,0x48,0x89,0xe5,0x0f,0xb6}},
{0x100164678,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x28,0x49,0x89,0xce}},
{0x10074e056,{0x55,0x48,0x89,0xe5,0x81,0xfe,0xd3,0xa3,0xfd,0xe4,0x7e,0x1e,0x81,0xfe,0xd4,0xa3}},
{0x10074da7c,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x07,0x48,0x8b,0x40,0x38,0x45,0x31,0xc9,0x5d,0xff}},
{0x100143ebc,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x100767b12,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x1006c6b4c,{0x55,0x48,0x89,0xe5,0x4c,0x8b,0x4f,0x08,0x4d,0x85,0xc9,0x74,0x6b,0x8b,0x0e,0x4d}},

{0x1003b2cd0,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x87,0x10,0x01,0x00,0x00,0x48,0x8b,0x0e,0x48,0x8b}},
{0x1000480ee,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x47,0x70,0x48,0x8b,0x0e,0x48,0x8b,0x49,0x78,0x48}},
{0x10004852a,{0x55,0x48,0x89,0xe5,0x80,0x7f,0x45,0x00,0x0f,0x95,0xc0,0x5d,0xc3,0x90,0x55,0x48}},
{0x100767d26,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x30,0x45}},
{0x1007615c4,{0x55,0x48,0x89,0xe5,0x8b,0x87,0xb4,0x00,0x00,0x00,0x89,0x06,0x8b,0x87,0xb8,0x00}},
{0x1006c4166,{0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x10,0x48,0x8d,0x45,0xfc,0x89,0x30,0x48,0x8d}},
{0x100131184,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0xf8,0x5d,0xe9,0x22,0xff,0xff,0xff,0x55,0x48}},
{0x100131192,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x10,0x41}},
{0x10019bb80,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x47,0x10,0x5d,0xc3,0x90,0x90,0x90,0x90,0x90,0x90}},
{0x100305dc6,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x48,0x83,0xec,0x10,0x48,0x89,0xf3,0x48,0x8d}},
{0x10019b46c,{0x55,0x48,0x89,0xe5,0xf3,0x0f,0x10,0x47,0x64,0xf3,0x0f,0x58,0x47,0x70,0xf3,0x0f}},
{0x10019b558,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x64,0x89,0x06,0x8b,0x47,0x68,0x89,0x46,0x04,0x8b}},
{0x10017fa80,{0x55,0x48,0x89,0xe5,0x0f,0xb6,0x47,0x40,0x89,0x06,0x0f,0xb6,0x47,0x41,0x89,0x46}},
{0x100183a68,{0x55,0x48,0x89,0xe5,0x0f,0xb6,0x87,0xd3,0x00,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48}},
{0x100183a76,{0x55,0x48,0x89,0xe5,0x0f,0xb6,0x87,0xd4,0x00,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48}},
{0x10004ae40,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x41,0x89,0xf6,0x48,0x89,0xfb,0xe8,0x72,0xf1}},
{0x10004c650,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x47,0x08,0x5d,0xc3,0x55,0x48,0x89,0xe5,0x48,0x8d}},
{0x10004c63c,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x20,0x5d,0xc3,0x90,0x55,0x48,0x89,0xe5,0x89,0x77}},
{0x100051c00,{0x55,0x48,0x89,0xe5,0x0f,0xb7,0x47,0x12,0x5d,0xc3,0x90,0x90,0x90,0x90,0x90,0x90}},
{0x10004c682,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x14,0x5d,0xc3,0x90,0x55,0x48,0x89,0xe5,0x48,0x8d}},
{0x10004c804,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x08,0x5d,0xc3,0x90,0x90,0x90,0x90,0x90,0x90}},
{0x10005f070,{0x55,0x48,0x89,0xe5,0xf6,0x47,0x08,0x01,0x75,0x02,0x5d,0xc3,0x48,0x8b,0x7f,0x18}},
{0x1004879e0,{0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x10,0x48,0x89,0x7d,0xf8,0x48,0x89,0x75,0xf0}},
};
static constexpr uint64_t cSC4Demolition_slots[] = {0x1000eb084,0x1000eafac,0x1000eafd6,0x1000eb0ea,0x1000eb3d2,0x1000eb86e,0x1000eb874,0x1000ed13e,0x1000ed496,0x1000edb00,0x1000ee0e2,0x1000ee4e6,0x1000eaf70,0x1000eafa2,0x1000eb4b0,0x1000eb508,0x1000ee64a,0x1000eb58c};
static constexpr uint64_t cSC4Demolition_vptr = 0x100a71830;
static constexpr uint64_t cSC4ZoneManager_slots[] = {0x10019829c,0x100198302,0x10019832c,0x1001983e4,0x100198810,0x100199ff6,0x1001992ba,0x1001992c8,0x1001992d4,0x1001992e4,0x10019939c,0x1001993a8,0x100199e64,0x100199e98,0x100199eb4,0x10019a000,0x10019a010,0x100199ee0,0x100199eee,0x10019a01e,0x10019a02e,0x1001981d0,0x100198234,0x1001988ee,0x100198bf6,0x10019a03e,0x100198ffe};
static constexpr uint64_t cSC4ZoneManager_vptr = 0x100a7b750;
static constexpr uint64_t cSC4BudgetSimulator_slots[] = {0x10021e1e8,0x10021e24e,0x10021e278,0x10021e330,0x10021f1fc,0x10022132a,0x100221344,0x10022134e,0x10022139e,0x1002213d6,0x1002213e4,0x1002213ee,0x100221452,0x10022145c,0x100220ff0,0x10022300c,0x1002210a8,0x10022301a,0x1002214bc,0x1002214ca,0x100223028,0x100223036,0x100223044,0x100223052,0x100222fa4,0x100221522,0x10022176a,0x10022184e,0x100221890,0x100221b7c,0x100221d0e,0x100221d30,0x100221f28,0x100221f56,0x100221f84,0x100221f8c,0x100221f94,0x100221fb2,0x1002222c4,0x100221fce,0x100221fdc,0x100222288,0x100222292,0x1002222a8,0x1002222b6,0x10022245e,0x1002222e2,0x100223060,0x1002223d2,0x10022243a,0x100222484,0x100222758,0x100222800,0x1002228da,0x1002229b4,0x100222ac0,0x100222b84,0x100222c4a,0x100222c84,0x10021e1b0,0x10021e1ba,0x10021f4a6,0x10021f918,0x10022306c,0x1002201fc,0x1002201c4};
static constexpr uint64_t cSC4BudgetSimulator_vptr = 0x100a87140;
static constexpr uint64_t cSC4BuildingDevelopmentSimulator_slots[] = {0x10014c16c,0x10014c1e8,0x10014c220,0x10014ce64,0x10014d8ec,0x100150c2a,0x100150d3e,0x100150d7a,0x100151146,0x1001511da,0x100151218,0x10015125e,0x1001512bc,0x10015131a,0x10015135c,0x100151386,0x1001513ca,0x100151654,0x100151b08,0x100151ba0,0x100151bfc,0x100151c60,0x100151d48,0x100151d84,0x1001522ce,0x100152302,0x100151fc4,0x100151ca2,0x100151d06,0x100152162,0x100153844,0x100153852,0x100153572,0x10015359a,0x10015385e,0x10015386c,0x10015387a,0x1001534b4,0x100153490,0x10015343e,0x100153462,0x100153888,0x100153896,0x1001538c6,0x1001538d4,0x1001533ee,0x100150b0c,0x100150c0c,0x1001535e4,0x10015362a,0x100153644,0x1001536b2,0x100153714,0x1001536e8,0x1001538e2,0x1001538ee,0x1001538fa,0x100153906,0x100153912,0x10014c134,0x10014c13e,0x10014dd20,0x10014e064,0x10015391e,0x10014e83c,0x10014e84c};
static constexpr uint64_t cSC4BuildingDevelopmentSimulator_vptr = 0x100a76080;
static constexpr uint64_t cSC4AuraSimulator_slots[] = {0x1002182e0,0x100218340,0x10021836a,0x100218422,0x100219384,0x10021b364,0x10021b3ba,0x10021b384,0x10021b376,0x10021d2fe,0x10021d316,0x10021d272,0x10021d32e,0x10021d338,0x10021d342,0x10021d350,0x1002182a8,0x1002182b2,0x10021997e,0x100219b70,0x10021d35e,0x100219f62};
static constexpr uint64_t cSC4AuraSimulator_vptr = 0x100a86fc0;
static constexpr uint64_t cSC4EffectsManager_slots[] = {0x1005dd616,0x1005dd6ce,0x1005dd746,0x1005dba0e,0x1005dcd62,0x1005e06ce,0x1005e075c,0x1005e0c08,0x1005e1318,0x1005e341a,0x1005e3428,0x1005db97a,0x1005db9b2,0x1005dd78e,0x1005dd7b8,0x1005df48a,0x1005e06be,0x1005e0d32,0x1005e0dea};
static constexpr uint64_t cSC4EffectsManager_vptr = 0x100ad3ba0;
static constexpr uint64_t cSC4OccupantManager_slots[] = {0x100246e38,0x100246cf6,0x100246db0,0x100246e98,0x1002470a6,0x1002475ac,0x100247626,0x100247636,0x100247646,0x100247852,0x100247656,0x100247668,0x10024767a,0x10024768e,0x1002476ba,0x1002476e6,0x100247744,0x100247764,0x1002477b0,0x1002477d0,0x1002478b6,0x100247be0,0x100247dea,0x100247e0a,0x100248144,0x10024787a,0x100248884,0x100248640,0x100248970,0x100248996,0x100248af6,0x100248b1e,0x100248c0a,0x100248c32,0x100248df6,0x100248e1e,0x100249114,0x100249150,0x10024961a,0x100249656,0x100249960,0x10024999c,0x100249b22,0x100249b5e,0x10024a0fa,0x10024a136,0x10024a5a4,0x10024a568,0x10024a728,0x100246cbe,0x100246cc8,0x10024712a,0x1002472f2,0x10024a758,0x100247588};
static constexpr uint64_t cSC4OccupantManager_vptr = 0x100a890f0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static bool file_hash(const char* path, const char* expected) {
    struct stat a{}, z{};
    if (lstat(path,&a) || !S_ISREG(a.st_mode) || a.st_size<=0 || a.st_size>128*1024*1024) return false;
    FILE* f=std::fopen(path,"rb"); if(!f) return false;
    CC_SHA256_CTX c; CC_SHA256_Init(&c); unsigned char b[16384], d[32]; size_t total=0,n;
    while ((n=std::fread(b,1,sizeof b,f))) { total+=n; if(total>128*1024*1024) break; CC_SHA256_Update(&c,b,CC_LONG(n)); }
    bool ok=!std::ferror(f) && total==size_t(a.st_size) && !fstat(fileno(f),&z) &&
        a.st_dev==z.st_dev && a.st_ino==z.st_ino && a.st_size==z.st_size &&
        a.st_mtimespec.tv_sec==z.st_mtimespec.tv_sec && a.st_mtimespec.tv_nsec==z.st_mtimespec.tv_nsec;
    std::fclose(f); CC_SHA256_Final(d,&c); char hex[65]={};
    for(unsigned i=0;i<32;++i) std::snprintf(hex+2*i,3,"%02x",d[i]);
    return ok && !std::strcmp(hex,expected);
}
#pragma clang diagnostic pop
static bool entries_guard() {
    for(const Entry& e:binding_entries) { unsigned char b[16];
        if(!read(relocated(e.address),b,sizeof b) || std::memcmp(b,e.bytes,sizeof b)) return false; }
    return true;
}
static bool string_copy(const ScopedRZString& s, char (&out)[1024]) {
    // Host c_str ABI: small string tag+8, inline+9, heap+18. Read boundedly.
    uint64_t p=reinterpret_cast<uint64_t>(s.buf)+9;
    if(s.buf[8]&1) std::memcpy(&p,s.buf+0x18,8);
    for(unsigned i=0;i<sizeof out;++i) { if(!read(p+i,out[i])) return false; if(!out[i]) return i>0; }
    return false;
}
// GetEyeRay @100164843 and _UpdateProjection @100164278 read through +1ab.
static constexpr size_t camera_extent = 0x1ac;
static bool projection_clean(const unsigned char* bytes) {
    uint32_t cached=0,generation=0;
    std::memcpy(&cached,bytes+0x160,4); std::memcpy(&generation,bytes+0x180,4);
    return cached==generation;
}
static bool camera_equal(const unsigned char* a,const unsigned char* b) {
    // UnProject @100164925 refreshes when these words differ. Both snapshots
    // must skip refresh; all inputs and matrices remain byte-identical.
    return projection_clean(a) && projection_clean(b) &&
        !std::memcmp(a,b,0x160) && !std::memcmp(a+0x164,b+0x164,0x1c) &&
        !std::memcmp(a+0x184,b+0x184,camera_extent-0x184);
}
struct Owner {
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
private:
    bool diagnostic_reload=false;
    friend struct placement_reload::Context;
public:
    bool reload_allowed() const { return diagnostic_reload; }
#endif
    uint64_t city=0, regional=0;
    char path[1024]={};
    State receipt{}; Point points[32]={}; unsigned count=0;
    uint64_t camera=0; unsigned char camera_bytes[camera_extent]={};
    bool inspected=false, spent=false;
    FixtureKind kind=FixtureKind::Unbound; // host receipt only; immutable after identity binds
};
// clean-room fallback (not authentic): ADAPTER observation candidate policy.
// Owned host values only. Pointer numbers identify this process window, never saves.
// This is NOT a save receipt: Job/click/later samples/PID latches/presave guards
// must be bound by a later integration before publishing any immutable receipt.
enum class PropertyStatus { Unreadable, Missing, Supported, Unsupported };
struct RawPropertyObservation {
    PropertyStatus status=PropertyStatus::Unreadable;
    bool complete=false;
    uint32_t id=0,type=0,count=0,word_count=0;
    uint32_t words[256]={};
};
struct LotObservation {
    bool complete=false;
    uint64_t lot=0,descriptor=0;
    Rect rect{}; uint32_t state=0,condition=0,raw_bounds[4]={};
};
struct OccupantObservation {
    bool complete=false;
    uint64_t occupant=0,base=0,holder=0,exemplar=0;
    uint32_t tgi[3]={},position_bounds[9]={},stored_position[6]={};
    LotObservation lot{};
    RawPropertyObservation properties[10]={};
};
struct ListOccurrenceObservation {
    unsigned offset=0; uint64_t index=0,node=0,prev=0,next=0;
    OccupantObservation value{};
};
struct InventoryObservation {
    bool complete=false,identity_complete=false,owner_inspected=false,owner_spent=false;
    int64_t pid=0;
    State initial{},final{},owner_receipt{};
    FixtureKind kind=FixtureKind::Unbound;
    uint64_t city=0,regional=0,guarded_descriptor=0,pollution=0,lot_manager=0;
    std::string region_path,path,manifest_path,manifest_sha256;
    std::vector<ListOccurrenceObservation> occurrences; // every occurrence, no deduplication
    uint64_t list_counts[3]={};
    size_t matching_c0_count=0,selected_occurrence=0;
    uint64_t selected_index=0,selected_node=0,selected_occupant=0;
};
static bool copy_property_words(RawPropertyObservation& out,const uint32_t* words,unsigned n) {
    out.complete=false; out.word_count=0; out.status=PropertyStatus::Unreadable;
    if(n>256 || (n && !words)) return false;
    if(n) std::memcpy(out.words,words,n*sizeof(uint32_t));
    out.word_count=n; out.status=PropertyStatus::Supported; out.complete=true;
    return true;
}
static void observe_owner(InventoryObservation& out,const Owner& o) {
    out.identity_complete=false;
    out.kind=o.kind; out.city=o.city; out.regional=o.regional;
    out.owner_inspected=o.inspected; out.owner_spent=o.spent; out.owner_receipt=o.receipt;
    out.region_path=region;
    const size_t n=strnlen(o.path,sizeof o.path);
    if(n==sizeof o.path) return;
    out.path.assign(o.path,n);
    out.manifest_path=o.kind==FixtureKind::SavedInactive?saved_manifest:manifest;
    out.manifest_sha256=o.kind==FixtureKind::SavedInactive?saved_manifest_hash:manifest_hash;
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    if(o.kind==FixtureKind::DiagnosticPlaced) {
        out.manifest_path=reload_manifest; out.manifest_sha256=reload_manifest_hash;
    }
#endif
    out.identity_complete=true;
}
static bool observed_property(const RawPropertyObservation& p,uint32_t id,uint32_t type,
                              uint32_t count,const uint32_t* words,unsigned n) {
    return p.complete && p.status==PropertyStatus::Supported && p.id==id &&
        p.type==type && p.count==count && p.word_count==n && n<=256 &&
        !std::memcmp(p.words,words,n*sizeof(uint32_t));
}
static bool placement_target(const OccupantObservation& o,uint64_t descriptor) {
    if(!o.complete || !o.occupant || !o.base || !o.holder || !o.exemplar ||
       o.base!=o.occupant-8 || o.holder!=o.occupant+0x10 ||
       o.tgi[0]!=0x6534284a || o.tgi[1]!=0xc8dbccba || o.tgi[2]!=0x1f4b0000 ||
       !o.lot.complete || !o.lot.lot || !descriptor || o.lot.descriptor!=descriptor ||
       o.lot.rect.l!=58 || o.lot.rect.t!=66 || o.lot.rect.r!=63 || o.lot.rect.b!=72 ||
       o.lot.state!=3 || o.lot.condition!=0) return false;
    static constexpr uint32_t ids[]={0xc8ebd131,0xa8ebcf0c,0x88ed1059,0x27812852,0xc910b4cf,
        0x8a1e07ee,0x28ecf510,0xea1cf220,0xea1cf221,0xea1cf222};
    static constexpr uint32_t curve[]={0x00000000,0x3f800000,0x3f000000,0x3f733333,
        0x3f266666,0x3f666666,0x3f400000,0x3f4ccccd,0x3f666666,0x3f0ccccd,
        0x3f800000,0x3e99999a,0x3f8ccccd,0x00000000};
    static constexpr uint32_t scalar[]={0,0,0,0,0,0,0,0x3a919ef9,0x3f800000,0x3f8ccccd};
    for(unsigned i=0;i<10;++i) {
        const auto& p=o.properties[i];
        if(i>=1 && i<=3) {
            if(!p.complete || p.id!=ids[i] || p.status!=PropertyStatus::Missing || p.word_count) return false;
        } else if(!observed_property(p,ids[i],i==0?6:i==6?7:i==5?0x800a:0xa,
                                    i==5?14:0,i==5?curve:scalar+i,i==5?14:1)) return false;
    }
    return true;
}
struct PlacementSelection {
    size_t count=0,occurrence=0; uint64_t index=0,node=0,occupant=0;
};
static PlacementSelection placement_selection(const InventoryObservation& o) {
    PlacementSelection r{};
    for(size_t i=0;i<o.occurrences.size();++i) {
        const auto& v=o.occurrences[i];
        if(v.offset==0xc0 && placement_target(v.value,o.guarded_descriptor)) {
            if(!r.count) { r.occurrence=i; r.index=v.index; r.node=v.node; r.occupant=v.value.occupant; }
            ++r.count;
        }
    }
    return r;
}
static bool placement_candidate_matches(const InventoryObservation& o) {
    // Conservative diagnostic restriction: exactly one MATCHING C0 occurrence.
    // Original inventory accepts empty lists, duplicates and unrelated occupants.
    if(!o.complete || !o.identity_complete || o.pid<=0 || !o.owner_inspected ||
       o.kind!=FixtureKind::SavedInactive || !o.city || !o.regional ||
       o.city!=o.initial.city || !o.initial.app || !o.initial.sim || !o.initial.view || !o.initial.renderer ||
       !same(o.initial,o.final) || o.owner_receipt.app!=o.initial.app ||
       o.owner_receipt.city!=o.initial.city || o.owner_receipt.sim!=o.initial.sim ||
       o.region_path!=region || o.path!=saved_path || o.manifest_path!=saved_manifest ||
       o.manifest_sha256!=saved_manifest_hash || !o.guarded_descriptor) return false;
    uint64_t counts[3]={};
    for(const auto& v:o.occurrences) {
        const unsigned k=v.offset==0xc0?0:v.offset==0xd8?1:v.offset==0x108?2:3;
        if(k==3 || !v.node || !v.value.complete || v.index!=counts[k]++) return false;
    }
    for(unsigned i=0;i<3;++i) if(counts[i]!=o.list_counts[i]) return false;
    const auto r=placement_selection(o);
    return r.count==1 && o.matching_c0_count==r.count && o.selected_occurrence==r.occurrence &&
        o.selected_index==r.index && o.selected_node==r.node && o.selected_occupant==r.occupant;
}
static void finish_inventory_observation(InventoryObservation& o,const State& end,bool ok) {
    o.final=end; o.complete=ok && same(o.initial,end);
    const auto r=placement_selection(o);
    o.matching_c0_count=r.count; o.selected_occurrence=r.occurrence;
    o.selected_index=r.index; o.selected_node=r.node; o.selected_occupant=r.occupant;
}
static void retain_successful_points(const Context& c, Owner& owner) {
    owner.count=0;
    for(unsigned i=0;i<c.point_count && i<32;++i)
        if(c.inspection_valid[i]) owner.points[owner.count++]=c.points[i];
}
static bool receipt_allows_point(const Owner& owner,Point point) {
    if(!owner.inspected || owner.spent) return false;
    for(unsigned i=0;i<owner.count && i<32;++i)
        if(owner.points[i].x==point.x && owner.points[i].y==point.y) return true;
    return false;
}
// Saved profile requires canonical components and a single-link regular selected file.
// Recheck metadata after hashing; no copies, repairs, or native calls.
static bool saved_file(const char* path) {
    if(!path || path[0]!='/' || std::strlen(path)>=1024) return false;
    char copy[1024]; std::strcpy(copy,path);
    for(char* q=copy+1;;++q) {
        if(*q && *q!='/') continue;
        const char saved=*q; *q=0; struct stat st{}; char canonical[PATH_MAX]={};
        if(lstat(copy,&st) || S_ISLNK(st.st_mode) || !realpath(copy,canonical) ||
           std::strcmp(copy,canonical) || (saved ? !S_ISDIR(st.st_mode) :
           (!S_ISREG(st.st_mode) || st.st_nlink!=1))) return false;
        *q=saved; if(!saved) return true;
    }
}
static bool saved_inactive(uint64_t city) {
    // Raw Mach reads only, as tutorial_reload_fragment.collect / tfexit_state.
    // No registry GUID/active or UI getter requirement: those are separate domains.
    uint64_t tutorial=0,ui=0; uint8_t active=0xff;
    return read(city+0x340,tutorial) && object(tutorial,0x100a6b7c0,0x68) &&
        read(tutorial+0x2f,active) && active==0 && read(tutorial+0x60,ui) && ui==0;
}
static FixtureKind fixture_profile(const char* path,uint64_t city,bool diagnostic=false) {
    const size_t n=sizeof(region)-1;
    if(std::strncmp(path,region,n) || path[n]!='/' || !path[n+1] || std::strchr(path+n+1,'/'))
        return FixtureKind::Unbound;
    const char* name=path+n+1;
    for(const Prepared& p:prepared)
        if(!std::strcmp(name,p.name) && file_hash(manifest,manifest_hash) && file_hash(path,p.sha))
            return FixtureKind::OriginalPrepared;
    if(!std::strcmp(path,saved_path) && saved_file(path) &&
       file_hash(saved_manifest,saved_manifest_hash) && file_hash(path,saved_sha) &&
       saved_file(path) && saved_inactive(city)) return FixtureKind::SavedInactive;
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    if(diagnostic && !std::strcmp(path,saved_path) && saved_file(path) &&
       file_hash(reload_manifest,reload_manifest_hash) && file_hash(path,reload_sha) &&
       saved_file(path) && saved_inactive(city)) return FixtureKind::DiagnosticPlaced;
#else
    (void)diagnostic;
#endif
    return FixtureKind::Unbound;
}
// Regional getter @ 0x100013688 proves app+98 is pointer to a ref, NOT city.
// Both original path getters must agree before any caller path is considered.
static bool loaded_path(const State& s, char (&path)[1024], uint64_t& regional, FixtureKind* accepted=nullptr,bool diagnostic=false) {
    if(accepted) *accepted=FixtureKind::Unbound;
    uint64_t ref=0;
    if(!entries_guard() || !read(s.app+0x98,ref) || !read(ref,regional) ||
       !object(regional,0x100aa3230,0x128) || !slot(regional,0xf8,0x1003b2cd0) ||
       !slot(s.city,0x50,0x1000480ee) || !slot(s.city,0xb0,0x10004852a) ||
       !original<bool(*)(uint64_t)>(0x10004852a)(s.city)) return false;
    ScopedRZString rp, cp;
    if(!slot(reinterpret_cast<uint64_t>(rp.buf),0x78,0x10001709a)) return false;
    original<void(*)(uint64_t,void*)>(0x1003b2cd0)(regional,rp.buf);
    original<void(*)(uint64_t,void*)>(0x1000480ee)(s.city,cp.buf);
    char r[1024]={},c[1024]={},canonical[PATH_MAX]={};
    if(!string_copy(rp,r) || !string_copy(cp,c) || std::strcmp(r,c) || !realpath(r,canonical) ||
       std::strcmp(r,canonical) || std::strlen(r)>=sizeof path ||
       std::strncmp(r,region,sizeof(region)-1) || r[sizeof(region)-1]!='/') return false;
    const FixtureKind kind=fixture_profile(r,s.city,diagnostic);
    if(kind==FixtureKind::Unbound) return false;
    std::strcpy(path,r);
    if(accepted) *accepted=kind;
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    if(kind==FixtureKind::DiagnosticPlaced) {
        logf_("FIXTURE IDENTITY city=%llx regional=%llx path=%s profile=DiagnosticPlaced prepared_sha=%s manifest=%s manifest_sha=%s saved_inactive_guard=pass diagnostic_only=1",U(s.city),U(regional),path,reload_sha,reload_manifest,reload_manifest_hash);
        return true;
    }
#endif
    const bool saved=kind==FixtureKind::SavedInactive;
    const char* sha=saved_sha;
    if(!saved) for(const Prepared& p:prepared) if(!std::strcmp(r+sizeof(region),p.name)) sha=p.sha;
    logf_("FIXTURE IDENTITY city=%llx regional=%llx path=%s profile=%s prepared_sha=%s manifest=%s manifest_sha=%s saved_inactive_guard=%s",
        U(s.city),U(regional),path,saved?"SavedInactive":"OriginalPrepared",
        sha,
        saved?saved_manifest:manifest,saved?saved_manifest_hash:manifest_hash,saved?"pass":"not_required");
    return true;
}
static bool identity(void* opaque,const State& s) {
    Owner& o=*static_cast<Owner*>(opaque); char path[1024]={}; uint64_t regional=0;
    FixtureKind kind=FixtureKind::Unbound;
    bool diagnostic=false;
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    diagnostic=o.reload_allowed();
#endif
    if(!loaded_path(s,path,regional,&kind,diagnostic) || kind==FixtureKind::Unbound || (o.inspected &&
       (o.city!=s.city || o.regional!=regional || std::strcmp(o.path,path)))) return false;
    if(o.kind!=FixtureKind::Unbound && o.kind!=kind) return false;
    o.kind=kind; // legacy inspected Owners bind only after all prior comparisons pass
    return true;
}
template<size_t N> static bool service(uint64_t global,uint64_t vt,const uint64_t (&slots)[N]) {
    uint64_t p=0; if(!read(relocated(global),p) || !object(p,vt,8)) return false;
    for(size_t i=0;i<N;++i) if(!slot(p,unsigned(i*8),slots[i])) return false;
    logf_("FIXTURE SERVICE global=%llx object=%llx vptr=%llx slots=%u",U(global),U(p),U(vt),unsigned(N));
    return true;
}
static bool services_impl(void* opaque,const State& s,uint64_t control,uint64_t* observed_descriptor) {
    if(observed_descriptor) *observed_descriptor=0;
    if(!identity(opaque,s) || !entries_guard() ||
       !slot(s.view,0xc8,0x1007823ae) || !slot(s.view,0x140,0x10079132c) ||
       !slot(s.view,0x150,0x100793abe) || !slot(s.view,0x160,0x100791c5a) ||
       !object(s.renderer,0x100aebdc0,0x290) || !slot(s.renderer,0x1e8,0x100767d26) ||
       !slot(s.renderer,0x1e0,0x100767b12) || !slot(s.renderer,0x58,0x1007615c4)) return false;
    uint32_t w=0,h=0; uint64_t camera=0,terrain=0;
    original<void(*)(uint64_t,uint32_t&,uint32_t&)>(0x1007615c4)(s.renderer,w,h);
    if(w!=uint64_t(int64_t(s.area.r)-s.area.l) || h!=uint64_t(int64_t(s.area.b)-s.area.t) ||
       !read(s.renderer+0xd8,camera) || !object(camera,0x100a784c0,camera_extent) ||
       !read(s.renderer+0x288,terrain) || !object(terrain,0x100aeab80,8) || !slot(terrain,0,0x10074e056) || !slot(terrain,0x20,0x10074da7c)) return false;
    logf_("FIXTURE VIEWPORT w=%u h=%u camera=%llx terrain=%llx client_domain=matched",w,h,U(camera),U(terrain));
#define FB_SERVICE(g,c) if(!service(g,c##_vptr,c##_slots)) return false
    // Immediate original globals in PlaceLot @ 0x1003beef8. Exact primary
    // tables checked, including every contiguous text slot emitted above.
    FB_SERVICE(0x100b21330,cSC4Demolition);
    FB_SERVICE(0x100b21358,cSC4ZoneManager);
    FB_SERVICE(0x100b213f8,cSC4BudgetSimulator);
    FB_SERVICE(0x100b21400,cSC4BuildingDevelopmentSimulator);
    FB_SERVICE(0x100b213f0,cSC4AuraSimulator);
    FB_SERVICE(0x100b21368,cSC4EffectsManager);
    FB_SERVICE(0x100b21348,cSC4OccupantManager);
#undef FB_SERVICE
    uint64_t wm=0,viewwm=0;
    // RZ::WM @ 0x100127d34 cached pointer; GetWindowManager @
    // 0x10016d194 returns IGZWin+8. Catalog uses GetCursorScreenPosition+108.
    if(!read(relocated(0x100b7b120),wm) || !object(wm,0x100ad8970,8) ||
       !slot(wm,0x108,0x100622ee0) || !read(s.view+0x20,viewwm) || viewwm!=wm) return false;
    // Bounded parent chain: GetParentWin @ 0x100169784 reads +58.
    // Children visited by the catalog's recursive hit-test remain an original
    // transitive fault boundary; this is NOT a complete hierarchy census.
    uint64_t win=s.view+0x18, seen[32]={}; unsigned depth=0;
    while(win) {
        if(depth==32) return false;
        for(unsigned i=0;i<depth;++i) if(seen[i]==win) return false;
        seen[depth++]=win;
        uint64_t parent=0,managerptr=0; Rect area{};
        if(!span(win,0x130) || !slot(win,0x30,0x10016d194) || !slot(win,0x58,0x100169784) ||
           !slot(win,0x188,0x10016c974) || !read(win+8,managerptr) || managerptr!=wm ||
           !read(win+0x20,area) || !read(win+0x58,parent)) return false;
        logf_("FIXTURE WINDOW depth=%u win=%llx parent=%llx absolute=%d,%d,%d,%d",depth,U(win),U(parent),area.l,area.t,area.r,area.b);
        win=parent;
    }
    uint64_t manager=0,lotmanager=0;
    if(!read(relocated(0x100b21340),lotmanager) || lotmanager<8 ||
       !object(lotmanager-8,0x100a758e0,0xa0) || !object(lotmanager,0x100a75a28,8) ||
       !slot(lotmanager,0xb0,0x100132caa) || !slot(lotmanager,0x70,0x100131184) || !slot(lotmanager-8,0x80,0x100131192) ||
       !read(relocated(0x100b21360),manager) || !object(manager,0x100ae1f90,8) ||
       !slot(manager,0x40,0x1006c4166)) return false;
    if(control) { uint64_t bound=0;
        if(!fixture_placement::control(control,s.view) || !read(control+0x68,bound) || bound!=manager) return false;
    }
    const uint64_t descriptor=original<uint64_t(*)(uint64_t,uint32_t)>(0x1006c4166)(manager,0x04000004);
    if(!object(descriptor,0x100aacc30,0x10) || !slot(descriptor,0x18,0x100402fba) ||
       original<uint32_t(*)(uint64_t)>(0x100402fba)(descriptor)!=0x04000004) return false;
    uint64_t devsim=0;
    if(!read(relocated(0x100b21400),devsim) || !slot(devsim,0x120,0x10015387a)) return false;
    const uint64_t developer=original<uint64_t(*)(uint64_t)>(0x10015387a)(devsim);
    if(!object(developer,0x100a75d70,8) || !slot(developer,0x38,0x100143ebc)) return false;
    logf_("FIXTURE DEVELOPER object=%llx ConstructDefaultLot=100143ebc",U(developer));
    logf_("FIXTURE DESCRIPTOR manager=%llx descriptor=%llx config=04000004 control=%llx coverage=immediate_globals_slots_only transitive_fault_boundary=original",U(manager),U(descriptor),U(control));
    if(observed_descriptor) *observed_descriptor=descriptor;
    return true;
}
static bool services(void* opaque,const State& s,uint64_t control) {
    return services_impl(opaque,s,control,nullptr);
}
static bool property_domain(uint32_t type) {
    return type==6 || type==7 || type==0xa || type==0x800a;
}
static bool log_property_domain(uint64_t holder,uint32_t id,uint64_t p,uint64_t v,
                                uint32_t type,uint32_t count) {
    const bool supported=property_domain(type);
    logf_("FIXTURE PROPERTY_DOMAIN holder=%llx id=%08x property=%llx variant=%llx type=%x count=%u supported=%u",
        U(holder),id,U(p),U(v),type,count,unsigned(supported));
    return supported;
}
// Original GetProperty @ 0x10004ae40, GetID @ 0x10004c63c,
// GetVariant @ 0x10004c650, GetType @ 0x100051c00, GetCount @ 0x10004c682,
// array payload @ 0x10004c804. Observation copies the same successful reads.
static bool property(uint64_t holder,uint32_t id,RawPropertyObservation* out=nullptr) {
    if(out) { *out={}; out->id=id; }
    const uint64_t p=original<uint64_t(*)(uint64_t,uint32_t)>(0x10004ae40)(holder,id);
    if(!p) { if(out) { out->status=PropertyStatus::Missing; out->complete=true; } logf_("FIXTURE PROPERTY holder=%llx id=%08x present=0",U(holder),id); return true; }
    if(!object(p,0x100a69a40,0x24) || !slot(p,0x18,0x10004c63c) || !slot(p,0x28,0x10004c650)) return false;
    const uint32_t embedded=original<uint32_t(*)(uint64_t)>(0x10004c63c)(p);
    const uint64_t v=original<uint64_t(*)(uint64_t)>(0x10004c650)(p);
    if(embedded!=id || v!=p+8 || !object(v,0x100a69ab0,0x18) ||
       !slot(v,0x18,0x100051c00) || !slot(v,0x20,0x10004c682)) return false;
    const uint32_t type=original<uint32_t(*)(uint64_t)>(0x100051c00)(v);
    const uint32_t count=original<uint32_t(*)(uint64_t)>(0x10004c682)(v);
    if(out) { out->type=type; out->count=count; }
    if(!log_property_domain(holder,id,p,v,type,count)) {
        if(out) { out->status=PropertyStatus::Unsupported; out->complete=true; }
        return false;
    }
    uint32_t words[256]={}; unsigned n=1; uint64_t data=v+8;
    if(type==0x800a) {
        if(count>256 || !slot(v,0x3a8,0x10004c804)) return false;
        n=count; data=original<uint64_t(*)(uint64_t)>(0x10004c804)(v);
    } else if(type!=6 && type!=7 && type!=0xa) return false;
    if(n && !read(data,words,n*4)) return false;
    logf_("FIXTURE PROPERTY holder=%llx id=%08x present=1 property=%llx variant=%llx type=%x count=%u words=%u",U(holder),id,U(p),U(v),type,count,n);
    for(unsigned i=0;i<n;++i) logf_("FIXTURE VALUE id=%08x index=%u bits=%08x",id,i,words[i]);
    if(out) copy_property_words(*out,words,n);
    return true;
}
static bool grid(uint64_t manager) {
    // GetLot @ 0x100131192: +48/+4c signed dimensions, +98 columns,
    // each column pointer at x*24, then z*8. Bound every possible read.
    const uint64_t base=manager-8; int32_t w=0,h=0; uint64_t cols=0;
    if(!object(base,0x100a758e0,0xa0) || !object(manager,0x100a75a28,8) ||
       !slot(manager,0x70,0x100131184) || !slot(base,0x80,0x100131192) ||
       !read(base+0x48,w) || !read(base+0x4c,h) || w<=0 || h<=0 || w>1024 || h>1024 ||
       !read(base+0x98,cols) || !span(cols,size_t(w)*24)) return false;
    for(int32_t x=0;x<w;++x) { uint64_t row=0;
        uint64_t end=0,capacity=0;
        if(!read(cols+uint64_t(x)*24,row) || !read(cols+uint64_t(x)*24+8,end) ||
           !read(cols+uint64_t(x)*24+16,capacity) || end<row || capacity<end ||
           end-row<uint64_t(h)*8 || capacity-row>8*1024 || !span(row,size_t(h)*8)) return false; }
    logf_("FIXTURE GRID manager=%llx cols=%llx w=%d h=%d",U(manager),U(cols),w,h);
    return true;
}
// Existing typed raw getter domain only; no conversions or bool/curve ABI.
// Aging inputs: monthly @ 0x100097f33 / 0x100097f77; Age @
// 0x100099db7 / 0x100099dc8 / 0x100099dd9 (raw asm in this packet).
static constexpr uint32_t aging_ids[]={0x28ecf510u,0xea1cf220u,0xea1cf221u,0xea1cf222u};
static bool aging_fields(FixtureKind kind) {
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    if(kind==FixtureKind::DiagnosticPlaced) return true;
#endif
    return kind==FixtureKind::SavedInactive;
}
// Existing original getter addresses below supply raw values; no new getters.
static bool occupant(uint64_t p,uint64_t manager,bool agingFields=false,OccupantObservation* out=nullptr) {
    if(out) { *out={}; out->occupant=p; }
    if(p<8 || !object(p-8,0x100a7b900,0x84) || !object(p,0x100a7ba08,0x7c) ||
       !slot(p,0x30,0x10019bb80) || !slot(p,0x40,0x10019b46c) || !slot(p,0x50,0x10019b558)) return false;
    const uint64_t holder=original<uint64_t(*)(uint64_t)>(0x10019bb80)(p);
    uint64_t exemplar=0;
    if(holder!=p+0x10 || !object(holder,0x100a7bb90,0x38) || !slot(holder,0x28,0x10004ae40) ||
       !read(holder+0x30,exemplar) || !object(exemplar,0x100a95bc0,0x24) || !slot(exemplar,0x58,0x100305dc6)) return false;
    uint32_t stored_position[6]={};
    if(!read(p-8+0x6c,stored_position)) return false;
    logf_("FIXTURE STORED_POSITION occupant=%llx raw6c=%08x,%08x,%08x,%08x,%08x,%08x",U(p),stored_position[0],stored_position[1],stored_position[2],stored_position[3],stored_position[4],stored_position[5]);
    uint32_t key[3]={}; float xyz[3]={},lo[3]={},hi[3]={}; uint32_t bits[9]={};
    if(!original<bool(*)(uint64_t,void*)>(0x100305dc6)(exemplar,key)) return false;
    original<void(*)(uint64_t,float*)>(0x10019b46c)(p,xyz);
    original<void(*)(uint64_t,float*,float*)>(0x10019b558)(p,lo,hi);
    std::memcpy(bits,xyz,12); std::memcpy(bits+3,lo,12); std::memcpy(bits+6,hi,12);
    logf_("FIXTURE OCCUPANT interface=%llx base=%llx holder=%llx exemplar=%llx tgi=%08x:%08x:%08x position=%08x,%08x,%08x bounds=%08x,%08x,%08x,%08x,%08x,%08x",U(p),U(p-8),U(holder),U(exemplar),key[0],key[1],key[2],bits[0],bits[1],bits[2],bits[3],bits[4],bits[5],bits[6],bits[7],bits[8]);
    if(out) {
        out->base=p-8; out->holder=holder; out->exemplar=exemplar;
        std::memcpy(out->tgi,key,sizeof key); std::memcpy(out->position_bounds,bits,sizeof bits);
        std::memcpy(out->stored_position,stored_position,sizeof stored_position);
    }
    // Deliberate original resolution; do not duplicate its float-to-cell code.
    const uint64_t lot=original<uint64_t(*)(uint64_t,uint64_t)>(0x100131184)(manager,p);
    if(!lot) { if(out) out->lot.complete=true; logf_("FIXTURE LOT occupant=%llx present=0",U(p)); }
    else {
        if(!object(lot,0x100a79fb0,0xd5) || !slot(lot,0x70,0x10017fa80) ||
           !slot(lot,0x48,0x10018399e) || !slot(lot,0x108,0x100183a68) || !slot(lot,0x118,0x100183a76)) return false;
        Rect r{}; uint64_t config=0; uint32_t raw[4]={};
        original<void(*)(uint64_t,Rect&)>(0x10017fa80)(lot,r);
        const uint32_t state=original<uint32_t(*)(uint64_t)>(0x100183a68)(lot);
        const uint32_t condition=original<uint32_t(*)(uint64_t)>(0x100183a76)(lot);
        if(!read(lot+0x60,raw)) return false;
        config=original<uint64_t(*)(uint64_t)>(0x10018399e)(lot);
        if(out) {
            out->lot.lot=lot; out->lot.descriptor=config; out->lot.rect=r;
            out->lot.state=state; out->lot.condition=condition;
            std::memcpy(out->lot.raw_bounds,raw,sizeof raw); out->lot.complete=true;
        }
        logf_("FIXTURE LOT occupant=%llx present=1 lot=%llx config=%llx rect=%d,%d,%d,%d state=%u condition=%u raw60=%08x,%08x,%08x,%08x",U(p),U(lot),U(config),r.l,r.t,r.r,r.b,state,condition,raw[0],raw[1],raw[2],raw[3]);
    }
    unsigned property_index=0;
    for(uint32_t id:{0xc8ebd131u,0xa8ebcf0cu,0x88ed1059u,0x27812852u,0xc910b4cfu,0x8a1e07eeu})
        if(!property(holder,id,out?&out->properties[property_index++]:nullptr)) return false;
    property_index=6;
    if(agingFields) for(uint32_t id:aging_ids)
        if(!property(holder,id,out?&out->properties[property_index++]:nullptr)) return false;
    if(out) out->complete=true;
    return true;
}
static bool inventory_impl(void* opaque,const State& s,const char* phase,
                           InventoryObservation* out,uint64_t descriptor=0) {
    if(out) { *out={}; out->pid=getpid(); out->initial=s; out->guarded_descriptor=descriptor; }
    if(!identity(opaque,s)) return false;
    if(out) observe_owner(*out,*static_cast<Owner*>(opaque));
    uint64_t pollution=0,manager=0;
    if(!read(s.city+0x2d8,pollution) || !object(pollution,0x100a6ce20,0x34c) ||
       !read(relocated(0x100b21340),manager) || !grid(manager)) return false;
    if(out) { out->pollution=pollution; out->lot_manager=manager; }
    unsigned list_index=0;
    logf_("FIXTURE INVENTORY_BEGIN phase=%s pollution=%llx scope=C0_D8_108_listed_occupants",phase,U(pollution));
    for(unsigned off:{0xc0u,0xd8u,0x108u}) {
        const uint64_t sentinel=pollution+off; uint64_t links[3]={};
        if(!read(sentinel,links) || links[2]>4096) return false;
        if(out) out->list_counts[list_index++]=links[2];
        logf_("FIXTURE LIST phase=%s offset=%x count=%llu head=%llx tail=%llx empty=%u",phase,off,U(links[2]),U(links[1]),U(links[0]),unsigned(!links[2]));
        uint64_t node=links[1],prev=sentinel; uint64_t seen[4096]={};
        for(uint64_t i=0;i<links[2];++i) {
            if(node==sentinel || (node&7)) return false;
            for(uint64_t j=0;j<i;++j) if(seen[j]==node) return false;
            seen[i]=node; uint64_t v[3]={};
            if(!read(node,v) || v[0]!=prev) return false;
            logf_("FIXTURE REF phase=%s list=%x index=%llu node=%llx prev=%llx next=%llx occupant=%llx",phase,off,U(i),U(node),U(v[0]),U(v[1]),U(v[2]));
            OccupantObservation* observed=nullptr;
            if(out) {
                out->occurrences.emplace_back(); auto& occurrence=out->occurrences.back();
                occurrence.offset=off; occurrence.index=i; occurrence.node=node;
                occurrence.prev=v[0]; occurrence.next=v[1]; observed=&occurrence.value;
            }
            if(!occupant(v[2],manager,aging_fields(static_cast<Owner*>(opaque)->kind),observed)) return false;
            prev=node; node=v[1];
        }
        uint64_t final[3]={};
        if(node!=sentinel || prev!=links[0] || !read(sentinel,final) || std::memcmp(final,links,sizeof links)) return false;
    }
    State end{};
    const bool ok=state(end) && same(s,end) && identity(opaque,end);
    if(out) finish_inventory_observation(*out,end,ok);
    logf_("FIXTURE INVENTORY_END phase=%s complete=%u",phase,unsigned(ok)); return ok;
}
// Exact three-argument dependency wrapper retained for ordinary inspect/attempt/query.
static bool inventory(void* opaque,const State& s,const char* phase) {
    return inventory_impl(opaque,s,phase,nullptr);
}
// Explicit host-only seam, intentionally unused by ordinary late_exec. Collecting
// true means readable inventory, even when placement_candidate_matches is false.
static bool inventory_observation(void* opaque,const State& s,const char* phase,InventoryObservation& out) {
    const State input=s; // permit host caller to reuse out.initial as its input
    out={};
    // Bound the optional collector's host binding before identity's legacy strcmp.
    if(!opaque || strnlen(static_cast<Owner*>(opaque)->path,sizeof(Owner::path))==sizeof(Owner::path)) return false;
    uint64_t descriptor=0;
    if(!services_impl(opaque,input,0,&descriptor)) return false;
    return inventory_impl(opaque,input,phase,&out,descriptor);
}
struct Job {
#ifdef OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION
    const bool checkpoint_mode;
    explicit Job(bool capture=false) : checkpoint_mode(capture) {}
#endif
    Context context{}; Owner candidate{}; bool attempt=false; char expected[1024]={};
    State late_expected{}, late_first{}, late_second{}; // identity values only, never retained native objects
    bool late_entered=false, late_ended=false, late_first_ok=false, late_second_ok=false;
};
#ifdef OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION
} // namespace fixture_placement_bindings
namespace placement_checkpoint {
static bool command_preflight(fixture_placement_bindings::Job&);
static bool transfer(std::unique_ptr<fixture_placement_bindings::Job>&);
}
namespace fixture_placement_bindings {
#endif
static std::unique_ptr<Owner> receipt; // main queue only; never exposed to parser
static std::atomic<bool> busy{false}; // held until process death after a saved click
static Job* late_context=nullptr; // process lifetime quarantine; never reset/deleted
struct Unlock {
    bool release=true;
    ~Unlock(){ if(release) busy.store(false,std::memory_order_release); }
};
enum class LateScope { None, One, Invalid };
static LateScope late_scope(bool attempt,unsigned clicks,FixtureKind kind) {
    if(!attempt || kind!=FixtureKind::SavedInactive || !clicks) return LateScope::None;
    return clicks==1?LateScope::One:LateScope::Invalid;
}
static bool late_owner(const Job& j) {
    // Compare the host pointer before dereferencing it; static receipt cannot reset
    // while busy is held. No old game pointer is used to make a native call.
    return j.context.owner && j.context.owner==receipt.get() && receipt->inspected &&
        receipt->spent && receipt->kind==FixtureKind::SavedInactive;
}
static bool late_ids(const State& expected,const State& fresh) {
    return expected.app==fresh.app && expected.city==fresh.city && expected.sim==fresh.sim;
}
static void late_state_log(const char* sample,const State& expected,const State& s,bool readable) {
    logf_("FIXTURE LATE_STATE sample=%s readable=%u app=%llx city=%llx sim=%llx date=%u pause=%d,%d,%d expected_date=%u expected_pause=%d,%d,%d date_delta=%lld pause_delta=%lld,%lld,%lld",
        sample,unsigned(readable),U(s.app),U(s.city),U(s.sim),s.date,s.pauses[0],s.pauses[1],s.pauses[2],
        expected.date,expected.pauses[0],expected.pauses[1],expected.pauses[2],
        static_cast<long long>(s.date)-expected.date,
        static_cast<long long>(s.pauses[0])-expected.pauses[0],
        static_cast<long long>(s.pauses[1])-expected.pauses[1],
        static_cast<long long>(s.pauses[2])-expected.pauses[2]);
}
static void late_end(Job& j,bool complete,const char* reason) {
    if(j.late_ended) return;
    j.late_ended=true; // exactly one END even if a C++ queue/callback exception occurs
    late_state_log("first",j.late_expected,j.late_first,j.late_first_ok);
    late_state_log("second",j.late_expected,j.late_second,j.late_second_ok);
    logf_("FIXTURE LATE_END click_entries=%u base_complete=%u later_inventory_complete=%u first_readable=%u second_readable=%u date=%u pause=%d,%d,%d reason=%s scope=C0_D8_108_listed_occupants queue_drained_claim=0 fixture_success=not_inferred immediate_stop_required=1",
        j.context.click_calls,unsigned(j.context.complete),unsigned(complete),
        unsigned(j.late_first_ok),unsigned(j.late_second_ok),j.late_second.date,
        j.late_second.pauses[0],j.late_second.pauses[1],j.late_second.pauses[2],reason);
}
static void late_exec(void* ptr) {
    Job* j=static_cast<Job*>(ptr);
    if(!j || j!=late_context || j->late_entered || j->late_ended) return;
    j->late_entered=true;
    logf_("FIXTURE LATE_BEGIN click_entries=%u base_complete=%u delay_ms=500 scope=C0_D8_108_listed_occupants queue_drained_claim=0 fixture_success=not_inferred immediate_stop_required=1",
        j->context.click_calls,unsigned(j->context.complete));
    bool complete=false; const char* reason="guard_or_inventory_incomplete";
    try {
        if(pthread_main_np() && late_owner(*j) && image_guard() && entries_guard()) {
            j->late_first_ok=state(j->late_first);
            if(j->late_first_ok && late_ids(j->late_expected,j->late_first) && identity(receipt.get(),j->late_first)) {
                // Renew all roots after the path checks; neither state is a retained reference.
                j->late_second_ok=state(j->late_second);
                if(j->late_second_ok && late_ids(j->late_expected,j->late_second) && identity(receipt.get(),j->late_second) &&
                   services(receipt.get(),j->late_second,0))
                    complete=inventory(receipt.get(),j->late_second,"LATER_AFTER_CLICK");
            }
        }
        if(complete) reason="inventory_readable_only";
    } catch(...) { reason="cpp_exception_quarantined"; }
    late_end(*j,complete,reason);
    // No cleanup, resume, retries or queue drain. Host Job/receipt/busy survive to death.
}
static bool late_transfer(std::unique_ptr<Job>& j,Unlock& unlock,LateScope scope) {
    if(scope==LateScope::None) return false;
    late_context=j.get(); // publish before dispatch; allocation already exists before mutation
    unlock.release=false;
    Job* held=j.release();
    logf_("FIXTURE COMMAND_END attempt=%u complete=%u late_pending=%u click_entries=%u",
        unsigned(held->attempt),unsigned(held->context.complete),unsigned(scope==LateScope::One),held->context.click_calls);
    if(scope==LateScope::Invalid) {
        late_end(*held,false,"invalid_click_scope_no_retry"); return true;
    }
    try {
        dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC),dispatch_get_main_queue(),held,late_exec);
    } catch(...) { late_end(*held,false,"queue_exception_quarantined"); }
    return true;
}
static void command_exec(void* ptr) {
    std::unique_ptr<Job> j(static_cast<Job*>(ptr));
    Unlock unlock;
    logf_("FIXTURE COMMAND_BEGIN attempt=%u",unsigned(j->attempt));
    try {
#ifdef OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION
        if(j->checkpoint_mode && !placement_checkpoint::command_preflight(*j)) {
            logf_("FIXTURE COMMAND_FAIL reason=checkpoint_preflight");
        } else
#endif
        if(!pthread_main_np()) { logf_("FIXTURE COMMAND_FAIL reason=not_main_thread"); }
        else {
            if(!j->attempt) receipt.reset();
            Owner* owner=j->attempt?receipt.get():&j->candidate;
            State s{}; char path[1024]={}; uint64_t regional=0; FixtureKind kind=FixtureKind::Unbound;
            bool ready=owner && image_guard() && entries_guard() && state(s) && loaded_path(s,path,regional,&kind);
            logf_("FIXTURE RECEIPT preflight=%u present=%u inspected=%u spent=%u points=%u",
                unsigned(ready),unsigned(owner!=nullptr),unsigned(owner && owner->inspected),
                unsigned(owner && owner->spent),owner?owner->count:0);
            if(j->attempt) ready=ready && owner->inspected && !owner->spent;
            if(ready && j->attempt) {
                uint64_t camera=0; unsigned char snapshot[camera_extent]={};
                ready=read(s.renderer+0xd8,camera) && camera==owner->camera &&
                    read(camera,snapshot,sizeof snapshot) && camera_equal(snapshot,owner->camera_bytes);
                if(!ready) {
                    logf_("FIXTURE RECEIPT_CAMERA mismatch=1 actual=%llx expected=%llx",U(camera),U(owner->camera));
                    if(camera==owner->camera && read(camera,snapshot,sizeof snapshot)) {
                        for(unsigned off=0;off<sizeof snapshot;off+=4) {
                            uint32_t a=0,b=0; std::memcpy(&a,snapshot+off,4); std::memcpy(&b,owner->camera_bytes+off,4);
                            if(a!=b) logf_("FIXTURE CAMERA_WORD offset=%x inspected=%08x current=%08x",off,b,a);
                        }
                    }
                }
            }
            if(ready && j->expected[0]) ready=!std::strcmp(path,j->expected);
            if(ready && j->attempt) {
                const bool matched=receipt_allows_point(*owner,j->context.target);
                ready=matched; // explicit coordinate must belong to this exact owned receipt
                if(!matched) logf_("FIXTURE RECEIPT_POINT mismatch=1 x=%d y=%d",j->context.target.x,j->context.target.y);
            }
            if(!ready) logf_("FIXTURE COMMAND_FAIL reason=identity_or_receipt_or_coordinate");
            else {
                Context& c=j->context; c.owner=owner; c.deps={identity,services,inventory};
                c.verified_city=s.city; std::strcpy(c.verified_path,path); std::strcpy(c.manifest_sha256,kind==FixtureKind::SavedInactive?saved_manifest_hash:manifest_hash);
                if(j->attempt) { c.attempt=true; c.inspection_complete=true; c.inspection_state=owner->receipt; owner->spent=true; }
                j->late_expected=s;
                fixture_placement::exec(&c);
                if(!j->attempt && c.complete && c.inspection_complete) {
                    // Commit city/address/path ownership only AFTER successful inspection.
                    owner->city=s.city; owner->regional=regional; std::strcpy(owner->path,path);
                    owner->receipt=c.inspection_state; retain_successful_points(c,*owner);
                    if(!read(s.renderer+0xd8,owner->camera) || !read(owner->camera,owner->camera_bytes,sizeof owner->camera_bytes) ||
                       !projection_clean(owner->camera_bytes)) {
                        logf_("FIXTURE COMMAND_FAIL reason=camera_receipt_read_or_dirty_projection");
                        c.complete=false; logf_("FIXTURE COMMAND_END attempt=0 complete=0"); return;
                    }
                    owner->inspected=true;
                    receipt.reset(new Owner(*owner));
                }
            }
        }
    } catch(...) { logf_("FIXTURE COMMAND_FAIL reason=cpp_exception"); }
#ifdef OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION
    if(j->checkpoint_mode) {
        unlock.release=false;
        placement_checkpoint::transfer(j); return;
    }
#endif
    const FixtureKind ownedKind=(j->context.owner && j->context.owner==receipt.get())?receipt->kind:FixtureKind::Unbound;
    if(late_transfer(j,unlock,late_scope(j->attempt,j->context.click_calls,ownedKind))) return;
    logf_("FIXTURE COMMAND_END attempt=%u complete=%u",unsigned(j->attempt),unsigned(j->context.complete));
}
// Returns queue acceptance only. Result is the main-queue BEGIN/FAIL/END log.
// Optional expected_path is a constraint, never proof of runtime identity.
static bool inspectfixture(const Point* points,unsigned count,const char* expected_path=nullptr) {
    if(!points || !count || count>32 || (expected_path && std::strlen(expected_path)>=1024)) return false;
    bool was=false; if(!busy.compare_exchange_strong(was,true)) return false;
    try {
        std::unique_ptr<Job> j(new Job); j->context.point_count=count;
        std::memcpy(j->context.points,points,count*sizeof(Point));
        if(expected_path) std::strcpy(j->expected,expected_path);
        dispatch_async_f(dispatch_get_main_queue(),j.get(),command_exec); j.release(); return true;
    } catch(...) { busy.store(false); return false; }
}
static bool attemptfixture(Point point) {
    bool was=false; if(!busy.compare_exchange_strong(was,true)) return false;
    try {
        std::unique_ptr<Job> j(new Job); j->attempt=true; j->context.target=point;
        dispatch_async_f(dispatch_get_main_queue(),j.get(),command_exec); j.release(); return true;
    } catch(...) { busy.store(false); return false; }
}
} // namespace fixture_placement_bindings
#endif
