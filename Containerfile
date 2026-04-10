FROM debian:testing

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    gdb \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-l10n-tools \
    qt6-multimedia-dev \
    qt6-svg-dev \
    qmake6 \
    libgl1-mesa-dev \
    libopengl-dev \
    libpoppler-qt6-dev \
    poppler-utils \
    tesseract-ocr \
    libtesseract-dev \
    libleptonica-dev \
    libarchive-dev \
    zlib1g-dev \
    libzip-dev \
    libopencv-dev \
    ca-certificates \
    bash \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
