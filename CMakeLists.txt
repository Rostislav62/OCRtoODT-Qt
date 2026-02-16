# ============================================================
#  OCRtoODT — CMake Project
#  File: CMakeLists.txt
#
#  Responsibility:
#      - Configure full build environment for OCRtoODT
#      - Enable modern C++17
#      - Enable Qt6 modules (including Concurrent)
#      - Enable Poppler-Qt6 (PDF rendering)
#      - Enable OpenCV (preprocessing filters)
#      - Enable embedded Tesseract (local libraries + headers)
#      - Register all subsystems and build final executable
# ============================================================

cmake_minimum_required(VERSION 3.16)

project(OCRtoODT LANGUAGES CXX)

# ------------------------------------------------------------
# Compiler configuration
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Qt6 modules
# ============================================================
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    Concurrent
)

# ============================================================
# Poppler-Qt6 (PDF rendering engine)
# ============================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLERQT6 REQUIRED poppler-qt6)

include_directories(${POPPLERQT6_INCLUDE_DIRS})
link_directories(${POPPLERQT6_LIBRARY_DIRS})

# ============================================================
# OpenCV (EnhanceProcessor)
# ============================================================
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

if(OpenCV_FOUND)
    message(STATUS "OpenCV version: ${OpenCV_VERSION}")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "OpenCV not found! Install libopencv-dev")
endif()

# ============================================================
# Embedded Tesseract OCR (local version inside project)
#
# Folder structure:
#   thirdparty/tesseract/include/tesseract/*.h
#   thirdparty/tesseract/lib/libtesseract.so
#   thirdparty/tesseract/tesseract (CLI)
#   thirdparty/tessdata/*.traineddata
#
# This completely removes dependency on system libtesseract-dev.
# ============================================================

set(EMBED_TESSERACT_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/tesseract)
set(EMBED_TESSDATA_ROOT  ${CMAKE_SOURCE_DIR}/thirdparty/tessdata)

# Header files (baseapi.h, publictypes.h, etc.)
include_directories(${EMBED_TESSERACT_ROOT}/include)

# Library directory (libtesseract.so)
link_directories(${EMBED_TESSERACT_ROOT}/lib)


# ============================================================
# Include directories for project headers
# ============================================================
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/providers
    ${CMAKE_SOURCE_DIR}/src/core/services
    ${CMAKE_SOURCE_DIR}/src/0_input
    ${CMAKE_SOURCE_DIR}/src/1_preprocess
    ${CMAKE_SOURCE_DIR}/src/1_preprocess/filters
    ${CMAKE_SOURCE_DIR}/src/2_ocr
    ${CMAKE_SOURCE_DIR}/src/3_LineTextBuilder
    ${CMAKE_SOURCE_DIR}/dialogs
    ${CMAKE_SOURCE_DIR}/settings
    ${CMAKE_SOURCE_DIR}/systeminfo
)



# Tesseract libraries
set(TESSERACT_LIBS
    ${EMBED_TESSERACT_ROOT}/lib/libtesseract.so
)

message(STATUS "Using embedded Tesseract")
message(STATUS "Tesseract include: ${EMBED_TESSERACT_ROOT}/include")
message(STATUS "Tesseract libs   : ${TESSERACT_LIBS}")

# ============================================================
# Qt automatic MOC/UIC/RCC
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/settings
)



# ============================================================
# Core subsystem sources / headers
# ============================================================
set(CORE_SOURCES
    src/core/ConfigManager.cpp
    src/core/ThemeManager.cpp
    src/core/LogRouter.cpp
    src/core/PerformanceProfiler.cpp
    src/core/ResourceManager.cpp
    src/core/ProgressManager.cpp
    src/core/ocrPdfCache.cpp
    src/core/pdfDocumentService.cpp
    src/core/providers/baseProvider.cpp
    src/core/services/PopplerService.cpp
)

set(CORE_HEADERS
    src/core/ConfigManager.h
    src/core/ThemeManager.h
    src/core/LogRouter.h
    src/core/PerformanceProfiler.h
    src/core/ResourceManager.h
    src/core/ProgressManager.h
    src/core/ocrPdfCache.h
    src/core/pdfDocumentService.h
    src/core/providers/baseProvider.h
    src/core/services/PopplerService.h
    src/core/VirtualPage.h
)

