# OCRtoODT — Developer Documentation

Technical documentation for contributors and maintainers of OCRtoODT.

This document covers:

- development workflow
- containerized Linux setup
- build instructions
- release workflow
- project architecture
- runtime configuration

For end users, see the root `README.md`.

Release packaging details are documented separately in `docs/RELEASING_APPIMAGE.md`.

## 🚀 One-command AppImage Release (Local + GitHub)

OCRtoODT uses a **single source of truth** for releases: the version declared in `CMakeLists.txt`:

`project(OCRtoODT VERSION X.Y.Z LANGUAGES CXX)`

Based on that version, the project provides a unified release script: `build_appimage.sh`.

It guarantees:

- Version is read **only** from `CMakeLists.txt`
- Git tag must be exactly `vX.Y.Z` and must point to the current `HEAD`
- Local AppImage output is always named: `OCRtoODT-vX.Y.Z-x86_64.AppImage`
- Optional push of commit + tag to GitHub to trigger GitHub Actions release build
- No `git clean` is ever executed
- Untracked/ignored local files (dumps, experiments, etc.) are not touched and do not block the release

## ✅ Script modes

### 1) Full release (local + GitHub)

- Auto-commit version bump, but only when the **only tracked change** is `CMakeLists.txt`
- Auto-create the required tag `vX.Y.Z` if missing on `HEAD`
- Push branch + tag to `origin`
- Build local AppImage

### 2) Local-only build (`--no-push`)

- Same validation/build pipeline
- **No push** to GitHub

## 📌 Release workflow

### 1) Change only the version in CMake

Edit `CMakeLists.txt`:

```cmake
project(OCRtoODT VERSION 3.0.5 LANGUAGES CXX)
```

Do not modify other tracked files.  
### **2) Run one command**  
Full release:  
chmod +x build_appimage.sh  
 ./build_appimage.sh --auto-commit-version  
Local-only:  
./build_appimage.sh --auto-commit-version --no-push  
## **🔒 Anti-desync guards**  
The release script validates:  
-  no staged/unstaged **tracked** changes   
-  version from CMakeLists.txt is X.Y.Z  
- HEAD has an exact tag vX.Y.Z  
- output file name always includes the version  
### **Auto-commit behavior (** **--auto-commit-version** **)**  
When --auto-commit-version is enabled, the script auto-commits **only** if the  **only tracked change** is CMakeLists.txt.  
Then it runs:  
git add CMakeLists.txt  
 git commit -m "Bump version to X.Y.Z"  
If any other tracked file is modified, it refuses to proceed.  
## **🧩 Local AppImage prerequisites**  
The repository root must contain:  
- linuxdeploy-x86_64.AppImage  
- linuxdeploy-plugin-qt-x86_64.AppImage  
If missing, download them once:  
wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \  
   -O linuxdeploy-x86_64.AppImage  
   
 wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \  
   -O linuxdeploy-plugin-qt-x86_64.AppImage  
   
 chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage  
### **Qt6 note**  
Some environments may default to Qt5 or qtchooser. The release script forces Qt6 qmake and provides local shims when needed to keep packaging reproducible.  
## **📌 Overview**  
**OCRtoODT** is a deterministic, inspectable OCR extraction system built in modern C++ with Qt 6.  
It is not a text editor.  
  It is not a word processor.  
It is a structured OCR data pipeline designed for:  
- archival digitization  
- research workflows  
- legal and academic OCR  
- structured document processing  
- reproducible OCR experiments  
Core goals:  
- structural correctness  
- predictable multi-stage processing  
- RAM-first execution  
- traceability  
# **🎯 Philosophy**  
Structure first. Formatting later.  
OCRtoODT preserves raw OCR results and transforms them into structured representations without hidden modifications.  
The application aims for:  
- deterministic pipeline execution  
- no silent post-processing distortion  
- explicit configuration control  
- isolated stage responsibilities  
# **🏗 Processing Pipeline**  
OCRtoODT is built as a strict stage-based engine:  
- STEP 0 → Input  
- STEP 1 → Preprocess  
- STEP 2 → OCR  
- STEP 3 → TSV Structuring  
- STEP 4 → UI Synchronization  
- STEP 5 → Export  
Each stage:  
- has a defined contract  
- produces predictable output  
- does not own UI logic  
- avoids ownership violations  
## **🔹 STEP 0 — Input**  
- PDF loading (Poppler-Qt6)  
- Raster image import (PNG, JPEG, TIFF)  
- Page expansion  
- Thumbnail generation  
- VirtualPage construction  
## **🔹 STEP 1 — Preprocessing**  
OpenCV-based:  
- grayscale normalization  
- adaptive thresholding  
- CLAHE  
- shadow removal  
- sharpening  
Threaded and resource-aware.  
## **🔹 STEP 2 — OCR**  
- Embedded Tesseract  
- Multi-pass PSM execution  
- Quality scoring  
- Best-pass selection  
- RAM-first TSV handling  
- Cooperative cancellation  
## **🔹 STEP 3 — TSV Structuring**  
- TSV → LineTable conversion  
- Structural line grouping  
- RAM-first processing  
- Optional debug disk persistence  
## **🔹 STEP 4 — UI Synchronization**  
- Preview ↔ text mapping  
- Line highlighting  
- Structured inspection  
The UI does not perform OCR logic.  
## **🔹 STEP 5 — Export**  
- ODT export  
- TXT export  
- DOCX export  
- Structured document building  
# **🧱 Core Data Model**  
## **Core::VirtualPage**  
Central pipeline object.  
Contains:  
- source metadata  
- OCR TSV text  
- OCR success flag  
-  structured LineTable  
- layout extensions for future work  
It is the single source of truth across the pipeline.  
# **⚙ Execution Model**  
## **RAM-first design**  
Primary data storage is memory.  
  Disk use is optional and policy-driven.  
