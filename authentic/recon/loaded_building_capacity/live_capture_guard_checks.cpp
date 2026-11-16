#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
static uint64_t g_slide=0;
static uint64_t get_app() { return 0; }
static void logf_(const char*, ...) __attribute__((format(printf,1,2)));
static void logf_(const char*, ...) {}
#include "live_capture_fragment.inc"
int main() {
    const auto page=static_cast<size_t>(sysconf(_SC_PAGESIZE));
    auto* p=static_cast<unsigned char*>(mmap(nullptr,page*2,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANON,-1,0));
    if (p==MAP_FAILED) return 2;
    if (mprotect(p+page,page,PROT_NONE)) return 3;
    const uint64_t vt=0x100a69a40;
    auto* shortObject=p+page-0x20;
    std::memcpy(shortObject,&vt,sizeof vt);
    const bool oldGuard=loaded_capacity_capture_detail::object(
        reinterpret_cast<uint64_t>(shortObject),vt,0x20);
    const bool rejects= !loaded_capacity_capture_detail::property_object(
        reinterpret_cast<uint64_t>(shortObject));
    auto* completeObject=p+page-0x24;
    std::memcpy(completeObject,&vt,sizeof vt);
    const bool accepts=loaded_capacity_capture_detail::property_object(
        reinterpret_cast<uint64_t>(completeObject));
    munmap(p,page*2);
    std::printf("property extent: old_guard_accepts=%u new_rejects=%u complete_accepts=%u\n",
        oldGuard,rejects,accepts);
    return oldGuard && rejects && accepts ? 0 : 1;
}
