// ============================================================
//  SystemInfo — Cross-Platform Hardware Introspection Module
//  File: /src/systeminfo/systeminfo.cpp
//
//  See systeminfo.h for the high-level description.
//
//  Implementation notes:
//      - This file uses #ifdef blocks to select the correct
//        OS-specific implementation (Windows / Linux / macOS).
//      - The public API is a pure C interface (extern "C"),
//        but the implementation is written in C++17 for
//        convenience (std::string, etc.).
//
//      - CPU feature detection (AVX, AVX2, SSE4.1) is implemented
//        only for x86/x86_64 via CPUID. On other architectures
//        those functions safely return 0.
//
//      - NEON detection is based on compile-time defines and can
//        be extended later if runtime detection is needed.
//
//      - Memory reporting is intentionally coarse-grained and
//        meant for resource decisions, not precise accounting.
// ============================================================

#include "systeminfo.h"

#include <string>
#include <cstring>

// ------------------------------------------------------------
// Platform headers
// ------------------------------------------------------------
#if defined(_WIN32)

#define NOMINMAX
#include <windows.h>
#include <intrin.h>

#elif defined(__linux__)

#include <unistd.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>

#elif defined(__APPLE__)

#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_host.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#else
// Other / unknown platform — functions will fall back
// to conservative defaults.
#endif


// ============================================================
//  Internal helpers (anonymous namespace)
// ============================================================
namespace {

// --------------------------------------------------------
// Built-in documentation text
// --------------------------------------------------------
const char* kSystemInfoDoc = R"SYSINFO(
SystemInfo — Cross-Platform Hardware Introspection Module
=========================================================

Purpose:
--------
SystemInfo provides a minimal, pure-C API for querying basic
hardware characteristics of the current machine:

  * CPU physical core count
  * CPU logical thread count
  * CPU brand string
  * SIMD feature flags (AVX, AVX2, SSE4.1, NEON)
  * Total RAM and free/available RAM in megabytes

It is designed to be:
  * independent from any UI framework (no Qt required),
  * easy to compile as a static or shared library,
  * consumable from C, C++, and other languages via FFI.

Supported platforms:
--------------------
  * Linux
  * Windows
  * macOS

On unsupported platforms, the functions fall back to
conservative defaults (e.g. -1 or "Unknown CPU").

Public C API (see systeminfo.h for details):
--------------------------------------------
  int si_cpu_physical_cores(void);
  int si_cpu_logical_threads(void);
  const char* si_cpu_brand_string(void);

  int si_has_avx(void);
  int si_has_avx2(void);
  int si_has_sse41(void);
  int si_has_neon(void);

  long long si_total_ram_mb(void);
  long long si_free_ram_mb(void);

  const char* si_documentation(void);

Typical usage (C/C++):
----------------------
  #include "systeminfo.h"

  int main(void)
  {
      printf("CPU: %s\n", si_cpu_brand_string());
      printf("Physical cores: %d\n", si_cpu_physical_cores());
      printf("Logical threads: %d\n", si_cpu_logical_threads());
      printf("Total RAM: %lld MB\n", si_total_ram_mb());
      printf("Free  RAM: %lld MB\n", si_free_ram_mb());

      if (si_has_avx2())
          printf("AVX2 is supported.\n");

      return 0;
  }

CLI helper (optional):
----------------------
You can build a small CLI tool that links against this module
and prints diagnostics or this documentation:

  int main(int argc, char** argv)
  {
      if (argc > 1 && strcmp(argv[1], "--help") == 0) {
          printf("%s\n", si_documentation());
          return 0;
      }
      // ... otherwise print system info ...
  }

This module is intentionally small and focused, so it can be
embedded into different projects or shipped as a standalone
library and reused across multiple applications.

)SYSINFO";

// --------------------------------------------------------
// Cached CPU brand string
// --------------------------------------------------------
std::string& cpuBrandCache()
{
    static std::string brand = "Unknown CPU";
    return brand;
}

void initCpuBrandOnce()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

#if defined(_WIN32)

    int cpuInfo[4] = {0};
    char brand[0x40] = {0};

    __cpuid(cpuInfo, 0x80000000);
    unsigned int maxExtId = cpuInfo[0];

    if (maxExtId >= 0x80000004) {
        __cpuid((int*)cpuInfo, 0x80000002);
        std::memcpy(brand, cpuInfo, sizeof(cpuInfo));

        __cpuid((int*)cpuInfo, 0x80000003);
        std::memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));

        __cpuid((int*)cpuInfo, 0x80000004);
        std::memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));

        cpuBrandCache() = brand;
    }

#elif defined(__APPLE__)

    char buffer[256];
    size_t bufferLen = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.brand_string", buffer, &bufferLen, nullptr, 0) == 0) {
        cpuBrandCache() = buffer;
    }