Modes:  
- ram_only  
- disk_only  
- debug_mode  
Configured via config.yaml.  
# **📊 Progress System**  
Centralized ProgressManager provides:  
- stage-aware progress  
- global percentage  
- ETA support  
- cancellation-safe reset  
- deterministic finish handling  
# **🧾 Logging System**  
LogRouter provides:  
- canonical log levels (0–4)  
- runtime verbosity control  
- file + console routing  
- profiling-oriented diagnostics  
Designed for auditability and debugging.  
# **🖥 Graphical Interface**  
Built with Qt 6 Widgets.  
Main UI areas:  
- input panel (files + thumbnails)  
- preview panel (zoom, fit, highlight)  
- structured text panel  
- settings dialog  
- export dialog  
The UI is inspection-focused, not editing-focused.  
# **🗂 Repository Structure**  
src/  
  ├── 0_input/  
  ├── 1_preprocess/  
  ├── 2_ocr/  
  ├── 3_LineTextBuilder/  
  ├── 4_edit_lines/  
  ├── 5_export/  
  ├── 5_document/  
  └── core/  
   
 dialogs/  
 settings/  
 resources/  
 docs/  
 scripts/  
The layout is modular and stage-aligned.  
# **🔧 Build Instructions**  
## **Linux development: recommended workflow**  
For Linux contributors, the recommended workflow is:  
- edit the project in Qt Creator  
- build in Podman  
- run in Podman  
- avoid installing the full dependency stack on the host  
This keeps the host system cleaner and avoids dependency drift between machines.  
## **Containerized development workflow (Podman)**  
OCRtoODT supports an isolated Podman-based development workflow on Linux.  
This workflow is recommended when you want:  
- reproducible builds  
- a clean host system  
- isolated project dependencies  
- consistent Qt / OpenCV / Poppler / Tesseract environment  
How it works:  
- source files remain on the host  
- the project is edited normally in Qt Creator  
- build and run happen inside a Podman container  
- the container mounts the project directory  
- the runtime container also mounts the user home directory for file access  
This avoids host-side dependency mismatches and makes development more reproducible.  
## **Podman development scripts**  
The Linux containerized workflow uses these helper scripts:  
- scripts/dev-shell.sh — open an interactive shell inside the development container   
- scripts/configure.sh — run CMake configure inside the container   
- scripts/build.sh — build the project inside the container   
- scripts/rebuild-in-podman.sh — rebuild the project from the host using Podman   
- scripts/run-in-podman.sh — run the application inside the container   
- scripts/rebuild-and-run.sh — rebuild and immediately run the application   
## **Requirements**  
General build requirements:  
- C++17 compatible compiler  
- CMake  
- Ninja  
- Qt 6  
- OpenCV  
- Poppler-Qt6  
- Tesseract development libraries  
- Podman for the recommended Linux workflow  
## **Generic build (non-container)**  
Standard CMake build flow:  
git clone https://github.com/Rostislav62/OCRtoODT.git  
 cd OCRtoODT  
   
 mkdir build  
 cd build  
   
 cmake ..  
 cmake --build . -j  
