#!/bin/bash
##
# Wrapper for the moolticute AppImage
##

me="Moolticute.AppImage"

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

UDEV_RULES_FILE_PATH="/etc/udev/rules.d/50-mooltipass.rules"

function install_udev_rule()
{
    SUDO_BIN="sudo"
    which kdesudo > /dev/null 2>&1 && SUDO_BIN="kdesudo"
    which kdesu > /dev/null 2>&1 && SUDO_BIN="kdesu"
    which gksudo > /dev/null 2>&1 && SUDO_BIN="gksudo"

    tmpfile=$(mktemp /tmp/mc-udev.XXXXXX)
    cat > "$tmpfile" <<- EOF
# udev rules for allowing console user(s) and libusb access to Mooltipass Mini devices

ACTION!="add|change", GOTO="mooltipass_end"

# console user
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="09a0", MODE="0660", SYMLINK+="mooltipass_keyboard", TAG+="uaccess"
# libusb
SUBSYSTEM=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="09a0", MODE="0660", SYMLINK+="mooltipass_device", TAG+="uaccess"

LABEL="mooltipass_end"
EOF

    chmod +r $tmpfile
    SUDO_MSG="Allow Mooltipass Udev rules to be applied in your system?"
    INSTALL_COMMAND="cp $tmpfile $UDEV_RULES_FILE_PATH && udevadm control --reload-rules"

    FILENAME=$(basename "$SUDO_BIN")
    case "$FILENAME" in
        'sudo')
            echo "$SUDO_MSG"
            $SUDO_BIN sh -c "$INSTALL_COMMAND"
        ;;
        'gksudo')
            LD_LIBRARY_PATH="" $SUDO_BIN -m "$SUDO_MSG" "sh -c '$INSTALL_COMMAND'"
        ;;
        'kdesudo')
            LD_LIBRARY_PATH="" $SUDO_BIN -i moolticute --comment "$SUDO_MSG" -c "$INSTALL_COMMAND"
        ;;
        'kdesu')
            LD_LIBRARY_PATH="" $SUDO_BIN -c "$INSTALL_COMMAND" --noignorebutton
        ;;
    esac

    rm "$tmpfile"  
}

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
    install_udev_rule
fi

#fix https://github.com/mooltipass/moolticute/issues/245
export QT_XKB_CONFIG_ROOT=/usr/share/X11/xkb

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
    install_udev_rule
elif (( $CHECK == 1 ));
then
    if [ -s $UDEV_RULES_FILE_PATH ]
    then
        echo "Moolticute Udev rules are installed."
        exit 0;
    else
        echo "Moolticute Udev rules are NOT installed."
        exit 1;
    fi
elif (( $UNINSTALL == 1 ));
then
    echo "Uninstalling moolticuted UDEV rules"
    sudo rm $UDEV_RULES_FILE_PATH
    sudo udevadm control --reload-rules
fi
