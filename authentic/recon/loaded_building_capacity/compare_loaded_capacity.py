#!/usr/bin/env python3
"""Bounded acceptance check for this ticket's captured Rush Hour observation."""
import hashlib
import json
from pathlib import Path
import re
import struct
from collections import Counter

ROOT = Path(__file__).resolve().parent
IDS = {"08fd1613", "48ebd0b1", "27812852", "00000001"}
PROPS = IDS | {"c910b4cf", "8a1e07ee"}

def require(ok, why):
    if not ok:
        raise ValueError(why)

def unique(rows, key=0):
    result = {}
    for row in rows:
        require(row[key] not in result, "duplicate record")
        result[row[key]] = row
    return result

def verify(live, native, fixture):
    # Parse every complete line before extracting values: regex searching alone
    # silently ignored malformed extra records and contradictory lookup status.
    live_lines = []
    for line in live.splitlines():
        m = re.fullmatch(r"\[\d{2}:\d{2}:\d{2}\] LOADED_CAPACITY (.+)", line)
        require(m is not None, "malformed live line")
        live_lines.append(m[1])
    word = r"[0-9a-f]{8}"
    ptr = r"[1-9a-f][0-9a-f]*"
    vector = rf"{word},{word},{word}"
    geometry = rf"position={vector} min={vector} max={vector}"
    patterns = {
        "BEGIN": r"version=1 expected_properties=6 expected_capacities=4 expected_curve_words=16 thread=main_required",
        "STATE": rf"app={ptr} state=2 city={ptr} pollution={ptr} simdate=unavailable pause=unavailable",
        "FIXTURE": rf"node={ptr} occupant={ptr} holder={ptr} exemplar={ptr} tgi=6534284a:c8dbccba:1f470000 pointer_identity=runtime_only",
        "GEOMETRY": geometry,
        "IDENTITY": r"status=matched basis=F0_single_node_TGI_raw_bounds saved_runtime_pointer_identity=not_claimed",
        "PROPERTY": rf"id={word} lookup=(?:missing|present embedded={word} property={ptr} variant={ptr} type=[0-9a-f]{{4}} count=\d+)",
        "SCALAR": rf"id={word} raw={word}",
        "ARRAY": rf"id=8a1e07ee data={ptr} count=16 tool_max=256",
        "CURVE": rf"index=\d+ raw={word}",
        "POWER": rf"manager={ptr} lot={ptr} flagsCD=[0-9a-f]{{2}} hasPower=[01] needsPower=[01] power={ptr} isPowered=[01] scale_raw={word} lot_config=unavailable lot_bounds=unavailable",
        "CAPACITY": rf"id={word} invocation=deliberate_original sentinel=a55a5aa5 returned=[01] output={word} decimal=\d+",
        "END": r"status=CAPTURE_COMPLETE complete=1 properties=6/6 capacities=4/4 curve_words=16/16 parity=not_tested natural_dispatch=not_tested aging=not_tested",
    }
    def parse_lines(lines, grammar, inventory):
        seen = Counter()
        for line in lines:
            kind, sep, body = line.partition(" ")
            require(sep and kind in grammar, "unknown capture record")
            require(re.fullmatch(grammar[kind], body) is not None, "malformed " + kind)
            seen[kind] += 1
        require(seen == Counter(inventory), "record inventory")
    inventory = dict.fromkeys(patterns, 1)
    inventory.update(PROPERTY=6, SCALAR=4, CURVE=16, CAPACITY=4)
    parse_lines(live_lines, patterns, inventory)
    native_lines = []
    for line in native.splitlines():
        require(line.startswith("NATIVE "), "malformed native line")
        native_lines.append(line[7:])
    parse_lines(native_lines, {
        "BEGIN": "cases=4",
        "FIXTURE": "ref=a9bd882d:138a8240 tgi=6534284a:c8dbccba:1f470000",
        "GEOMETRY": geometry,
        "POWER": rf"cell=38,115 lot=c9bd5d4a:13a49798 flagsCD=[0-9a-f]{{2}} isPowered=[01] scale_raw={word}",
        "CAPACITY": rf"id={word} returned=[01] output={word} decimal=\d+",
        "END": "complete=1 cases=4",
    }, dict(BEGIN=1,FIXTURE=1,GEOMETRY=1,POWER=1,CAPACITY=4,END=1))
    require(len([s for s in live_lines if s.startswith("BEGIN ")]) == 1, "BEGIN count")
    require(live_lines[0] == "BEGIN version=1 expected_properties=6 expected_capacities=4 expected_curve_words=16 thread=main_required", "BEGIN")
    require(live_lines[-1] == "END status=CAPTURE_COMPLETE complete=1 properties=6/6 capacities=4/4 curve_words=16/16 parity=not_tested natural_dispatch=not_tested aging=not_tested", "END")
    require(sum(s.startswith("END ") for s in live_lines) == 1, "END count")
    require(not any("FAIL" in s for s in live_lines), "capture FAIL")
    require("NATIVE BEGIN cases=4" == native.splitlines()[0], "native BEGIN")
    require("NATIVE END complete=1 cases=4" == native.splitlines()[-1], "native END")
    require(native.count("NATIVE BEGIN ") == 1 and native.count("NATIVE END ") == 1, "native envelope")
    require(sum(s.startswith("PROPERTY ") for s in live_lines) == 6, "property count")
    props=unique(re.findall(r"PROPERTY id=([0-9a-f]{8}) lookup=(present|missing)([^\n]*)", live))
    require(set(props) == PROPS, "property IDs")
    require(props["00000001"][1:] == ("missing", ""), "missing property")
    require(all(props[key][1] == "present" for key in PROPS - {"00000001"}), "required property missing")
    scalar=unique(re.findall(r"SCALAR id=([0-9a-f]{8}) raw=([0-9a-f]{8})", live))
    require(set(scalar) == {"08fd1613","48ebd0b1","27812852","c910b4cf"}, "scalar inventory")
    saved=unique(re.findall(r"saved holder=([0-9a-f]{8}) property=([0-9a-f]{8}) tag=([0-9a-f]{8}) dbtype=([0-9a-f]{8}) count=(\d+) bytes=([0-9a-f]+)", fixture))
    defaults=unique(re.findall(r"property id=([0-9a-f]{8})[^\n]*record_bytes=([0-9a-f]+)", fixture))
    for key, (_, bits) in scalar.items():
        if key in saved:
            row=saved[key]
            require(row[2]=="00000000" and row[4]=="0", "saved scalar domain")
            code=int(row[3],16); raw=bytes.fromhex(row[5])
        else:
            raw=bytes.fromhex(defaults[key][1])
            pid, flag, code=struct.unpack("<IBI",raw[:9])
            require(pid==int(key,16) and flag==0, "default field sequence")
            raw=raw[9:]
        require(len(raw)==4 and int.from_bytes(raw,"little")==int(bits,16), "scalar raw mismatch")
        tag={3:6,7:7,9:10}[code]
        require(re.fullmatch(rf" embedded={key} property=[0-9a-f]+ variant=[0-9a-f]+ type={tag:04x} count=0",props[key][2]) is not None, "scalar runtime tag")
    raw=bytes.fromhex(defaults["8a1e07ee"][1])
    pid, flag, code, count=struct.unpack("<IBII", raw[:13])
    require((pid,flag,code,count)==(0x8a1e07ee,0,0x8009,16) and len(raw)==77, "curve fixture domain")
    require(re.fullmatch(r" embedded=8a1e07ee property=[0-9a-f]+ variant=[0-9a-f]+ type=800a count=16",props["8a1e07ee"][2]) is not None, "curve runtime tag/count")
    curves=unique(re.findall(r"CURVE index=(\d+) raw=([0-9a-f]{8})",live))
    require(set(curves)=={str(i) for i in range(16)}, "curve inventory")
    require([int(curves[str(i)][1],16) for i in range(16)] == list(struct.unpack("<16I",raw[13:])), "curve raw mismatch")
    for label, pattern in [
        ("TGI",r"tgi=([0-9a-f:]+)"),
        ("geometry",r"GEOMETRY (position=[^\n]+)"),
    ]:
        a=re.findall(pattern,live); b=re.findall(pattern,native)
        require(len(a)==len(b)==1 and a==b,label+" mismatch")
    require("IDENTITY status=matched basis=F0_single_node_TGI_raw_bounds saved_runtime_pointer_identity=not_claimed" in live, "identity")
    for pattern in (r"flagsCD=([0-9a-f]{2})",r"isPowered=(\d)",r"scale_raw=([0-9a-f]{8})"):
        a=re.findall(pattern,live); b=re.findall(pattern,native)
        require(len(a)==len(b)==1 and a==b,"power/scale mismatch")
    flags = int(re.search(r"flagsCD=([0-9a-f]{2})",live)[1],16)
    has_power = int(re.search(r"hasPower=([01])",live)[1])
    needs_power = int(re.search(r"needsPower=([01])",live)[1])
    powered = int(re.search(r"isPowered=([01])",live)[1])
    require(has_power == bool(flags & 0x10) and needs_power == bool(flags & 8)
            and powered == (has_power or not needs_power), "contradictory power")
    live_cases=unique(re.findall(r"CAPACITY id=([0-9a-f]{8}) invocation=deliberate_original sentinel=a55a5aa5 returned=([01]) output=([0-9a-f]{8}) decimal=(\d+)",live))
    native_cases=unique(re.findall(r"NATIVE CAPACITY id=([0-9a-f]{8}) returned=([01]) output=([0-9a-f]{8}) decimal=(\d+)",native))
    require(set(live_cases)==set(native_cases)==IDS, "capacity inventory")
    require(sum(s.startswith("CAPACITY ") for s in live_lines)==4, "capacity line count")
    require(sum(s.startswith("NATIVE CAPACITY ") for s in native.splitlines())==4, "native capacity line count")
    for key in IDS:
        require(live_cases[key]==native_cases[key], "capacity mismatch "+key)
        require(int(live_cases[key][2],16)==int(live_cases[key][3]), "decimal mismatch")
    return {"cases":4,"mismatches":0,"raw_scalars":4,"raw_curve_words":16,
            "geometry_power_scale":"match","native_results":native_cases,
            "scope":"one loaded observation, deliberate calls; no natural dispatch, aging or Phase B"}

