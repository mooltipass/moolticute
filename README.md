Moolticute
==========

[![License](https://img.shields.io/badge/license-GPLv3%2B-blue.svg)](http://www.gnu.org/licenses/gpl.html)

[![Build Status](https://travis-ci.org/mooltipass/moolticute.svg?branch=master)](https://travis-ci.org/mooltipass/moolticute)

This project aims to be an easy to use companion to your [Mooltipass](http://www.themooltipass.com) device and extend
the power of the device to more platform/tools.
With it you can manage your Mooltipass with a cross-platform app, as well as provide a daemon service that
handles all USB communication with the device.

This tool comes with a daemon that runs in background, and a user interface app to control your Mooltipass.
Other clients can also connect and talk to the daemon (it uses a websocket connection and simple JSON messages).

It is completely cross platform, and runs on Linux (using native hidraw API), OS X (native IOKit API), and Windows (native HID API).

### Downloads
Packages are build and available here: https://github.com/mooltipass/moolticute/releases

### Dependencies

##### Windows, Linux, OSX
 - Requires Qt 5.6 or higher.
 - These Qt5 modules are required:
   - qt-core
   - qt-gui
   - qt-widgets
   - qt-network
   - qt-websockets

##### Linux
 - Requires the qt-dbus module
 - Requires to install [udev rule](https://github.com/mooltipass/mooltipass-udev) for it
   - Note: For Linux system without systemd (eg: Void Linux) please
     refer to [dedicated section](#non-systemd-linux).

Moolticute comes in two part on Linux:
  - A daemon that runs in background named moolticuted that proxy
    command between device and GUI
  - A GUI application for user to interact with the device using moolticuted

One way of using Moolticute is allowing systemd to manage the daemon
as a system service and your desktop environment execing the GUI when
you need it. In such setup it is best to disable the option found in
the GUI called "Start Moolticute with the computer"
The default build recipe will drop a systemd system unit
`moolticuted.service`, after build and installation:

```bash
systemctl daemon-reload
systemctl enable --now moolticuted.service
```

At this point the Moolticute daemon should be running. As the build
also drop an `moolticute.desktop` you should be able to launch the GUI
using your environment application launcher and searching for "moolticute".

##### Ubuntu 16.04
```bash
sudo apt install libqt5websockets5-dev qt-sdk qt5-qmake qt5-default libudev-dev
curl https://raw.githubusercontent.com/mooltipass/mooltipass-udev/master/udev/69-mooltipass.rules | sudo tee /etc/udev/rules.d/69-mooltipass.rules
sudo udevadm control --reload-rules
```

##### Arch Linux

There are packages in AUR both for udev rules and for the Moolticute:
  - [udev rules](https://aur.archlinux.org/packages/mooltipass-udev/)
  - [Moolticute](https://aur.archlinux.org/packages/moolticute/)

##### Fedora Linux
```bash
sudo dnf install gcc-c++ qt5 qt5-qtwebsockets qt5-qtwebsockets-devel qt5-qttools-devel systemd-devel
curl https://raw.githubusercontent.com/mooltipass/mooltipass-udev/master/udev/69-mooltipass.rules | sudo tee /etc/udev/rules.d/69-mooltipass.rules
sudo udevadm control --reload-rules
```

##### Fedora Linux (33 and >)
```bash
sudo dnf install gcc-c++ qt5-qtbase qt5-qtwebsockets qt5-qtwebsockets-devel qt5-qttools-devel systemd-devel
curl https://raw.githubusercontent.com/mooltipass/mooltipass-udev/master/udev/69-mooltipass.rules | sudo tee /etc/udev/rules.d/69-mooltipass.rules
sudo udevadm control --reload-rules
```

##### NixOS
Udev rules are lifted from the package if [this](https://github.com/NixOS/nixpkgs/pull/154355) is merged, otherwise paste them in as `extraRules`.
```
environment.systemPackages = with pkgs; [
  ...
  moolticute
  ];
};

services.udev.packages = [ pkgs.moolticute.udev ]; # if linked PR is merged
services.udev.extraRules = '' # if not slip rules in manually
  <the udev rules from data/moolticute.sh>
            '';
```

##### Non systemd Linux
Main [udev rules](https://aur.archlinux.org/packages/mooltipass-udev/)
relies on integration between systemd, systemd-login and systemd-udevd
to automatically allow user logged in to access mooltipass devices.
This will not works on Linux ecosystem without systemd and one
workaround might be to change the `TAG+="uaccess"` to
`GROUP="plugdev"` and add your interactive user to this group.

So please apply the following patch to the official udev rules:
```
TODO PATCH
```
Or use home made pkg recipe from PoroCYon. TODO add this

Refer to this issue if needed https://github.com/mooltipass/mooltipass-udev/pull/3



### How to build

For now, no binary releases are out yet. You will need to build the software by following the next step.

Two method can be used to build, by using QtCreator IDE, or from command line (typically on Linux). After the build succeeded, two executable are created:
 - moolticuted (the daemon process)
 - moolticute (the main GUI app)

##### Using Qt SDK

 - Download and install the Qt SDK from their [website](http://qt.io)
 - Start Qt-Creator
 - Open the main Moolticute.pro project file
    - Note for Windows users: make sure that you select a "kit" that uses the MinGW compiler.
      Moolticute currently won't compile successfully when using the Microsoft Visual C++ compiler.
 - Click on the "play" button to build and run

##### Command line

Qt needs to be installed correctly (see your Linux distribution for that)

```
mkdir build
cd build
qmake ../Moolticute.pro
make
```

Be sure to use the Qt5 qmake. You can check if you are using the correct qmake by using the command
```
➜  ~  qmake --version
QMake version 3.0
Using Qt version 5.5.1 in /usr/lib
```

On Gentoo, a wrapper is created for qmake, so this command should be used:
```
qmake -qt=5 ../Moolticute.pro
```

On Fedora, use this qmake command:
```
qmake-qt5 ../Moolticute.pro
```

### Licensing

Moolticute is free software: you can redistribute it and/or modify it under the terms of the GNU Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.
