# PeakMan

PeakMan is an easy-to-use and fast peak detection and artifact processing tool for electrocardiogram data.

It builds upon a [peak detection algorithm](http://www.billauer.co.il/peakdet.html) by Eli Billauer and features local and global thresholding for R waves.

The application is written in C++ using the Qt framework and runs on all major platforms (Windows, Mac OS X and Linux). [QCustomPlot](http://qcustomplot.com/) is used for data visualization.

## Building from source

If you want to build PeakMan from source you need Qt 5.x ([qt.io](http://www.qt.io/download/)). On Windows, be sure to install gcc together with Qt. On Mac OS X you need to have Xcode installed. The easiest way then is to open the `peakman.pro` file in the `src` folder with Qt Creator and compile PeakMan from there.