def main():
    paths=[ROOT/"live_loaded_capacity_output.txt",ROOT/"capacity_native_output.txt",ROOT/"fixture_capacity_output.txt"]
    live,native,fixture=(p.read_text() for p in paths)
    result=verify(live,native,fixture)
    mutations=[
        (live[:live.rfind("[02:51:54] LOADED_CAPACITY END")],native,fixture),
        (live.replace("CURVE index=15 raw=00000000","CURVE index=14 raw=00000000"),native,fixture),
        (live.replace("CURVE index=0 raw=00000000","CURVE index=0 raw=80000000"),native,fixture),
        (live.replace("SCALAR id=08fd1613 raw=00001388","SCALAR id=08fd1613 raw=00001389"),native,fixture),
        (live.replace("PROPERTY id=00000001 lookup=missing","PROPERTY id=00000002 lookup=missing"),native,fixture),
        (live,native.replace("returned=1 output=00001387","returned=1 output=00001388"),fixture),
        (live,native.replace("NATIVE END complete=1 cases=4",""),fixture),
        (live.replace("decimal=5000","decimal=5001"),native,fixture),
        (live.replace("lookup=present","lookup=missing"),native,fixture),
        (live.replace("[02:51:54] LOADED_CAPACITY END", "[02:51:54] LOADED_CAPACITY SCALAR id=08fd1613 raw=INVALID\n[02:51:54] LOADED_CAPACITY END"),native,fixture),
        ("\n".join(s for s in live.splitlines() if " STATE " not in s and " ARRAY " not in s),native,fixture),
        (live.replace("hasPower=1","hasPower=0"),native,fixture),
        (live.replace("CURVE index=0 raw=00000000","CURVE index=0 raw=00000000 trailing=ignored"),native,fixture),
        (live,native.replace("NATIVE END", "NATIVE SCALAR id=08fd1613 raw=INVALID\nNATIVE END"),fixture),
    ]
    # Every required line is mandatory, including STATE/ARRAY and the native
    # envelope. A duplicate or malformed instance cannot disappear in a search.
    for i, line in enumerate(live.splitlines()):
        lines = live.splitlines()
        mutations.append(("\n".join(lines[:i]+lines[i+1:]),native,fixture))
        mutations.append(("\n".join(lines[:i]+[line]+lines[i:]),native,fixture))
        mutations.append(("\n".join(lines[:i]+[line+" MALFORMED"]+lines[i+1:]),native,fixture))
    for i, line in enumerate(native.splitlines()):
        lines = native.splitlines()
        mutations.append((live,"\n".join(lines[:i]+lines[i+1:]),fixture))
        mutations.append((live,"\n".join(lines[:i]+[line]+lines[i:]),fixture))
        mutations.append((live,"\n".join(lines[:i]+[line+" MALFORMED"]+lines[i+1:]),fixture))
    for mutation in mutations:
        try:
            verify(*mutation)
        except (ValueError, KeyError, IndexError):
            continue
        raise ValueError("negative capture check unexpectedly accepted")
    result["negative_capture_checks"]=len(mutations)
    result["sha256"]={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    print(json.dumps(result,indent=2))
if __name__=="__main__":
    main()
