# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PeakMan is a Qt-based desktop application for ECG (electrocardiogram) peak detection and artifact processing. It provides interactive visualization and analysis of cardiac data, implementing peak detection algorithms with local and global thresholding for R wave detection.

## Build System

### Building the Application

PeakMan uses Qt's qmake build system. The project file is located at `src/peakman.pro`.

**Using Qt Creator (Recommended):**
1. Open `src/peakman.pro` in Qt Creator
2. Build and run directly from the IDE

**Command Line Build:**
```bash
cd src
qmake peakman.pro
make  # or nmake on Windows, or mingw32-make with MinGW
```

### Requirements

- Qt 5.x or higher (with widgets and printsupport modules)
- Platform-specific:
  - **Windows**: GCC compiler (install with Qt)
  - **macOS**: Xcode
  - **Linux**: May need `mesa-common-dev` and `libgl1-mesa-dev` packages

## Architecture Overview

### Component Structure

PeakMan follows a Qt widgets-based MVC pattern with specialized plot classes:

**MainWindow (src/mainwindow.{h,cpp,ui})**
- Central coordinator and primary UI controller
- Manages file I/O for ECG data, peaks, and IBI files
- Provides unified export system that exports both peaks and IBIs simultaneously
- Coordinates signal flow between plot widgets
- Handles menu actions and user dialogs
- Manages settings persistence across sessions

**ECGPlot (src/ecgplot.{h,cpp})**
- Custom QCustomPlot subclass for ECG signal visualization
- Implements the core peak detection algorithm (`peakdet()`) based on Eli Billauer's algorithm
- Manages peak visualization as QCPItemStraightLine objects stored in a QLinkedList
- Handles interactive peak editing (insert via double-click, delete via selection)
- Supports both local and global threshold modes for R wave detection
- Provides draggable global threshold line for real-time adjustment
- Stores ECG data vectors (ecg_x for time, ecg_y for voltage)

**IBIPlot (src/ibiplot.{h,cpp})**
- Displays inter-beat interval (IBI) sequence as scatter plot
- Computes IBIs from peak positions (`computeInterbeatIntervals()`)
- Implements artifact detection algorithm to identify anomalous intervals
- Interactive selection with tracer for examining specific intervals
- Signals MainWindow to highlight corresponding ECG regions
- Supports "insert missing peaks" workflow for correcting artifacts

**SettingsDialog (src/settingsdialog.{h,cpp,ui})**
- Application settings dialog accessible from Edit → Settings menu
- Configures IBI export options (include signal start and end timestamps)
- Settings persisted to peakman.ini via QSettings
- Can be extended with additional configuration options in the future

### Signal Flow Pattern

The application uses Qt's signal/slot mechanism extensively:

1. **File Loading**: MainWindow → openEcgFile() → ECGPlot::plot()
2. **Peak Detection**: MainWindow::peakDetection() → ECGPlot::peakdet() → emits peaksChanged()
3. **Peak Changes**: ECGPlot::peaksChanged() → MainWindow::setupIbiPlot() → IBIPlot::setup()
4. **Selection Feedback**: IBIPlot selection → MainWindow::jumpToSelection() → ECGPlot::showIbiHighlightRect()
5. **Export**: MainWindow::exportData() → exportPeaksToFile() + exportIbiToFile() → writes both files

### Data Flow

- ECG signal loaded as QVector<double> pairs (time, voltage)
- Peaks stored as QLinkedList<QCPItemStraightLine*> in ECGPlot
- IBIs calculated from peak time differences
- All plots maintain their own data copies but derive from peaks

### Key Algorithms

**Peak Detection (ecgplot.cpp:peakdet())**
- Implements local maxima detection with dual thresholding
- Local threshold: adaptive minimum amplitude for peak candidates
- Global threshold: absolute voltage cutoff across entire signal
- Minimum RR interval constraint prevents false double-detection
- Results stored as vertical lines at detected time points

**Artifact Detection (ibiplot.cpp:artifactDetection())**
- Statistical analysis of IBI sequence
- Flags intervals that deviate from expected patterns
- Visualized as separate data series in IBI plot

## File Formats

The application supports:
- **ECG files**: Text files with raw voltage samples
- **Peaks files**: Saved peak positions (time points in seconds)
- **IBI files**: Inter-beat interval data (milliseconds), optionally including signal start/end timestamps

**Export System:**
- Unified export via File → Export Peaks and Interbeat Intervals
- Single file dialog prompts for base filename
- Automatically creates two files: `{base}_peaks.txt` and `{base}_ibi.txt`
- **IBI Export Behavior:**
  - Controlled by "Include signal start and end when saving IBI" setting in Settings dialog (Edit → Settings)
  - When enabled (default): exported IBI file includes initial interval (signal start to first peak) and final interval (last peak to signal end)
  - When disabled: only the inter-peak intervals are exported
- Export status messages indicate success or partial failures

File loading is handled through OpenFileDialog which prompts for sample rate when loading raw ECG data.

## UI Interaction Patterns

- **ECG Plot**: Double-click to insert peaks, select and delete to remove
- **IBI Plot**: Click to select interval, double-click to jump to ECG location, press 'i' key to insert missing peaks
- **Global Threshold**: Drag horizontal line in ECG plot or use spinbox
- **Settings**: Access via Edit → Settings menu to configure application preferences
- Rubber band selection in ECG plot for multi-peak operations
- Drag/drop files from file explorer

## Menu Structure

- **File**: Open File, Close current file, Export Peaks and Interbeat Intervals, Quit
- **Edit**: Settings
- **Help**: Instructions, About PeakMan

## Dependencies

- **QCustomPlot** (src/qcustomplot.{h,cpp}): Third-party plotting library embedded in source
- All other code is custom implementation for ECG analysis

## Code Organization Notes

- The codebase contains TODO comments indicating ongoing refactoring to move functionality from MainWindow into specialized plot classes
- **Export Implementation** (mainwindow.cpp):
  - `exportData()`: Main export handler that prompts for base filename and coordinates export
  - `exportPeaksToFile()`: Helper method to write peaks file (time values in seconds)
  - `exportIbiToFile()`: Helper method to write IBI file (intervals in milliseconds, respects includeStartEnd setting)
- Settings are persisted using QSettings to `peakman.ini` in the current working directory
  - Window size and position
  - Peak detection algorithm parameters (local threshold, global threshold, minimum RR interval)
  - IBI export preferences (include signal start/end)
  - Sample rate
- The application uses Qt's UI forms (.ui files) for layout, edited in Qt Designer
