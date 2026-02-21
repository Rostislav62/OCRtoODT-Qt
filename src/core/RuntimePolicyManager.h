// ============================================================
//  OCRtoODT — RuntimePolicyManager
//  File: src/core/RuntimePolicyManager.h
//
//  Responsibility:
//      Central runtime policy re-application.
//
//      • Reads user intent from ConfigManager (general.*)
//      • Computes effective runtime policy (threads + memory mode)
//      • Publishes effective values into ConfigManager (general.*)
//      • Applies ThreadPoolGuard
//
//  Safety:
//      • Safe to call multiple times when OCR is NOT active.
//      • In low-memory conditions, may force disk_only to avoid OOM.
// ============================================================

#pragma once

class RuntimePolicyManager
{
public:
    // Called once at startup.
    // Stores CPU logical thread count snapshot.
    static void initialize(int cpuLogical);

    // Immediate reapply (caller guarantees OCR is NOT running)
    static void reapply();

    // Safe public entry point:
    // If OCR running -> defer
    // If idle        -> apply immediately
    static void requestReapply(bool ocrIsRunning);

    // Called when OCR pipeline becomes idle
    static void onPipelineBecameIdle();
};
