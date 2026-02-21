// ============================================================
//  OCRtoODT — Crash Handler
//  File: core/CrashHandler.cpp
//
//  Implements:
//      - Qt message redirection (low-level only)
//      - Fatal signal interception
//
//  Design constraints:
//      - MUST NOT call LogRouter (avoid recursion)
//      - MUST NOT allocate memory inside signal handler
//      - Must be async-signal-safe
// ============================================================

#include "CrashHandler.h"

#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <QtGlobal>
#include <QString>
#include <QByteArray>


// ------------------------------------------------------------
// Low-level Qt message handler
//
// IMPORTANT:
//   - No LogRouter here
//   - No Qt logging inside this function
// ------------------------------------------------------------
static void qtMessageHandler(QtMsgType type,
                             const QMessageLogContext &,
                             const QString &msg)
{
    const char* typeStr = "";

    switch (type)
    {
    case QtDebugMsg:    typeStr = "DEBUG"; break;
    case QtInfoMsg:     typeStr = "INFO"; break;
    case QtWarningMsg:  typeStr = "WARNING"; break;
    case QtCriticalMsg: typeStr = "CRITICAL"; break;
    case QtFatalMsg:    typeStr = "FATAL"; break;
    }

    fprintf(stderr, "[Qt %s] %s\n",
            typeStr,
            msg.toLocal8Bit().constData());

    fflush(stderr);

    if (type == QtFatalMsg)
        std::abort();
}

// ------------------------------------------------------------
// POSIX signal handler
//
// STRICT low-level only
// ------------------------------------------------------------
static void signalHandler(int sig)
{
    fprintf(stderr, "Fatal signal received: %d\n", sig);
    fflush(stderr);

    std::_Exit(EXIT_FAILURE);
}

// ------------------------------------------------------------
// Install crash handlers
// ------------------------------------------------------------
void CrashHandler::install()
{
    qInstallMessageHandler(qtMessageHandler);

    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE,  signalHandler);
    std::signal(SIGILL,  signalHandler);
}
