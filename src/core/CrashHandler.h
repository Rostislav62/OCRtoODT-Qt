// ============================================================
//  OCRtoODT — Crash Handler
//  File: core/CrashHandler.h
//
//  Responsibility:
//      - Install global Qt message handler
//      - Install POSIX signal handlers (SIGSEGV, SIGABRT, etc.)
//      - Redirect fatal runtime errors into LogRouter
//
//  Policy:
//      - Must be installed AFTER LogRouter::configure()
//      - Must be installed AFTER QApplication construction
//      - Must NOT allocate heavy resources inside signal handler
// ============================================================

#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

class CrashHandler
{
public:
    // --------------------------------------------------------
    // Install global crash handlers
    //
    // NOTE:
    //   Must be called AFTER:
    //     - QApplication app(argc, argv);
    //     - LogRouter::configure(...)
    // --------------------------------------------------------
    static void install();
};

#endif // CRASHHANDLER_H
