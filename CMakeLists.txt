# ============================================================
#  OCRtoODT — CMake Project
#  File: CMakeLists.txt
#
#  Responsibility:
#      - Configure full build environment for OCRtoODT
#      - Enable modern C++17
#      - Enable Qt6 modules (Core/Gui/Widgets/Concurrent/Multimedia)
#      - Enable Poppler-Qt6 (PDF rendering) via pkg-config
#      - Enable OpenCV (preprocessing filters)
#      - Enable Tesseract OCR (SYSTEM default) via pkg-config
#      - Build a single executable target (no duplicates)
#
#  Policy:
#      - SYSTEM Tesseract is the default and the only supported mode.
#      - No global include_directories()/link_directories().
#      - All includes/libs are attached to the OCRtoODT target.
# ============================================================

cmake_minimum_required(VERSION 3.16)

project(OCRtoODT VERSION 1.0.0 LANGUAGES CXX)

# ------------------------------------------------------------
# Application version (available in C++ as APP_VERSION)
# ------------------------------------------------------------
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

# ------------------------------------------------------------
# Compiler configuration
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# Qt build helpers (UI / MOC / RCC)
# ------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/settings
)

# ============================================================
# Qt6 modules
# ============================================================
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
    Multimedia
    LinguistTools
    Svg
)

# ============================================================
# PkgConfig
# ============================================================
find_package(PkgConfig REQUIRED)

# ============================================================
# Poppler-Qt6 (PDF rendering)
#   Ubuntu/Debian: sudo apt install libpoppler-qt6-dev
# ============================================================
pkg_check_modules(POPPLERQT6 REQUIRED IMPORTED_TARGET poppler-qt6)

# ============================================================
# OpenCV (preprocessing filters)
#   Ubuntu/Debian: sudo apt install libopencv-dev
# ============================================================
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")

# ============================================================
# Tesseract OCR (SYSTEM default)
#   Ubuntu/Debian: sudo apt install libtesseract-dev libleptonica-dev
#
# We use pkg-config to get correct transitive dependencies.
# ============================================================
pkg_check_modules(TESSERACT REQUIRED IMPORTED_TARGET tesseract)
message(STATUS "Using SYSTEM Tesseract via pkg-config")
message(STATUS "Tesseract include: ${TESSERACT_INCLUDE_DIRS}")
message(STATUS "Tesseract libs   : ${TESSERACT_LIBRARIES}")

# ============================================================
# Project sources
#
# Notes:
#   - Keep lists explicit and single-source-of-truth.
#   - Do NOT duplicate files across multiple variables.
#   - Do NOT create OCRtoODT target more than once.
# ============================================================

# ------------------------------------------------------------
# Core subsystem
# ------------------------------------------------------------
set(CORE_SOURCES
    src/core/ConfigManager.cpp
    src/core/ThemeManager.cpp
    src/core/LanguageManager.cpp
    src/core/LogRouter.cpp
    src/core/PerformanceProfiler.cpp
    src/core/ResourceManager.cpp
    src/core/ProgressManager.cpp
    src/core/ocrPdfCache.cpp
    src/core/pdfDocumentService.cpp
    src/core/providers/baseProvider.cpp
    src/core/services/PopplerService.cpp

    src/core/processors/InputProcessor.cpp
    src/core/processors/RecognitionProcessor.cpp
    src/core/processors/ExportProcessor.cpp

    src/core/layout/OdtLayoutModel.cpp
)

set(CORE_HEADERS
    src/core/ConfigManager.h
    src/core/ThemeManager.h
    src/core/LanguageManager.h
    src/core/LogRouter.h
    src/core/PerformanceProfiler.h
    src/core/ResourceManager.h
    src/core/ProgressManager.h
    src/core/ocrPdfCache.h
    src/core/pdfDocumentService.h
    src/core/providers/baseProvider.h
    src/core/services/PopplerService.h
    src/core/VirtualPage.h

    src/core/processors/InputProcessor.h
    src/core/processors/RecognitionProcessor.h
    src/core/processors/ExportProcessor.h

    src/core/layout/OdtLayoutModel.h
)

