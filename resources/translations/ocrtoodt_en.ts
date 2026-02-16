<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US">

<context>
    <name>MainWindow</name>
    <message><source>OCRtoODT GUI</source><translation>OCRtoODT GUI</translation></message>
    <message><source>File list</source><translation>File list</translation></message>
    <message><source>Preview</source><translation>Preview</translation></message>
    <message><source>Fit</source><translation>Fit</translation></message>
    <message><source>OCR Result</source><translation>OCR Result</translation></message>
    <message><source>Ready</source><translation>Ready</translation></message>
    <message><source>OCR in progress…</source><translation>OCR in progress…</translation></message>
    <message><source>Open</source><translation>Open</translation></message>
    <message><source>Clear</source><translation>Clear</translation></message>
    <message><source>Settings</source><translation>Settings</translation></message>
    <message><source>Run</source><translation>Run</translation></message>
    <message><source>Stop</source><translation>Stop</translation></message>
    <message><source>Export</source><translation>Export</translation></message>
    <message><source>About</source><translation>About</translation></message>
    <message><source>Help</source><translation>Help</translation></message>
</context>

<context>
    <name>AboutDialog</name>

    <!-- Window title -->
    <message>
        <source>About OCRtoODT</source>
        <translation>About OCRtoODT</translation>
    </message>

    <!-- Title + description (rich text) -->
    <message>
        <source>
       &lt;h2&gt;OCRtoODT&lt;/h2&gt;
       &lt;p&gt;GUI for OCR processing and ODT document assembly&lt;/p&gt;
      </source>
        <translation>
       &lt;h2&gt;OCRtoODT&lt;/h2&gt;
       &lt;p&gt;GUI for OCR processing and ODT document assembly&lt;/p&gt;
      </translation>
    </message>

    <!-- Version -->
    <message>
        <source>Version 0.3 (November 2025)</source>
        <translation>Version 0.3 (November 2025)</translation>
    </message>

    <!-- GitHub link (rich text) -->
    <message>
        <source>
       &lt;a href="https://github.com/Rostislav62/OCRtoODT"&gt;
       GitHub: OCRtoODT
       &lt;/a&gt;
      </source>
        <translation>
       &lt;a href="https://github.com/Rostislav62/OCRtoODT"&gt;
       GitHub: OCRtoODT
       &lt;/a&gt;
      </translation>
    </message>

    <!-- Dialog button -->
    <message>
        <source>OK</source>
        <translation>OK</translation>
    </message>

</context>

<context>
    <name>ExportDialog</name>

    <!-- Window title -->
    <message>
        <source>Export Document</source>
        <translation>Export Document</translation>
    </message>

    <!-- Dialog title -->
    <message>
        <source>Export OCR Result</source>
        <translation>Export OCR Result</translation>
    </message>

    <!-- Form labels -->
    <message>
        <source>Export format:</source>
        <translation>Export format:</translation>
    </message>

    <message>
        <source>File name:</source>
        <translation>File name:</translation>
    </message>

    <message>
        <source>Destination folder:</source>
        <translation>Destination folder:</translation>
    </message>

    <!-- Placeholder -->
    <message>
        <source>ocr_output</source>
        <translation>ocr_output</translation>
    </message>

    <!-- Buttons / actions -->
    <message>
        <source>Browse…</source>
        <translation>Browse…</translation>
    </message>

    <message>
        <source>Open file after export</source>
        <translation>Open file after export</translation>
    </message>

    <message>
        <source>OK</source>
        <translation>OK</translation>
    </message>

    <message>
        <source>Cancel</source>
        <translation>Cancel</translation>
    </message>

</context>


