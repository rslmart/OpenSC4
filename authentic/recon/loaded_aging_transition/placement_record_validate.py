"""Scoped standalone diagnostic build/run; never writes assets or rebuilds core."""
import hashlib,json,pathlib,subprocess,datetime,platform,shlex
root=pathlib.Path(__file__).resolve().parents[3]
p=pathlib.Path(__file__).resolve().parent
identity={'startedUTC':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commands':[], 'coreRebuilt':False,'coreSanitizerInstrumented':False}
def sha(f):
    with open(f,'rb') as h:return hashlib.file_digest(h,'sha256').hexdigest()
def run(args,log,expected=0):
    r=subprocess.run(args,cwd=root,capture_output=True,text=True)
    (p/log).write_text(r.stdout+r.stderr)
    identity['commands'].append({'command':shlex.join(map(str,args)),'exit':r.returncode,'expected':expected,'log':log})
    if r.returncode!=expected:raise RuntimeError(log+' unexpected exit '+str(r.returncode))
    return r.stdout
manifest=p/'PLACEMENT_CHECKPOINT_EXPORT.json'
m=json.loads(manifest.read_text());region=pathlib.Path(m['checkpoint'])
def region_snapshot():
    files={str(f.relative_to(region)):sha(f) for f in region.rglob('*') if f.is_file()}
    if files!=m['files'] or len(files)!=93:raise RuntimeError('93-file export manifest mismatch')
    return files
identity['manifestSHA256']=sha(manifest)
identity['selectedBeforeSHA256']=sha(region/m['selectedCity'])
identity['beforeFiles']=region_snapshot()
game=pathlib.Path.home()/'Library/Application Support/Steam/steamapps/common/SimCity 4 Deluxe/Sim City 4 Deluxe Edition.app/Contents/MacOS/Sim City 4 Deluxe Editionsub'
protected=[manifest,game,root/'build/authentic/libopensc4_authentic_core.a',p/'native_pollution_snapshot.h',p/'native_tutorial_snapshot.h',p/'native_tutorial_probe.cpp',root/'authentic/CMakeLists.txt',root/'authentic/src/city.cpp',root/'authentic/tools/oracle/oracle_dylib.cpp']
identity['protectedBefore']={str(f):sha(f) for f in protected}
identity['gitHead']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
identity['host']=platform.machine()
identity['compiler']=subprocess.check_output(['xcrun','clang++','--version'],text=True)
sdk='/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk'
identity['SDKPath']=sdk
identity['SDKSettingsSHA256']=sha(pathlib.Path(sdk)/'SDKSettings.json')
flags=['xcrun','clang++','-std=c++20','-O2','-arch','arm64','-isysroot',sdk,'-ffp-model=strict','-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations','-fsanitize=address,undefined','-Iauthentic/include']
try:
    sources=[p/'placement_record_snapshot.h',p/'placement_record_checks.cpp',p/'placement_record_probe.cpp']
    run(['python3','/Users/russellmartin/DecompBench/tools/static_guardrail.py','--mode','opensc4','--files',*map(str,sources)],'placement_record_guardrail.log')
    for kind in ['checks','probe']:
        run(flags+[str(p/f'placement_record_{kind}.cpp'),'build/authentic/libopensc4_authentic_core.a','-Wl,-dead_strip','-framework','Cocoa','-framework','OpenGL','-o',str(p/f'placement_record_{kind}.bin')],f'placement_record_{kind}_build.log')
        run(['xcrun','vtool','-show-build',str(p/f'placement_record_{kind}.bin')],f'placement_record_{kind}_macho.log')
    run([str(p/'placement_record_checks.bin')],'placement_record_checks.log')
    city=region/m['selectedCity'];digest=m['selectedCitySHA256']
    for n in [1,2]:run([str(p/'placement_record_probe.bin'),str(city),digest],f'placement_record_run{n}.log')
    identity['deterministicOutput']=(p/'placement_record_run1.log').read_bytes()==(p/'placement_record_run2.log').read_bytes()
    assert identity['deterministicOutput']
    run([str(p/'placement_record_probe.bin'),str(city),'0'*64],'placement_record_hash_rejection.log',1)
    run([str(p/'placement_record_probe.bin'),str(city),'A'*64],'placement_record_hex_rejection.log',1)
    run(['python3',str(p/'placement_record_source_checks.py')],'placement_record_source_checks.log')
    run(['git','diff','--check'],'placement_record_diffcheck.log')
finally:
    identity['afterFiles']=region_snapshot()
    identity['selectedAfterSHA256']=sha(region/m['selectedCity'])
    identity['protectedAfter']={str(f):sha(f) for f in protected}
    identity['protectedUnchanged']=identity['protectedBefore']==identity['protectedAfter']
    identity['hashes']={str(f):sha(f) for f in p.glob('placement_record_*') if f.is_file() and f.name!='placement_record_identity.json'}
    identity['finishedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat()
    (p/'placement_record_identity.json').write_text(json.dumps(identity,indent=2)+'\n')
print('93 manifest hashes before/after verified; selected hash pinned; protected inputs unchanged:',identity['protectedUnchanged'])
