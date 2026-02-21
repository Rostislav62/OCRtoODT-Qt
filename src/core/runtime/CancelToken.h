// ============================================================
//  OCRtoODT — Cancel Token
//  File: core/runtime/CancelToken.h
//
//  Responsibility:
//      - Thread-safe cooperative cancellation mechanism
//      - Used by OCR pipeline and preprocessing
// ============================================================

#ifndef CANCELTOKEN_H
#define CANCELTOKEN_H

#include <atomic>

class CancelToken
{
public:
    void requestCancel()
    {
        m_cancelled.store(true, std::memory_order_relaxed);
    }

    bool isCancelled() const
    {
        return m_cancelled.load(std::memory_order_relaxed);
    }

    void reset()
    {
        m_cancelled.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic_bool m_cancelled{false};
};

#endif // CANCELTOKEN_H