#elif defined(__linux__)

    std::ifstream f("/proc/cpuinfo");
    if (!f.is_open())
        return;

    std::string line;
    while (std::getline(f, line)) {
        const std::string key = "model name";
        auto pos = line.find(key);
        if (pos != std::string::npos) {
            auto colon = line.find(':', pos + key.size());
            if (colon != std::string::npos) {
                std::string value = line.substr(colon + 1);
                // trim leading spaces
                size_t start = value.find_first_not_of(" \t");
                if (start != std::string::npos)
                    value = value.substr(start);
                cpuBrandCache() = value;
                break;
            }
        }
    }

#else
    // Unknown platform -> keep default string.
#endif
}

// --------------------------------------------------------
// x86 CPUID helpers (for AVX/SSE flags)
// --------------------------------------------------------
bool isX86()
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    return true;
#else
    return false;
#endif
}

void cpuidWrapper(unsigned int leaf, unsigned int subleaf,
                  unsigned int& eax, unsigned int& ebx,
                  unsigned int& ecx, unsigned int& edx)
{
    eax = ebx = ecx = edx = 0;

#if defined(_WIN32) && (defined(_M_X64) || defined(_M_IX86))

    int regs[4] = {0};
    __cpuidex(regs, static_cast<int>(leaf), static_cast<int>(subleaf));
    eax = static_cast<unsigned int>(regs[0]);
    ebx = static_cast<unsigned int>(regs[1]);
    ecx = static_cast<unsigned int>(regs[2]);
    edx = static_cast<unsigned int>(regs[3]);

#elif (defined(__x86_64__) || defined(__i386__)) && defined(__GNUC__)

    unsigned int a, b, c, d;
    __asm__ volatile ("cpuid"
                     : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                     : "a"(leaf), "c"(subleaf));
    eax = a;
    ebx = b;
    ecx = c;
    edx = d;

#else
    // Non-x86 or unsupported compiler -> leave zeros
    (void)leaf;
    (void)subleaf;
#endif
}

bool hasAVX_Impl()
{
    if (!isX86())
        return false;

    unsigned int eax, ebx, ecx, edx;
    cpuidWrapper(1u, 0u, eax, ebx, ecx, edx);

    // ECX bit 28 = AVX
    return (ecx & (1u << 28)) != 0u;
}

bool hasAVX2_Impl()
{
    if (!isX86())
        return false;

    unsigned int eax, ebx, ecx, edx;
    cpuidWrapper(7u, 0u, eax, ebx, ecx, edx);

    // EBX bit 5 = AVX2
    return (ebx & (1u << 5)) != 0u;
}

bool hasSSE41_Impl()
{
    if (!isX86())
        return false;

    unsigned int eax, ebx, ecx, edx;
    cpuidWrapper(1u, 0u, eax, ebx, ecx, edx);

    // ECX bit 19 = SSE4.1
    return (ecx & (1u << 19)) != 0u;
}

bool hasNEON_Impl()
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    return true;
#else
    return false;
#endif
}

// --------------------------------------------------------
// Helpers for core counts
// --------------------------------------------------------
int linuxLogicalThreads()
{
#if defined(__linux__)
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? static_cast<int>(n) : -1;
#else
    return -1;
#endif
}

int linuxPhysicalCores()
{
#if defined(__linux__)
    // Heuristic: parse /proc/cpuinfo for "physical id" + "cpu cores"
    std::ifstream f("/proc/cpuinfo");
    if (!f.is_open())
        return -1;

    std::string line;
    std::string currentPhysicalId;
    std::string lastPhysicalId;
    int coresPerSocket = 0;
    int sockets = 0;

    while (std::getline(f, line)) {
        if (line.find("physical id") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                currentPhysicalId = line.substr(pos + 1);
                size_t start = currentPhysicalId.find_first_not_of(" \t");
                if (start != std::string::npos)
                    currentPhysicalId = currentPhysicalId.substr(start);

                if (currentPhysicalId != lastPhysicalId) {
                    sockets++;
                    lastPhysicalId = currentPhysicalId;
                }
            }
        } else if (line.find("cpu cores") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string value = line.substr(pos + 1);
                size_t start = value.find_first_not_of(" \t");
                if (start != std::string::npos)
                    value = value.substr(start);
                int v = std::stoi(value);
                if (v > coresPerSocket)
                    coresPerSocket = v;
            }
        }
    }

    if (sockets == 0 || coresPerSocket == 0)
        return linuxLogicalThreads(); // fallback

    return sockets * coresPerSocket;
#else
    return -1;
#endif
}

int macosLogicalThreads()
{
#if defined(__APPLE__)
    int value = 0;
    size_t size = sizeof(value);
    if (sysctlbyname("hw.logicalcpu", &value, &size, nullptr, 0) == 0)
        return value;
#endif
    return -1;
}

int macosPhysicalCores()
{
#if defined(__APPLE__)
    int value = 0;
    size_t size = sizeof(value);
    if (sysctlbyname("hw.physicalcpu", &value, &size, nullptr, 0) == 0)
        return value;
#endif
    return -1;
}

