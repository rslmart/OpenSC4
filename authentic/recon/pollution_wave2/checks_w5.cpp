// W5 curated check design (from the handoff; fixture helpers to be built at
// merge: a MockMessage seam, a captured PostMessageSeam list, a mock
// ISimManagerSeam, a curve with known output, and a record fixture setter).
// Every quirk gets a discriminator; the merge session adapts to the suite's
// Check() idiom.

// DoMessage band edges: 0x99ef1142 -> added=true; 0x99ef1143 -> added=false @ 0x100096a7b
Check(msgDispatched(0x99ef1142, kOccupantAdded), "band member 1 = added @ 0x100096b00");
Check(msgDispatched(0x99ef1143, kOccupantAddedFalse), "band member 2 = removed @ 0x100096b00");
Check(msgDispatched(0x99ef1141, kNoop) && msgDispatched(0x99ef1144, kNoop),
      "band boundaries excluded by +0x6610eebe/cmp 2/jb @ 0x100096a81");
Check(msgDispatched(0x26d31ec0, kNoop), "jg boundary: 0x26d31ec0 not dispatched @ 0x100096a79");
Check(msgDispatched(0x26d31ec1, kVisual) && msgDispatched(0x66956810, kSimBegin)
      && msgDispatched(0x66956814, kDaily) && msgDispatched(0x66956816, kMonthly)
      && msgDispatched(0xca1b151c, kTuning) && msgDispatched(0x66956813, kNoop),
      "dispatch table @ 0x100096a58-0x100096ae8");
Check(msgDispatched(0x12345678, kNoop) && lastReturn == true,
      "unknown id: no-op but returns true @ 0x100096b4c");
Check(gzStringToggle(1u, false) == kVisualSetTrue && gzStringToggle(1u, true) == kNoop
      && gzStringToggle(0u, true) == kVisualSetFalse && gzStringToggle(2u, true) == kVisualSetFalse,
      "field32C set/clear only on change; value!=1 (incl. 2) means false @ 0x100096b25-0x100096b3c");
// SimulationBegin / EndOfCycle curve: exact op order @ 0x100098056-0x10009806f
Check(Bits(field348) == Bits(CurveEval(123456.0f * 100.0f) * 0x3c23d70a_bits),
      "op order pop*100 -> curve -> *0.01 @ 0x100098056");
Check(rec.SimulationBeginPop(0.0f) == 0.0f, "population zero crossing @ 0x100098052");
// EndOfCycle division edge geometry @ 0x100099709-0x100099779
Check(eocWith(tractCount=0), "tractCount==0 -> all four averages 0 (je @ 0x100099723)");
Check(avgAir(5u, 2u) == 2u && avgWater(5u, 2u) == 2u && garbageScaled(5u, 2u) == 250u,
      "u32 div truncation @ 0x100099735/0x10009973d/0x100099753");
Check(field234_after(0x10000u, 1u) == 0u && historyValue(0xea110134) == 0u,
      "field234 stores low 16 bits; history sees zero-extended field234 not the quotient @ 0x100099771/0x1000997b9");
Check(field244_after(0u) == false && field244_after(1u) == true,
      "field244 = (field248 != 0) via setne @ 0x100099719");
Check(historyLast(0xea110138) == avgAir + garbageScaled, "sum computed after field20C store @ 0x1000997e4");
Check(fieldsZeroedAfterEoc(field22C|field230|field21C|field220|field214|field218),
      "qword zeros at +0x22c/+0x21c/+0x214 (field230 zeroed too) @ 0x1000997f9-0x100099809");
Check(historyStillCalledWhenTractCount0, "history posts happen even on the zero path @ 0x1000997e1");
Check(mirrorCopy(0x11223344, 0x55667778) into (field224, field228), "8-byte copy +0x224<-+0x21c @ 0x100099779");
// Flag posts @ 0x100099810-0x100099890
Check(flagPosts({328:0xda6dd35d, 329:0xfa6dd0bb, 32a:0x5a6dd1aa, 32b:0xda6dd2cb}) in order,
      "conditional post order/ids @ 0x10009981a-0x10009988b");
Check(allFlagsClearedAfterEoc && postCount(0xea418d8fu) == 1 && itsArg2 == 8u,
      "dword clear +0x328 wipes all 4 flags; final post (0xea418d8f, 8) always @ 0x100099890/0x1000998ff");