# ------------------------------------------------------------
# Input subsystem (STEP 0)
# ------------------------------------------------------------
set(INPUT_SOURCES
    src/0_input/InputController.cpp
    src/0_input/PreviewController.cpp
    src/0_input/PdfPageProvider.cpp
    src/0_input/ClearController.cpp
    src/0_input/ImageThumbnailProvider.cpp
)

set(INPUT_HEADERS
    src/0_input/InputController.h
    src/0_input/PreviewController.h
    src/0_input/PdfPageProvider.h
    src/0_input/ClearController.h
    src/0_input/ImageThumbnailProvider.h
)

# ------------------------------------------------------------
# Preprocess subsystem (STEP 1)
# ------------------------------------------------------------
set(PREPROCESS_SOURCES
    src/1_preprocess/ImageLoader.cpp
    src/1_preprocess/EnhanceProcessor.cpp
    src/1_preprocess/Preprocess_Pipeline.cpp
    src/1_preprocess/ImageAnalyzer.cpp
    src/1_preprocess/StrategySelector.cpp

    src/1_preprocess/filters/adaptive_threshold.cpp
    src/1_preprocess/filters/background_norm.cpp
    src/1_preprocess/filters/clahe.cpp
    src/1_preprocess/filters/gaussian.cpp
    src/1_preprocess/filters/sauvola.cpp
    src/1_preprocess/filters/shadow_removal.cpp
    src/1_preprocess/filters/sharpen.cpp
)

set(PREPROCESS_HEADERS
    src/1_preprocess/ImageLoader.h
    src/1_preprocess/EnhanceProcessor.h
    src/1_preprocess/Preprocess_Pipeline.h
    src/1_preprocess/PageJob.h
    src/1_preprocess/ImageAnalyzer.h
    src/1_preprocess/StrategySelector.h

    src/1_preprocess/filters/adaptive_threshold.h
    src/1_preprocess/filters/background_norm.h
    src/1_preprocess/filters/clahe.h
    src/1_preprocess/filters/gaussian.h
    src/1_preprocess/filters/sauvola.h
    src/1_preprocess/filters/shadow_removal.h
    src/1_preprocess/filters/sharpen.h
)

# ------------------------------------------------------------
# OCR subsystem (STEP 2)
# ------------------------------------------------------------
set(OCR_SOURCES
    src/2_ocr/OcrPipeLineController.cpp
    src/2_ocr/OcrPipeLineWorker.cpp
    src/2_ocr/OcrPageWorker.cpp
    src/2_ocr/OcrTsvQuality.cpp
    src/2_ocr/OcrMultipassSelector.cpp
)

set(OCR_HEADERS
    src/2_ocr/OcrPipeLineController.h
    src/2_ocr/OcrPipeLineWorker.h
    src/2_ocr/OcrPageWorker.h
    src/2_ocr/OcrResult.h
    src/2_ocr/OcrPassConfig.h
    src/2_ocr/OcrTsvQuality.h
    src/2_ocr/OcrMultipassSelector.h
)

# ------------------------------------------------------------
# Structuring subsystem (STEP 3)
# ------------------------------------------------------------
set(STRUCT_SOURCES
    src/3_LineTextBuilder/LineTextBuilder.cpp
    src/3_LineTextBuilder/LineTableSerializer.cpp
)

set(STRUCT_HEADERS
    src/3_LineTextBuilder/LineRow.h
    src/3_LineTextBuilder/LineTable.h
    src/3_LineTextBuilder/LineTextBuilder.h
    src/3_LineTextBuilder/LineTableSerializer.h
)

# ------------------------------------------------------------
# Edit lines subsystem (STEP 4)
# ------------------------------------------------------------
set(EDIT_SOURCES
    src/4_edit_lines/EditLinesController.cpp
    src/4_edit_lines/LineHitTest.cpp
    src/4_edit_lines/LineTableModel.cpp
    src/4_edit_lines/LineTextDelegate.cpp
)

