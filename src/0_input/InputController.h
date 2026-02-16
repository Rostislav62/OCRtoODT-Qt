// ============================================================
//  OCRtoODT — Input Controller
//  File: src/0_input/InputController.h
//
//  Responsibility:
//      STEP 0_input
//
//      * Expand selected files into logical pages
//      * Create placeholder list model for UI
//      * Load first page synchronously for preview
//      * Load remaining pages asynchronously
//      * Generate thumbnails via ImageThumbnailProvider
//      * Emit page activation events for downstream UI sync
//
// ============================================================

#ifndef INPUTCONTROLLER_H
#define INPUTCONTROLLER_H

#include <QObject>
#include <QVector>
#include <QFutureWatcher>
#include <QModelIndex>

class QWidget;
class QStandardItemModel;
class QStandardItem;
class QImage;
class QPixmap;

namespace Core {
struct VirtualPage;
}

namespace Input {

class ImageThumbnailProvider;

class InputController : public QObject
{
    Q_OBJECT

public:
    explicit InputController(QObject *parent = nullptr);
    ~InputController() override;

    void openFiles(QWidget *parentWidget);
    void reset();

    // --------------------------------------------------------
    // UI → Input: page activation from thumbnails / list
    // --------------------------------------------------------
    void handleItemActivated(const QModelIndex &index);

    // --------------------------------------------------------
    // Access to prepared pages (STEP 0 output)
    // --------------------------------------------------------
    const QVector<Core::VirtualPage>& pages() const { return m_pages; }

signals:
    // --------------------------------------------------------
    // UI wiring signals
    // --------------------------------------------------------
    void filesLoaded(QStandardItemModel *model);

    // Preview image ready (image + page identity)
    void previewReady(const Core::VirtualPage &vp, const QImage &img);

    // 🔑 CRITICAL:
    // Emitted whenever user activates/selects a page.
    // Used to synchronize STEP 4 Text Tab with selected page.
    void pageActivated(int globalIndex);

private slots:
    void onThumbnailReady(const QString &key, const QPixmap &pix);
    void onJobResultReady(int index);
    void onJobsFinished();

private:
    // --------------------------------------------------------
    // Internal structs
    // --------------------------------------------------------
    struct PageWorkItem
    {
        QString path;
        bool    isPdf = false;
        int     pdfPageIndex = -1;
        int     sequenceIndex = -1;
    };

    struct PageResult
    {
        bool    ok = false;
        int     sequenceIndex = -1;

        QString finalPath;
        QString displayName;

        int     imgWidth = 0;
        int     imgHeight = 0;
        QString imgFormat;
    };

    // --------------------------------------------------------
    // Helpers
    // --------------------------------------------------------
    void initializePlaceholders(int totalPages);
    PageResult processSingleItem(const PageWorkItem &it);
    void finalizePage(const PageResult &res);

private:
    QVector<Core::VirtualPage> m_pages;
    QStandardItemModel        *m_model = nullptr;

    ImageThumbnailProvider   *m_thumbProvider = nullptr;

    QFutureWatcher<PageResult> m_watcher;
};

} // namespace Input

#endif // INPUTCONTROLLER_H