<context>
    <name>HelpDialog</name>

    <!-- Window title -->
    <message>
        <source>OCRtoODT Help</source>
        <translation>OCRtoODT Help</translation>
    </message>

    <!-- Header -->
    <message>
        <source>&lt;h2&gt;OCRtoODT — Documentation&lt;/h2&gt;</source>
        <translation>&lt;h2&gt;OCRtoODT — Documentation&lt;/h2&gt;</translation>
    </message>

    <!-- Search -->
    <message>
        <source>Find:</source>
        <translation>Find:</translation>
    </message>

    <message>
        <source>←</source>
        <translation>←</translation>
    </message>

    <message>
        <source>→</source>
        <translation>→</translation>
    </message>

    <message>
        <source>0 / 0</source>
        <translation>0 / 0</translation>
    </message>

    <!-- Hint -->
    <message>
        <source>[!] Tip: use Ctrl+F to focus the search field</source>
        <translation>[!] Tip: use Ctrl+F to focus the search field</translation>
    </message>

    <!-- Buttons -->
    <message>
        <source>Close</source>
        <translation>Close</translation>
    </message>

</context>


<context>
    <name>SettingsDialog</name>
    <message><source>OCRtoODT Settings</source><translation>OCRtoODT Settings</translation></message>
    <message><source>OK</source><translation>OK</translation></message>
    <message><source>Cancel</source><translation>Cancel</translation></message>
</context>


<context>
    <name>GeneralSettingsPane</name>
    <message><source>General</source><translation>General</translation></message>


    <message><source>Execution strategy</source><translation>Execution strategy</translation></message>

    <message><source>Automatic</source><translation>Automatic</translation></message>
    <message><source>Sequential</source><translation>Sequential</translation></message>
    <message><source>Automatic mode is recommended.</source><translation>Automatic mode is recommended.</translation></message>
    <message><source>Sequential mode uses fewer resources.</source><translation>Sequential mode uses fewer resources.</translation></message>
    <message><source>Automatic mode uses all available CPU cores.</source><translation>Automatic mode uses all available CPU cores.</translation></message>
    <message><source>Sequential mode uses a single CPU core and fewer resources.</source><translation>Sequential mode uses a single CPU core and fewer resources.</translation></message>



    <message><source>Data execution mode</source><translation>Data execution mode</translation></message>

    <message><source>Memory only</source><translation>Memory only</translation></message>
    <message><source>Disk assisted</source><translation>Disk assisted</translation></message>
    <message><source>Program automatically chooses the optimal data execution mode.</source><translation>Program automatically chooses the optimal data execution mode.</translation></message>
    <message><source>All processing steps are executed strictly in memory.</source><translation>All processing steps are executed strictly in memory.</translation></message>
    <message><source>Disk is used as primary data storage (recommended for large documents).</source><translation>Disk is used as primary data storage (recommended for large documents).</translation></message>



    <message><source>Control level</source><translation>Control level</translation></message>

    <message><source>Standard</source><translation>Standard</translation></message>
    <message><source>Professional</source><translation>Professional</translation></message>
    <message><source>Recommended mode with safe default settings.</source><translation>Recommended mode with safe default settings.</translation></message>
    <message><source>Professional mode unlocks advanced settings. Changing them may affect OCR quality.</source><translation>Professional mode unlocks advanced settings. Changing them may affect OCR quality.</translation></message>
    <message><source>Unlocks advanced settings for experienced users.</source><translation>Unlocks advanced settings for experienced users.</translation></message>


</context>


<context>
    <name>PreprocessSettingsPane</name>

    <!-- UI: basic profile selection -->
    <message>
        <source>Input image type:</source>
        <translation>Input image type:</translation>
    </message>

    <message>
        <source>Photo from mobile phone</source>
        <translation>Photo from mobile phone</translation>
    </message>

    <message>
        <source>Scanner image</source>
        <translation>Scanner image</translation>
    </message>

    <message>
        <source>Low quality</source>
        <translation>Low quality</translation>
    </message>

    <message>
        <source>Automatic analyzer</source>
        <translation>Automatic analyzer</translation>
    </message>

    <!-- Descriptions -->
    <message>
        <source>Photo from a mobile phone — uneven lighting, shadows, requires more aggressive preprocessing.</source>
        <translation>Photo from a mobile phone — uneven lighting, shadows, requires more aggressive preprocessing.</translation>
    </message>

    <message>
        <source>Scanner image — minimal processing without unnecessary distortions. Recommended as default.</source>
        <translation>Scanner image — minimal processing without unnecessary distortions. Recommended as default.</translation>
    </message>

    <message>
        <source>Low quality — noise, blurred characters, old books. May require stronger filtering.</source>
        <translation>Low quality — noise, blurred characters, old books. May require stronger filtering.</translation>
    </message>

    <message>
        <source>Automatic analyzer — profile is determined automatically based on source and quality.</source>
        <translation>Automatic analyzer — profile is determined automatically based on source and quality.</translation>
    </message>

    <message>
        <source>Reset current profile to default values.</source>
        <translation>Reset current profile to default values.</translation>
    </message>

    <!-- Advanced section -->
    <message>
        <source>Advanced preprocessing parameters</source>
        <translation>Advanced preprocessing parameters</translation>
    </message>

    <message>
        <source>Changes affect recognition quality.
