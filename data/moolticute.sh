#!/bin/bash

me="Moolticute.AppImage"
help_msg="Moolticute for linux
Usage:
$me <arg>

Arguments:
-d --daemon        Runs daemon only mode.
-i --install       Install moolticute as a systemd service
-u --uninstall     Uninstall moolticute as a systemd service
-h --help          This help

Example:
Run Moolticute normaly
./$me

Run only the moolticuted daemon
./$me --daemon
";

DAEMON_ONLY=0
INSTALL=0
UNINSTALL=0


case "$1" in
    -d|--daemon)
        DAEMON_ONLY=1
        shift
        ;;
    -i|--install)
        INSTALL=1
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
elif (( $INSTALL == 0 )) && (( $UNINSTALL == 0 ));
then
    moolticute
elif (( $INSTALL == 1 ));
then
    echo "Installing moolticuted as service"
elif (( $UNINSTALL == 1 ));
then
    echo "Uninstalling moolticuted as service"
fi
