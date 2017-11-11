#!/bin/bash
##
# Wrapper for the moolticute AppImage
##

me="Moolticute.AppImage"
UDEV_RULES_FILE_PATH="/etc/udev/rules.d/50-mooltipass.rules"
UDEV_RULE='SUBSYSTEMS=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="09a0", SYMLINK+="mooltipass" TAG+="uaccess", TAG+="udev-acl"'

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

SUDO_MSG="Allow Mooltipass Udev rules to be applied in your system?"

DAEMON_ONLY=0
INSTALL=0
UNINSTALL=0
CHECK=0

case "$1" in
    -d|--daemon)
        DAEMON_ONLY=1
        ;;
    -i|--install)
        INSTALL=1
    ;;
        -c|--check)
        CHECK=1
    ;;
    -u|--uninstall)
        UNINSTALL=1
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

# Check if UDEV rules are set on normal run
if (( $# == 0 )) &&\
    [ ! -s $UDEV_RULES_FILE_PATH ];
then
    
    SUDO_BIN=$( which kdesudo || which gksudo || which sudo)

    INSTALL_COMMAND="echo \"$UDEV_RULE\" | tee $UDEV_RULES_FILE_PATH && udevadm control --reload-rules"
    
    SUDO_ARGS=
    FILENAME=$(basename "$SUDO_BIN")
    case "$FILENAME" in
        'sudo')
            echo "$SUDO_MSG"
            $SUDO_BIN sh -c "$INSTALL_COMMAND"
        ;;
        'gksudo')
            $SUDO_BIN -m "$SUDO_MSG" "sh -c '$INSTALL_COMMAND'"
        ;;
        'kdesudo')
            $SUDO_BIN -i moolticute --comment "$SUDO_MSG" -c "$INSTALL_COMMAND"
        ;;
    esac
fi

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
    echo $UDEV_RULE | sudo tee $UDEV_RULES_FILE_PATH
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
    sudo rm $UDEV_RULES_FILE_PATH
    sudo udevadm control --reload-rules
fi