int windowsLogicalThreads()
{
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return static_cast<int>(info.dwNumberOfProcessors);
#else
    return -1;
#endif
}

int windowsPhysicalCores()
{
#if defined(_WIN32)
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &len);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return windowsLogicalThreads();

    std::unique_ptr<uint8_t[]> buffer(new uint8_t[len]);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr =
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.get());

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, ptr, &len))
        return windowsLogicalThreads();

    int cores = 0;
    uint8_t* p = buffer.get();
    uint8_t* end = buffer.get() + len;

    while (p < end) {
        auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(p);
        if (info->Relationship == RelationProcessorCore)
            cores++;
        p += info->Size;
    }

    return (cores > 0) ? cores : windowsLogicalThreads();
#else
    return -1;
#endif
}

// --------------------------------------------------------
// Helpers for RAM
// --------------------------------------------------------
long long linuxTotalRamMB()
{
#if defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return -1;
    long long totalBytes =
        static_cast<long long>(info.totalram) * info.mem_unit;
    return totalBytes / (1024LL * 1024LL);
#else
    return -1;
#endif
}

long long linuxFreeRamMB()
{
#if defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return -1;
    long long freeBytes =
        static_cast<long long>(info.freeram) * info.mem_unit;
    return freeBytes / (1024LL * 1024LL);
#else
    return -1;
#endif
}

long long macosTotalRamMB()
{
#if defined(__APPLE__)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memsize = 0;
    size_t len = sizeof(memsize);
    if (sysctl(mib, 2, &memsize, &len, nullptr, 0) != 0)
        return -1;
    return static_cast<long long>(memsize / (1024ULL * 1024ULL));
#else
    return -1;
#endif
}

long long macosFreeRamMB()
{
#if defined(__APPLE__)
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmstat),
                          &count) != KERN_SUCCESS)
        return -1;

    // Approximation: free + inactive + speculative
    uint64_t pageSize = 0;
    size_t size = sizeof(pageSize);
    if (sysctlbyname("hw.pagesize", &pageSize, &size, nullptr, 0) != 0)
        pageSize = 4096;

    uint64_t freePages =
        static_cast<uint64_t>(vmstat.free_count) +
        static_cast<uint64_t>(vmstat.inactive_count) +
        static_cast<uint64_t>(vmstat.speculative_count);

    uint64_t freeBytes = freePages * pageSize;
    return static_cast<long long>(freeBytes / (1024ULL * 1024ULL));
#else
    return -1;
#endif
}

long long windowsTotalRamMB()
{
#if defined(_WIN32)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (!GlobalMemoryStatusEx(&statex))
        return -1;
    return static_cast<long long>(statex.ullTotalPhys / (1024ULL * 1024ULL));
#else
    return -1;
#endif
}

long long windowsFreeRamMB()
{
#if defined(_WIN32)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (!GlobalMemoryStatusEx(&statex))
        return -1;
    return static_cast<long long>(statex.ullAvailPhys / (1024ULL * 1024ULL));
#else
    return -1;
#endif
}

} // anonymous namespace


// ============================================================
//  Public C API implementation
// ============================================================

extern "C" {

int si_cpu_physical_cores(void)
{
#if defined(_WIN32)
    return windowsPhysicalCores();
#elif defined(__linux__)
    return linuxPhysicalCores();
#elif defined(__APPLE__)
    return macosPhysicalCores();
#else
    return -1;
#endif
}

int si_cpu_logical_threads(void)
{
#if defined(_WIN32)
    return windowsLogicalThreads();
#elif defined(__linux__)
    return linuxLogicalThreads();
#elif defined(__APPLE__)
    return macosLogicalThreads();
#else
    return -1;
#endif
}

const char* si_cpu_brand_string(void)
{
    initCpuBrandOnce();
    return cpuBrandCache().c_str();
}

int si_has_avx(void)
{
    return hasAVX_Impl() ? 1 : 0;
}

int si_has_avx2(void)
{
    return hasAVX2_Impl() ? 1 : 0;
}

int si_has_sse41(void)
{
    return hasSSE41_Impl() ? 1 : 0;
}

int si_has_neon(void)
{
    return hasNEON_Impl() ? 1 : 0;
}

long long si_total_ram_mb(void)
{
#if defined(_WIN32)
    return windowsTotalRamMB();
#elif defined(__linux__)
    return linuxTotalRamMB();
#elif defined(__APPLE__)
    return macosTotalRamMB();
#else
    return -1;
#endif
}

long long si_free_ram_mb(void)
{
#if defined(_WIN32)
    return windowsFreeRamMB();
#elif defined(__linux__)
    return linuxFreeRamMB();
#elif defined(__APPLE__)
    return macosFreeRamMB();
#else
    return -1;
#endif
}

const char* si_documentation(void)
{
    return kSystemInfoDoc;
}

} // extern "C"