Run:  
./OCRtoODT  
For Linux contributors, this is not the preferred daily workflow. The recommended Linux development path is the Podman-based workflow described below.  
# **Linux development (Qt Creator + Podman)**  
## **Qt Creator workflow**  
Recommended workflow:  
-  open the project in Qt Creator using **Open Workspace**  
- edit files normally in Qt Creator  
- use an external terminal for build and run  
- do not rely on host-side dependency resolution for this workflow  
Practical model:  
- **Qt Creator** = editor / navigation / forms / code model   
- **Podman** = build environment and runtime environment   
## **Typical development loop**  
Project path on host:  
/home/dev/projects/cpp/OCRtoODT  
### **Rebuild**  
cd /home/dev/projects/cpp/OCRtoODT  
 ./scripts/rebuild-in-podman.sh  
### **Run**  
cd /home/dev/projects/cpp/OCRtoODT  
 ./scripts/run-in-podman.sh  
### **Rebuild and run**  
cd /home/dev/projects/cpp/OCRtoODT  
 ./scripts/rebuild-and-run.sh  
## **Host vs container responsibilities**  
### **Run on the host**  
These scripts must be executed from the host system:  
- ./scripts/dev-shell.sh  
- ./scripts/rebuild-in-podman.sh  
- ./scripts/run-in-podman.sh  
- ./scripts/rebuild-and-run.sh  
### **Run inside the container**  
These commands are intended to run inside the container:  
- ./scripts/configure.sh  
- ./scripts/build.sh  
- ./build/podman-debug/OCRtoODT  
Container mount mapping:  
-  host: /home/dev/projects/cpp/OCRtoODT  
-  container: /workspace  
Editing a file on the host immediately affects the same mounted project inside the container.  
## **Why runtime execution uses the container**  
The development binary is built against the libraries installed inside the Podman image.  
If the host system provides different Qt runtime versions than the container, launching the raw binary directly on the host may fail with shared-library or symbol-version errors.  
For that reason, the recommended development run path is:  
./scripts/run-in-podman.sh  
This guarantees that build-time and run-time libraries are identical.  
## **Access to user files during containerized run**  
The Podman runtime script mounts the user home directory into the container.  
That means the application can access normal user folders during development, including:  
- Pictures  
- Documents  
- Downloads  
This allows realistic file-import testing without copying files into the container manually.  
## **Optional host-side dependency installation**  
If you explicitly want to build on the host instead of using Podman, install dependencies such as:  
sudo apt update  
 sudo apt install \  
     build-essential \  
     cmake \  
     ninja-build \  
     qt6-base-dev \  
     qt6-multimedia-dev \  
     qt6-tools-dev \  
     qt6-svg-dev \  
     libopencv-dev \  
     libpoppler-qt6-dev \  
     libtesseract-dev \  
     libleptonica-dev \  
     libarchive-dev \  
     libzip-dev  
Then use the normal CMake workflow.  
# **Windows build notes**  
Current Windows build expectations:  
- Qt 6 with MSVC toolchain  
- OpenCV  
- Poppler-Qt6  
- CMake + Ninja or Qt Creator  
-  runtime DLL availability in PATH or beside the executable   
Windows packaging and workflow details should be documented separately as they stabilize.  
# **🧪 Configuration**  
All runtime behavior is controlled via:  
config.yaml  
Features:  
- hierarchical structure  
- comment-preserving parser  
- safe runtime reload  
Example:  
general:  
   mode: ram_only  
   debug_mode: false  
   
 ocr:  
   languages: eng  
   psm_1: 4  
   psm_2: 6  
   
 ui:  
   theme_mode: dark  
   thumbnail_size: 160  
# **🧠 Technical Stack**  
| | |  
|-|-|  
| **Component** | **Technology** |   
| Language | C++17 |   
| GUI | Qt 6 |   
| OCR | Embedded Tesseract |   
| Image Processing | OpenCV |   
| PDF Handling | Poppler-Qt6 |   
| Concurrency | QtConcurrent |   
| Config | Custom YAML (comment-preserving) |   
| Platforms | Linux / Windows / macOS |   
# **🚧 Roadmap**  
## **Developer quick reference**  
### **Open container shell**  
./scripts/dev-shell.sh  
### **Rebuild project**  
./scripts/rebuild-in-podman.sh  
### **Run application**  
./scripts/run-in-podman.sh  
### **Rebuild and run**  
./scripts/rebuild-and-run.sh  
Future directions:  
- advanced paragraph recovery  
- column detection improvements  
- footnote handling  
- layout reconstruction  
- packaging improvements  
- performance profiling dashboard  
# **👨‍💻 Author**  
**Rostislav Smigliuc**  
GitHub: [https://github.com/Rostislav62/](https://github.com/Rostislav62/ "https://github.com/Rostislav62/")  
# **📜 License**  
MIT License  
   