set(EDIT_HEADERS
    src/4_edit_lines/EditLinesController.h
    src/4_edit_lines/EditLinesControllerEvents.h
    src/4_edit_lines/LineHitTest.h
    src/4_edit_lines/LineTableModel.h
    src/4_edit_lines/LineTextDelegate.h
)

# ------------------------------------------------------------
# Document model (STEP 5 doc model)
# ------------------------------------------------------------
set(DOC_SOURCES
    src/5_document/DocumentBuilder.cpp
    src/5_document/DocumentDebugWriter.cpp
)

set(DOC_HEADERS
    src/5_document/DocumentModel.h
    src/5_document/DocumentBuilder.h
    src/5_document/DocumentDebugWriter.h
)

# ------------------------------------------------------------
# Export (STEP 5 export)
# ------------------------------------------------------------
set(EXPORT_SOURCES
    src/5_export/ExportController.cpp
    src/5_export/ExportTextNormalizer.cpp

    src/5_export/txt_export/TxtExporter.cpp
    src/5_export/odt_export/OdtExporter.cpp
    src/5_export/docx_export/DocxExporter.cpp
)

set(EXPORT_HEADERS
    src/5_export/ExportController.h
    src/5_export/ExportTextNormalizer.h

    src/5_export/txt_export/TxtExporter.h
    src/5_export/odt_export/OdtExporter.h
    src/5_export/docx_export/DocxExporter.h
)

# ------------------------------------------------------------
# Dialogs / UI
# ------------------------------------------------------------
set(DIALOG_SOURCES
    dialogs/aboutdialog.cpp
    dialogs/helpdialog.cpp
    dialogs/settingsdialog.cpp
    dialogs/searchhighlighter.cpp
    dialogs/pageframe.cpp
    dialogs/export.cpp
    dialogs/OcrCompletionDialog.cpp
)

set(DIALOG_HEADERS
    dialogs/aboutdialog.h
    dialogs/helpdialog.h
    dialogs/settingsdialog.h
    dialogs/searchhighlighter.h
    dialogs/pageframe.h
    dialogs/export.h
    dialogs/OcrCompletionDialog.h
)

set(UI_FILES
    ui/mainwindow.ui
    ui/aboutdialog.ui
    ui/helpdialog.ui
    ui/settingsdialog.ui
    ui/exportdialog.ui
    ui/OcrCompletionDialog.ui
)

# ------------------------------------------------------------
# Settings panes
# ------------------------------------------------------------
set(SETTINGS_SOURCES
    settings/generalpane.cpp
    settings/preprocesspane.cpp
    settings/recognitionpane.cpp
    settings/odtpane.cpp
    settings/interfacepane.cpp
    settings/interfacepane_preview.cpp
    settings/interfacepane_samples.cpp
    settings/interfacepane_qss.cpp
    settings/interfacepane_language.cpp
    settings/loggingpane.cpp
)

set(SETTINGS_HEADERS
    settings/generalpane.h
    settings/preprocesspane.h
    settings/recognitionpane.h
    settings/odtpane.h
    settings/interfacepane.h
    settings/loggingpane.h
)

set(SETTINGS_UI
    settings/generalpane.ui
    settings/preprocesspane.ui
    settings/recognitionpane.ui
    settings/odtpane.ui
    settings/interfacepane.ui
    settings/loggingpane.ui
)

# ------------------------------------------------------------
# System info module
# ------------------------------------------------------------
set(SYSTEMINFO_SOURCES
    src/systeminfo/systeminfo.cpp
)

set(SYSTEMINFO_HEADERS
    src/systeminfo/systeminfo.h
)

# ------------------------------------------------------------
# Main window / entry point
# ------------------------------------------------------------
set(APP_SOURCES
    src/main.cpp
    src/mainwindow.cpp
)

set(APP_HEADERS
    src/mainwindow.h
)

