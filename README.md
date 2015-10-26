Moolticute
==========

[![License](https://img.shields.io/badge/license-GPLv3%2B-blue.svg)](http://www.gnu.org/licenses/gpl.html)

This project aims to be an easy companion to your [Mooltipass](http://www.themooltipass.com) device and extend
the power of the device to more platform/tools.
With it you can manage your Mooltipass with a crossplatform app, as well as provide a daemon service that
handle all USB communication with the device.

This tool is written with a daemon that runs in background, and a user interface app to control you Mooltipass.
Other clients could also connect and talk to the daemon (it uses a websocket connection and simple JSON messages).
The official Mooltipass App only works with Chrome as it relies on USB HID library that is only implemented in Chrome.
A Firefox (or any other browser) extension could easily be written by using the Moolticute daemon.

It is completely cross platform, and runs on Linux (using libusb), OS X (native IOKit API), and Windows (native HID API).

> Warning! This project is a work in progress!

### Dependencies

##### Windows, Linux, OSX
 - Requires Qt 5.4 or higher.

##### Linux
 - Requires libusb

### How to build

For now, no binary releases are out yet. You will need to build the software by following the next step.

Two method can be used to build, by using QtCreator IDE, or from command line (typically on Linux). After the build succeeded, two executable are created:
 - moolticuted (the daemon process)
 - MoolticuteApp (the main GUI app)

##### Using Qt SDK

 - Download and install the Qt SDK from their [website](http://qt.io)
 - Start Qt-Creator
 - Open the main Moolticute.pro project file
 - Click on the "play" button to build and run

##### Command line

Qt needs to be installed correctly (see you Linux distribution for that)

```
mkdir build
cd build
qmake
make
```

### Licensing

Moolticute is free software; you can redistribute it and/or modify it under the terms of the GNU Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

