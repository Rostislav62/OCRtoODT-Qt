# ============================================================
# OCRtoODT — Configuration Schema (v1.0)
# File: docs/config_schema.md
#
# This document defines the STRICT structure of config.yaml.
#
# RULES:
#  - Only keys defined here are allowed.
#  - No dynamic sections.
#  - No arrays.
#  - Only scalar values.
#  - Indentation is 2 spaces per level.
#  - Max nesting depth: 2.
#
# This file acts as:
#  - Contract between modules
#  - Validation reference
#  - Production safety specification
#
# ============================================================

config:
  version: 1
  # Increment ONLY on breaking schema change.


# ------------------------------------------------------------
# Logging system configuration
# ------------------------------------------------------------
logging:

  # Master logging switch
  enabled: true

  # Verbosity level:
  # 0 = Errors only
  # 1 = Warnings
  # 2 = Info
  # 3 = Verbose
  # 4 = Debug/Perf
  level: 3

  # Write log file to disk
  file_output: false

  # Show log inside UI
  gui_output: true

  # Print log to stdout
  console_output: true

  # File path relative to application dir
  file_path: log/ocrtoodt.log


# ------------------------------------------------------------
# User Interface configuration
# ------------------------------------------------------------
ui:

  # Theme mode:
  # light | dark | auto | system
  theme_mode: dark

  # Optional custom QSS path
  custom_qss: ""

  # Main application font
  app_font_family: ""
  app_font_size: 11

  # Log panel font
  log_font_size: 10

  # Toolbar display mode:
  # icons | text | icons_text
  toolbar_style: icons

  # Thumbnail size (100–200)
  thumbnail_size: 160

  # Enable advanced settings
  expert_mode: false

  # Notification settings
  notify_on_finish: true
  play_sound_on_finish: true
  sound_volume: 70
  sound_path: sounds/done.wav


# ------------------------------------------------------------
# General execution settings
# ------------------------------------------------------------
general:

  # Enable parallel processing
  parallel_enabled: true

  # Worker count:
  # auto | numeric value (future)
  num_processes: auto

  # Data execution mode:
  # auto | ram_only | disk_only
  mode: auto

  # Enable additional debug output
  debug_mode: false

  # Cache subfolders
  input_dir: input
  preprocess_path: preprocess


# ------------------------------------------------------------
# Preprocess configuration
# ------------------------------------------------------------
preprocess:

  # Preprocess profile
  profile: scanner


# ------------------------------------------------------------
# Recognition configuration
# ------------------------------------------------------------
recognition:

  # OCR language (tesseract code)
  language: eng

  # Page segmentation mode
  psm: 3


# ------------------------------------------------------------
# ODT export configuration
# ------------------------------------------------------------
odt:

  # Default font family
  font_family: Times New Roman

  # Default font size
  font_size: 12

  # Justify text
  justify: true
