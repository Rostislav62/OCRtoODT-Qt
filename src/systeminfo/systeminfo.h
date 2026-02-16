// ============================================================
//  SystemInfo — Cross-Platform Hardware Introspection Module
//  File: systeminfo.h
//
//  Responsibility:
//      Provide a minimal, pure-C API for querying basic hardware
//      information on Linux, Windows and macOS.
//
//      This module is designed to be:
//          - Independent from Qt and any GUI framework
//          - Usable from C and C++ (and easily from other languages)
//          - Suitable for building as a static or shared library
//            (.a / .so / .dll / .dylib)
//
//      Exposed information:
//          - CPU physical core count
//          - CPU logical thread count
//          - CPU brand string
//          - SIMD feature flags (AVX, AVX2, SSE4.1, NEON)
//          - Total RAM in MB
//          - Free/available RAM in MB
//          - Built-in text documentation (usage/help)
//
//      Notes:
//          - All functions are side-effect free and thread-safe
//            for typical use (they may read /proc or OS APIs).
//          - CPU feature detection is implemented for x86/x86_64;
//            on other architectures AVX/SSE flags will return 0.
//          - NEON detection is implemented via compile-time flags
//            for ARM; may be extended later if needed.
// ============================================================

#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------------
// CPU information
// ------------------------------------------------------------

/**
 * @brief Returns the number of physical CPU cores.
 *
 * On:
 *   - Linux:   parsed from /proc/cpuinfo (physical id + cpu cores)
 *   - macOS:   hw.physicalcpu
 *   - Windows: counted via GetLogicalProcessorInformationEx
 *
 * @return Number of physical cores, or -1 on error.
 */
int si_cpu_physical_cores(void);

/**
 * @brief Returns the number of logical CPU threads.
 *
 * On:
 *   - Linux:   sysconf(_SC_NPROCESSORS_ONLN)
 *   - macOS:   hw.logicalcpu
 *   - Windows: GetSystemInfo().dwNumberOfProcessors
 *
 * @return Number of logical threads, or -1 on error.
 */
int si_cpu_logical_threads(void);

/**
 * @brief Returns a pointer to a static CPU brand string.
 *
 * Example:
 *   "Intel(R) Core(TM) i9-14900KF" or similar.
 *
 * IMPORTANT:
 *   - Returned pointer is owned by the module and remains valid
 *     for the entire lifetime of the process.
 *   - Do NOT free() or modify this memory.
 *
 * @return const char* to a null-terminated UTF-8 string.
 *         Returns "Unknown CPU" on failure.
 */
const char* si_cpu_brand_string(void);


// ------------------------------------------------------------
// SIMD feature flags
// ------------------------------------------------------------

/**
 * @brief Returns non-zero if AVX is supported, zero otherwise.
 *
 * Implemented for x86/x86_64 via CPUID.
 * On non-x86 architectures returns 0.
 */
int si_has_avx(void);

/**
 * @brief Returns non-zero if AVX2 is supported, zero otherwise.
 *
 * Implemented for x86/x86_64 via CPUID (leaf 7, EBX bit 5).
 * On non-x86 architectures returns 0.
 */
int si_has_avx2(void);

/**
 * @brief Returns non-zero if SSE4.1 is supported, zero otherwise.
 *
 * Implemented for x86/x86_64 via CPUID (leaf 1, ECX bit 19).
 * On non-x86 architectures returns 0.
 */
int si_has_sse41(void);

/**
 * @brief Returns non-zero if NEON is supported, zero otherwise.
 *
 * Implemented via compile-time checks for ARM (e.g. __ARM_NEON).
 * On non-ARM architectures returns 0.
 */
int si_has_neon(void);


// ------------------------------------------------------------
// Memory information
// ------------------------------------------------------------

/**
 * @brief Returns total physical RAM in megabytes.
 *
 * On:
 *   - Linux:   sysinfo().totalram
 *   - macOS:   hw.memsize
 *   - Windows: GlobalMemoryStatusEx().ullTotalPhys
 *
 * @return Total RAM in MB, or -1 on error.
 */
long long si_total_ram_mb(void);

/**
 * @brief Returns free/available RAM in megabytes.
 *
 * On:
 *   - Linux:   sysinfo().freeram (approximate)
 *   - macOS:   host_statistics64() (approximate)
 *   - Windows: GlobalMemoryStatusEx().ullAvailPhys
 *
 * NOTE:
 *   The exact meaning of "free" vs "available" differs per OS.
 *   Value is intended for coarse decisions, not precise accounting.
 *
 * @return Free/available RAM in MB, or -1 on error.
 */
long long si_free_ram_mb(void);


// ------------------------------------------------------------
// Documentation / self-description
// ------------------------------------------------------------

/**
 * @brief Returns a built-in documentation string for this module.
 *
 * The returned text explains:
 *   - Purpose of SystemInfo
 *   - Platform coverage (Linux / Windows / macOS)
 *   - Exposed C API functions
 *   - Typical usage examples
 *
 * The string is static, null-terminated, and owned by the module.
 *
 * Example use from C/C++:
 *   printf("%s\n", si_documentation());
 *
 * Example from CLI tool:
 *   if (user_passed --help) print si_documentation().
 *
 * @return const char* to a null-terminated UTF-8 help text.
 */
const char* si_documentation(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SYSTEMINFO_H
