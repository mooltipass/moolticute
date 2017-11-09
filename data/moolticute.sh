#!/bin/bash
##
# Wrapper for the moolticute AppImage
##
me="Moolticute.AppImage"
UDEV_RULES_FILE_PATH="/etc/udev/rules.d/50-mooltipass.rules"

help_msg="Moolticute for linux
Usage:
$me <arg>

Arguments:
-d --daemon         Runs daemon only mode.
-i --install        Install moolticute UDev rules
-c --check          Check moolticute UDev rules
-u --uninstall      Uninstall moolticute UDev rules
-h --help           This help

Example:
Run Moolticute normaly
./$me

Run only the moolticuted daemon
./$me --daemon
";

DAEMON_ONLY=0
INSTALL=0
UNINSTALL=0
CHECK=0


case "$1" in
    -d|--daemon)
        DAEMON_ONLY=1
        shift
        ;;
    -i|--install)
        INSTALL=1
        shift
    ;;
        -c|--check)
        CHECK=1
        shift
    ;;
    -u|--uninstall)
        UNINSTALL=1
        shift
    ;;
    -h|-\?|--help)
        printf "$help_msg"
        exit
    ;;
    -?*)
        printf 'WARN: Unknown option (ignored): %s\n' "$1" >&2
        exit 1
    ;;

esac

if (( $DAEMON_ONLY == 1 )) ;
then
    echo "Running daemon only"
    moolticuted
elif (( $INSTALL == 0 )) && (( $UNINSTALL == 0 )) && (( $CHECK == 0 ));
then
    moolticute
elif (( $INSTALL == 1 ));
then
    echo "Installing moolticute UDEV rules"
    echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="09a0", SYMLINK+="mooltipass" TAG+="uaccess", TAG+="udev-acl"' | sudo tee /etc/udev/rules.d/50-mooltipass.rules
    sudo udevadm control --reload-rules
elif (( $CHECK == 1 ));
then
    if [ -s $UDEV_RULES_FILE_PATH ];
    then
        echo "Moolticute Udev rules are NOT installed."
        exit 1;
    else
        echo "Moolticute Udev rules are installed."
        exit 0;
    fi
elif (( $UNINSTALL == 1 ));
then
    echo "Uninstalling moolticuted UDEV rules"
    sudo rm /etc/udev/rules.d/50-mooltipass.rules
    sudo udevadm control --reload-rules
fi