# ------------------------------------------------------------
# Qt Resource files
# ------------------------------------------------------------
set(RESOURCE_FILES
    resources/icons.qrc
    resources/sounds.qrc
    resources/docs.qrc
    resources/themes.qrc
    resources/sample.qrc
)

# ============================================================
# Copy config.yaml to binary directory
# ============================================================
configure_file(
    ${CMAKE_SOURCE_DIR}/config.yaml
    ${CMAKE_BINARY_DIR}/config.yaml
    COPYONLY
)

# ============================================================
# Build executable (single target)
# ============================================================
add_executable(OCRtoODT
    ${APP_SOURCES}
    ${APP_HEADERS}

    ${CORE_SOURCES}
    ${CORE_HEADERS}

    ${INPUT_SOURCES}
    ${INPUT_HEADERS}

    ${PREPROCESS_SOURCES}
    ${PREPROCESS_HEADERS}

    ${OCR_SOURCES}
    ${OCR_HEADERS}

    ${STRUCT_SOURCES}
    ${STRUCT_HEADERS}

    ${EDIT_SOURCES}
    ${EDIT_HEADERS}

    ${DOC_SOURCES}
    ${DOC_HEADERS}

    ${EXPORT_SOURCES}
    ${EXPORT_HEADERS}

    ${DIALOG_SOURCES}
    ${DIALOG_HEADERS}

    ${SETTINGS_SOURCES}
    ${SETTINGS_HEADERS}
    ${SETTINGS_UI}

    ${SYSTEMINFO_SOURCES}
    ${SYSTEMINFO_HEADERS}

    ${UI_FILES}
    ${RESOURCE_FILES}
    docs/README.md
    resources/styles/general_cards.qss
    docs/config_schema.md
    src/core/CrashHandler.h
    src/core/CrashHandler.cpp
    src/core/runtime/CancelToken.h
    src/core/ThreadPoolGuard.h
    src/core/ThreadPoolGuard.cpp
    src/core/RuntimePolicyManager.h
    src/core/RuntimePolicyManager.cpp
)

# ============================================================
# Include directories (project headers only)
# ============================================================
target_include_directories(OCRtoODT PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/dialogs
    ${CMAKE_SOURCE_DIR}/settings
    ${CMAKE_SOURCE_DIR}/systeminfo
)

# ============================================================
# Link libraries
#   - Qt6 imported targets
#   - Poppler and Tesseract via PkgConfig imported targets
#   - OpenCV via standard find_package variables
# ============================================================
target_link_libraries(OCRtoODT PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::Svg

    PkgConfig::POPPLERQT6
    PkgConfig::TESSERACT

    ${OpenCV_LIBS}
)

# ============================================================
# Translations (.ts -> .qm)
# ============================================================
qt_add_translations(OCRtoODT
    TS_FILES
        resources/translations/ocrtoodt_ru.ts
        resources/translations/ocrtoodt_en.ts
        resources/translations/ocrtoodt_ro.ts
    RESOURCE_PREFIX "/translations"
)


# ============================================================
# Diagnostics
# ============================================================
message(STATUS "------------------------------------------------------------")
message(STATUS "Build target       : OCRtoODT")
message(STATUS "Project version    : ${PROJECT_VERSION}")
message(STATUS "Qt6 found          : YES")
message(STATUS "Poppler-Qt6 found  : YES (pkg-config)")
message(STATUS "OpenCV found       : YES")
message(STATUS "Tesseract          : SYSTEM (pkg-config)")
message(STATUS "------------------------------------------------------------")


# ============================================================
# Install rules (required for AppImage)
# ============================================================

install(TARGETS OCRtoODT
    RUNTIME DESTINATION bin
)

install(FILES config.yaml
    DESTINATION bin
)

install(FILES resources/icons/ocrtoodt.png
    DESTINATION share/icons/hicolor/256x256/apps
)

install(FILES OCRtoODT.desktop
    DESTINATION share/applications
)
