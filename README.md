Moolticute
==========

[![License](https://img.shields.io/badge/license-GPLv3%2B-blue.svg)](http://www.gnu.org/licenses/gpl.html)

[![Build Status](https://travis-ci.org/raoulh/moolticute.svg?branch=master)](https://travis-ci.org/raoulh/moolticute)

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

### Downloads
Packages are build and available here: https://calaos.fr/mooltipass/

### Dependencies

##### Windows, Linux, OSX
 - Requires Qt 5.6 or higher.
 - Those Qt5 modules are required:
  - qt-base, qt-widgets, qt-gui, qt-network, qt-websockets

##### Linux
 - Requires libusb
 - Requires a [udev rule for libusb](https://github.com/bobsaintcool/mooltipass-udev)

##### Ubuntu 16.04
```bash
sudo apt install libqt5websockets5-dev libusb-dev libusb-1.0-0-dev qt-sdk qt5-qmake qt5-default
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="09a0", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/50-mooltipass.rules
sudo udevadm control --reload-rules
```

##### Arch Linux
```bash
sudo pacman -S --needed qt5-websockets libusb qt5-base
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="09a0", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/50-mooltipass.rules
sudo udevadm control --reload-rules
```

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
qmake ../Moolticute.pro
make
```

Be careful to use the Qt5 qmake. You can check if you are using the correct qmake by using the command
```
➜  ~  qmake --version
QMake version 3.0
Using Qt version 5.5.1 in /usr/lib
```

On Gentoo, a wrapper is created for qmake, so this command should be used:
```
qmake -qt=5 ../Moolticute.pro
```

### Licensing

Moolticute is free software; you can redistribute it and/or modify it under the terms of the GNU Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.