If you are unsure, use the standard profile.</source>
        <translation>Changes affect recognition quality.
If you are unsure, use the standard profile.</translation>
    </message>

    <message>
        <source>Enabled</source>
        <translation>Enabled</translation>
    </message>

    <!-- Reset confirmation dialog -->
    <message>
        <source>Reset profile</source>
        <translation>Reset profile</translation>
    </message>

    <message>
        <source>Reset profile "%1" to default values?

All changes made in expert mode for this profile will be lost.</source>
        <translation>Reset profile "%1" to default values?

All changes made in expert mode for this profile will be lost.</translation>
    </message>

    <message>
        <source>Shadow removal (shadow_removal)</source>
        <translation>Shadow removal (shadow_removal)</translation>
    </message>

    <message>
        <source>Background normalization (background_normalization)</source>
        <translation>Background normalization (background_normalization)</translation>
    </message>

    <message>
        <source>Noise reduction (gaussian_blur)</source>
        <translation>Noise reduction (gaussian_blur)</translation>
    </message>

    <message>
        <source>Local contrast enhancement (clahe)</source>
        <translation>Local contrast enhancement (clahe)</translation>
    </message>

    <message>
        <source>Sharpening (sharpen)</source>
        <translation>Sharpening (sharpen)</translation>
    </message>

    <message>
        <source>Adaptive thresholding (adaptive_threshold)</source>
        <translation>Adaptive thresholding (adaptive_threshold)</translation>
    </message>

</context>

<context>
    <name>RecognitionSettingsPane</name>

    <!-- OCR languages -->
    <message>
        <source>OCR languages:</source>
        <translation>OCR languages:</translation>
    </message>

    <message>
        <source>Refresh language list</source>
        <translation>Refresh language list</translation>
    </message>

    <message>
        <source>Install language…</source>
        <translation>Install language…</translation>
    </message>

    <message>
        <source>Active languages:</source>
        <translation>Active languages:</translation>
    </message>

    <!-- Notifications -->
    <message>
        <source>Show notification dialog after OCR completion</source>
        <translation>Show notification dialog after OCR completion</translation>
    </message>

    <message>
        <source>Play sound after OCR completion</source>
        <translation>Play sound after OCR completion</translation>
    </message>

    <!-- Sound -->
    <message>
        <source>Test</source>
        <translation>Test</translation>
    </message>

</context>