# ============================================================
# Input subsystem (0_input)
# ============================================================
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

# ============================================================
# Preprocess subsystem (1_preprocess)
# ============================================================
set(PREPROCESS_SOURCES
    src/1_preprocess/ImageLoader.cpp
    src/1_preprocess/EnhanceProcessor.cpp
    src/1_preprocess/Preprocess_Pipeline.cpp
)

set(PREPROCESS_HEADERS
    src/1_preprocess/ImageLoader.h
    src/1_preprocess/EnhanceProcessor.h
    src/1_preprocess/Preprocess_Pipeline.h
    src/1_preprocess/PageJob.h
)

set(PREPROCESS_FILTER_SOURCES
    src/1_preprocess/filters/gaussian.cpp
    src/1_preprocess/filters/clahe.cpp
    src/1_preprocess/filters/sharpen.cpp
    src/1_preprocess/filters/background_norm.cpp
    src/1_preprocess/filters/shadow_removal.cpp
    src/1_preprocess/filters/sauvola.cpp
)

set(PREPROCESS_FILTER_HEADERS
    src/1_preprocess/filters/gaussian.h
    src/1_preprocess/filters/clahe.h
    src/1_preprocess/filters/sharpen.h
    src/1_preprocess/filters/background_norm.h
    src/1_preprocess/filters/shadow_removal.h
    src/1_preprocess/filters/sauvola.h
)

# ============================================================
# OCR subsystem (2_ocr)
# ============================================================
set(OCR_SOURCES

    src/2_ocr/OcrPipeLineController.cpp
    src/2_ocr/OcrPageWorker.cpp
)

set(OCR_HEADERS

    src/2_ocr/OcrPipeLineController.h

    src/2_ocr/OcrResult.h
    src/2_ocr/OcrPageWorker.h
)

# ============================================================
# Dialogs (top-level)
# ============================================================
set(DIALOG_SOURCES
    dialogs/aboutdialog.cpp
    dialogs/helpdialog.cpp
    dialogs/settingsdialog.cpp
    dialogs/searchhighlighter.cpp
    dialogs/pageframe.cpp
    dialogs/export.cpp
)

set(DIALOG_HEADERS
    dialogs/aboutdialog.h
    dialogs/helpdialog.h
    dialogs/settingsdialog.h
    dialogs/searchhighlighter.h
    dialogs/pageframe.h
    dialogs/export.h
)