<context>
    <name>InterfaceSettingsPane</name>

    <!-- Window / tab title -->
    <message>
        <source>Interface</source>
        <translation>Interface</translation>
    </message>

    <!-- Appearance -->
    <message>
        <source>Appearance</source>
        <translation>Appearance</translation>
    </message>

    <message>
        <source>Theme mode</source>
        <translation>Theme mode</translation>
    </message>

    <message>
        <source>Custom theme (QSS)</source>
        <translation>Custom theme (QSS)</translation>
    </message>

    <message>
        <source>Browse…</source>
        <translation>Browse…</translation>
    </message>

    <!-- Language -->
    <message>
        <source>Language</source>
        <translation>Language</translation>
    </message>

    <message>
        <source>Interface language</source>
        <translation>Interface language</translation>
    </message>

    <!-- Fonts -->
    <message>
        <source>Fonts</source>
        <translation>Fonts</translation>
    </message>

    <message>
        <source>Application font</source>
        <translation>Application font</translation>
    </message>

    <message>
        <source>Interface font size</source>
        <translation>Interface font size</translation>
    </message>

    <!-- Toolbar -->
    <message>
        <source>Toolbar</source>
        <translation>Toolbar</translation>
    </message>

    <message>
        <source>Icons only</source>
        <translation>Icons only</translation>
    </message>

    <message>
        <source>Text only</source>
        <translation>Text only</translation>
    </message>

    <message>
        <source>Icons and text</source>
        <translation>Icons and text</translation>
    </message>

    <message>
        <source>Icon size</source>
        <translation>Icon size</translation>
    </message>

    <!-- Thumbnails -->
    <message>
        <source>Page thumbnails</source>
        <translation>Page thumbnails</translation>
    </message>

    <message>
        <source>Thumbnail size (px)</source>
        <translation>Thumbnail size (px)</translation>
    </message>

    <!-- Preview -->
    <message>
        <source>OCRtoODT GUI</source>
        <translation>OCRtoODT GUI</translation>
    </message>

    <message>
        <source>Pages</source>
        <translation>Pages</translation>
    </message>

    <message>
        <source>OCR Result</source>
        <translation>OCR Result</translation>
    </message>

    <!-- Zoom -->
    <message>
        <source>+</source>
        <translation>+</translation>
    </message>

    <message>
        <source>-</source>
        <translation>-</translation>
    </message>

    <message>
        <source>Fit</source>
        <translation>Fit</translation>
    </message>

    <message>
        <source>100%</source>
        <translation>100%</translation>
    </message>

    <message><source>Open</source><translation>Open</translation></message>
    <message><source>Clear</source><translation>Clear</translation></message>
    <message><source>Settings</source><translation>Settings</translation></message>
    <message><source>Run</source><translation>Run</translation></message>
    <message><source>Stop</source><translation>Stop</translation></message>
    <message><source>Export</source><translation>Export</translation></message>
    <message><source>About</source><translation>About</translation></message>
    <message><source>Help</source><translation>Help</translation></message>

</context>


<context>
    <name>ODTSettingsPane</name>

    <message>
        <source>Typography</source>
        <translation>Typography</translation>
    </message>

    <message>
        <source>Paragraph</source>
        <translation>Paragraph</translation>
    </message>

    <message>
        <source>Line spacing</source>
        <translation>Line spacing</translation>
    </message>

    <message>
        <source>Page layout</source>
        <translation>Page layout</translation>
    </message>

    <message>
        <source>Page size</source>
        <translation>Page size</translation>
    </message>

    <message>
        <source>Font</source>
        <translation>Font</translation>
    </message>

    <message>
        <source>Font size (pt)</source>
        <translation>Font size (pt)</translation>
    </message>

    <message>
        <source>Alignment</source>
        <translation>Alignment</translation>
    </message>

    <message>
        <source>First line indent (mm)</source>
        <translation>First line indent (mm)</translation>
    </message>

    <message>
        <source>Spacing after</source>
        <translation>Spacing after</translation>
    </message>

    <message>
        <source>Single</source>
        <translation>Single</translation>
    </message>

    <message>
        <source>1.5</source>
        <translation>1.5</translation>
    </message>

    <message>
        <source>Double</source>
        <translation>Double</translation>
    </message>

    <message>
        <source>Custom…</source>
        <translation>Custom…</translation>
    </message>

    <message>
        <source>Top margin</source>
        <translation>Top margin</translation>
    </message>

    <message>
        <source>Bottom margin</source>
        <translation>Bottom margin</translation>
    </message>

    <message>
        <source>Left margin</source>
        <translation>Left margin</translation>
    </message>

    <message>
        <source>Right margin</source>
        <translation>Right margin</translation>
    </message>

    <message>
        <source>Max empty lines</source>
        <translation>Max empty lines</translation>
    </message>

    <message>
        <source>Insert page break between OCR pages</source>
        <translation>Insert page break between OCR pages</translation>
    </message>

    <message>
        <source>0 pt</source>
        <translation>0 pt</translation>
    </message>

    <message>
        <source>3 pt</source>
        <translation>3 pt</translation>
    </message>

    <message>
        <source>6 pt</source>
        <translation>6 pt</translation>
    </message>

    <message>
        <source>12 pt</source>
        <translation>12 pt</translation>
    </message>

</context>



</TS>