# ============================================================
# Settings panes
# ============================================================
set(SETTINGS_SOURCES
    settings/generalpane.cpp
    settings/preprocesspane.cpp
    settings/recognitionpane.cpp
    settings/odtpane.cpp
    settings/interfacepane.cpp

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

# ============================================================
# System info module
# ============================================================
set(SYSTEMINFO_SOURCES
    src/systeminfo/systeminfo.cpp
)

set(SYSTEMINFO_HEADERS
    src/systeminfo/systeminfo.h
)

# ============================================================
# Main window / entry point
# ============================================================
set(APP_SOURCES
    src/main.cpp
    src/mainwindow.cpp
)

set(APP_HEADERS
    src/mainwindow.h
)

# ============================================================
# UI files (global)
# ============================================================
set(UI_FILES
    ui/mainwindow.ui
    ui/aboutdialog.ui
    ui/helpdialog.ui
    ui/settingsdialog.ui
    ui/exportdialog.ui
)

# ============================================================
# Qt Resource files
# ============================================================
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
# Build executable
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
    ${PREPROCESS_FILTER_SOURCES}
    ${PREPROCESS_FILTER_HEADERS}

    ${OCR_SOURCES}
    ${OCR_HEADERS}

    ${DIALOG_SOURCES}
    ${DIALOG_HEADERS}

    ${SETTINGS_SOURCES}
    ${SETTINGS_HEADERS}

    ${SYSTEMINFO_SOURCES}
    ${SYSTEMINFO_HEADERS}

    ${UI_FILES}
    ${SETTINGS_UI}
    ${RESOURCE_FILES}

    src/1_preprocess/filters/adaptive_threshold.h
    src/1_preprocess/filters/adaptive_threshold.cpp
    src/1_preprocess/ImageAnalyzer.h
    src/1_preprocess/ImageAnalyzer.cpp
    src/1_preprocess/StrategySelector.h
    src/1_preprocess/StrategySelector.cpp

    src/2_ocr/OcrPassConfig.h
    src/2_ocr/OcrTsvQuality.h
    src/2_ocr/OcrTsvQuality.cpp
    src/2_ocr/OcrMultipassSelector.h
    src/2_ocr/OcrMultipassSelector.cpp
    src/2_ocr/OcrPipeLineWorker.h
    src/2_ocr/OcrPipeLineWorker.cpp

    src/3_LineTextBuilder/LineRow.h
    src/3_LineTextBuilder/LineTable.h
    src/3_LineTextBuilder/LineTextBuilder.h
    src/3_LineTextBuilder/LineTextBuilder.cpp


    src/core/processors/RecognitionProcessor.h
    src/core/processors/RecognitionProcessor.cpp
    src/core/processors/ExportProcessor.h
    src/core/processors/ExportProcessor.cpp
    src/core/processors/InputProcessor.h
    src/core/processors/InputProcessor.cpp

    resources/styles/general_cards.qss
    src/3_LineTextBuilder/LineTableSerializer.h
    src/3_LineTextBuilder/LineTableSerializer.cpp
    src/4_edit_lines/LineTableModel.h
    src/4_edit_lines/LineTextDelegate.h
    src/4_edit_lines/EditLinesController.h
    src/4_edit_lines/EditLinesControllerEvents.h
    src/4_edit_lines/LineTableModel.cpp
    src/4_edit_lines/LineHitTest.h
    src/4_edit_lines/LineHitTest.cpp
    src/4_edit_lines/EditLinesController.cpp
    src/4_edit_lines/LineTextDelegate.cpp
    src/5_document/DocumentModel.h
    src/5_document/DocumentBuilder.h
    src/5_document/DocumentBuilder.cpp
    src/5_document/DocumentDebugWriter.h
    src/5_document/DocumentDebugWriter.cpp
    src/5_export/txt_export/TxtExporter.h
    src/5_export/txt_export/TxtExporter.cpp
    src/5_export/ExportController.h
    src/5_export/ExportController.cpp
    src/5_export/odt_export/OdtExporter.h
    src/5_export/odt_export/OdtExporter.cpp
    src/5_export/docx_export/DocxExporter.h
    src/5_export/docx_export/DocxExporter.cpp
    src/5_export/docx_export/DocxExporter.cpp
    resources/themes/dark.qss
    resources/themes/light.qss
    settings/interfacepane_preview.cpp
    settings/interfacepane_samples.cpp
    settings/interfacepane_qss.cpp
    settings/interfacepane_language.cpp
    src/core/LanguageManager.h
    src/core/LanguageManager.cpp
    resources/translations/ocrtoodt_en.qm
    resources/translations/ocrtoodt_ro.qm
    resources/translations/ocrtoodt_ru.qm
    resources/icons.qrc
    ui/OcrCompletionDialog.ui
    dialogs/OcrCompletionDialog.h
    dialogs/OcrCompletionDialog.cpp


    src/core/layout/OdtLayoutModel.cpp
    src/core/layout/OdtLayoutModel.h
    src/5_export/ExportTextNormalizer.h
    src/5_export/ExportTextNormalizer.cpp
    src/5_export/docx_export/DocxExporter.cpp
    docs/README.md

)

qt_add_translations(OCRtoODT TS_FILES resources/translations/ocrtoodt_ru.ts
    resources/translations/ocrtoodt_en.ts
    resources/translations/ocrtoodt_ro.ts)

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(OCRtoODT PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    ${POPPLERQT6_LIBRARIES}
    ${OpenCV_LIBS}
    ${TESSERACT_LIBS}
)
