/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef MOOLTIPASSCMDS_H
#define MOOLTIPASSCMDS_H

#include <QObject>

//USB packet defines
// Indexes
#define MP_LEN_FIELD_INDEX      0x00
#define MP_CMD_FIELD_INDEX      0x01
#define MP_PAYLOAD_FIELD_INDEX  0x02
// Max lengths
#define MP_MAX_SERVICE_LENGTH   121
#define MP_MAX_LOGIN_LENGTH     63
#define MP_MAX_PASSWORD_LENGTH  32
#define MP_MAX_PACKET_LENGTH    64
#define MP_MAX_DESC_LENGTH      24
#define MP_MAX_PAYLOAD_LENGTH   MP_MAX_PACKET_LENGTH-MP_PAYLOAD_FIELD_INDEX
// Bitmasks
#define MP_UNLOCKING_SCREEN_BITMASK 0x02

class MPCmd: public QObject
{
    Q_OBJECT
public:

    /**
     * @brief The Command enum
     * Generic ids for commands
     * Mapping is created in device message protocols
     */
    enum Command: quint16
        {
            EXPORT_FLASH_START=0,
            EXPORT_FLASH        ,
            EXPORT_FLASH_END    ,
            IMPORT_FLASH_BEGIN  ,
            IMPORT_FLASH        ,
            IMPORT_FLASH_END    ,
            EXPORT_EEPROM_START ,
            EXPORT_EEPROM       ,
            EXPORT_EEPROM_END   ,
            IMPORT_EEPROM_BEGIN ,
            IMPORT_EEPROM       ,
            IMPORT_EEPROM_END   ,
            ERASE_EEPROM        ,
            ERASE_FLASH         ,
            ERASE_SMC           ,
            DRAW_BITMAP         ,
            SET_FONT            ,
            USB_KEYBOARD_PRESS  ,
            STACK_FREE          ,
            CLONE_SMARTCARD     ,
            DEBUG               ,
            PING                ,
            VERSION             ,
            CONTEXT             ,
            GET_LOGIN           ,
            GET_PASSWORD        ,
            SET_LOGIN           ,
            SET_PASSWORD        ,
            CHECK_PASSWORD      ,
            ADD_CONTEXT         ,
            SET_BOOTLOADER_PWD  ,
            JUMP_TO_BOOTLOADER  ,
            GET_RANDOM_NUMBER   ,
            START_MEMORYMGMT    ,
            IMPORT_MEDIA_START  ,
            IMPORT_MEDIA        ,
            IMPORT_MEDIA_END    ,
            SET_MOOLTIPASS_PARM ,
            GET_MOOLTIPASS_PARM ,
            RESET_CARD          ,
            READ_CARD_LOGIN     ,
            READ_CARD_PASS      ,
            SET_CARD_LOGIN      ,
            SET_CARD_PASS       ,
            ADD_UNKNOWN_CARD    ,
            MOOLTIPASS_STATUS   ,
            FUNCTIONAL_TEST_RES ,
            SET_DATE            ,
            SET_UID             ,
            GET_UID             ,
            SET_DATA_SERVICE    ,
            ADD_DATA_SERVICE    ,
            WRITE_DATA_FILE     ,
            READ_DATA_FILE      ,
            MODIFY_DATA_FILE    ,
            CHECK_DATA_FILE     ,
            GET_CUR_CARD_CPZ    ,
            CANCEL_USER_REQUEST ,
            PLEASE_RETRY        ,
            READ_FLASH_NODE     ,
            WRITE_FLASH_NODE    ,
            GET_FAVORITE        ,
            GET_FAVORITES       ,
            SET_FAVORITE        ,
            GET_STARTING_PARENT ,
            SET_STARTING_PARENT ,
            GET_CTRVALUE        ,
            SET_CTRVALUE        ,
            ADD_CARD_CPZ_CTR    ,
            GET_CARD_CPZ_CTR    ,
            CARD_CPZ_CTR_PACKET ,
            GET_FREE_ADDRESSES  ,
            GET_DN_START_PARENT ,
            SET_DN_START_PARENT ,
            END_MEMORYMGMT      ,
            SET_USER_CHANGE_NB  ,
            SET_DATA_CHANGE_NB  ,
            GET_DESCRIPTION     ,
            GET_USER_CHANGE_NB  ,
            SET_DESCRIPTION     ,
            LOCK_DEVICE         ,
            GET_SERIAL          ,
            GET_PLAT_INFO       ,
            STORE_CREDENTIAL    ,
            GET_CREDENTIAL      ,
            GET_DEVICE_SETTINGS ,
            SET_DEVICE_SETTINGS ,
            GET_AVAILABLE_USERS ,
            CHECK_CREDENTIAL    ,
            GET_USER_SETTINGS   ,
            GET_USER_CATEGORIES ,
            SET_USER_CATEGORIES ,
            GET_USER_LANG       ,
            GET_DEVICE_LANG     ,
            GET_KEYB_LAYOUT_ID  ,
            GET_LANG_NUM        ,
            GET_KEYB_LAYOUT_NUM ,
            GET_LANG_DESC       ,
            GET_LAYOUT_DESC     ,
            SET_KEYB_LAYOUT_ID  ,
            SET_USER_LANG       ,
            SET_DEVICE_LANG     ,
            INFORM_LOCKED       ,
            INFORM_UNLOCKED     ,
            SET_NODE_PASSWORD   ,
            STORE_TOTP_CRED     ,
            NIMH_RECONDITION    ,
            START_BUNDLE_UPLOAD ,
            WRITE_256B_TO_FLASH ,
            END_BUNDLE_UPLOAD   ,
            AUTH_CHALLENGE      ,
            CMD_DBG_MESSAGE     ,
            CMD_DBG_OPEN_DISP_BUFFER    ,
            CMD_DBG_SEND_TO_DISP_BUFFER ,
            CMD_DBG_CLOSE_DISP_BUFFER   ,
            CMD_DBG_ERASE_DATA_FLASH    ,
            CMD_DBG_IS_DATA_FLASH_READY ,
            CMD_DBG_DATAFLASH_WRITE_256B,
            CMD_DBG_REBOOT_TO_BOOTLOADER,
            CMD_DBG_GET_ACC_32_SAMPLES  ,
            CMD_DBG_FLASH_AUX_MCU       ,
            CMD_DBG_GET_PLAT_INFO       ,
            CMD_DBG_REINDEX_BUNDLE      ,
            CMD_DBG_UPDATE_MAIN_AUX
        };
        Q_ENUM(Command)

    static Command from(char c);
    static bool isUserRequired(Command c);
    static QString toHexString(Command c);
    static QString toHexString(quint16 c);
    static QString printCmd(const QByteArray &ba);
    static QString printCmd(const Command &cmd);
};

class MPParams: public QObject
{
    Q_OBJECT
public:

    enum Param
    {
        USER_PARAM_INIT_KEY_PARAM = 0,
        KEYBOARD_LAYOUT_PARAM,
        USER_INTER_TIMEOUT_PARAM,
        LOCK_TIMEOUT_ENABLE_PARAM,
        LOCK_TIMEOUT_PARAM,
        TOUCH_DI_PARAM,
        TOUCH_WHEEL_OS_PARAM_OLD,
        TOUCH_PROX_OS_PARAM,
        OFFLINE_MODE_PARAM,
        SCREENSAVER_PARAM,
        TOUCH_CHARGE_TIME_PARAM,
        TOUCH_WHEEL_OS_PARAM0,
        TOUCH_WHEEL_OS_PARAM1,
        TOUCH_WHEEL_OS_PARAM2,
        FLASH_SCREEN_PARAM,
        USER_REQ_CANCEL_PARAM,
        TUTORIAL_BOOL_PARAM,
        SCREEN_SAVER_SPEED_PARAM,
        LUT_BOOT_POPULATING_PARAM,
        KEY_AFTER_LOGIN_SEND_BOOL_PARAM,
        KEY_AFTER_LOGIN_SEND_PARAM,
        KEY_AFTER_PASS_SEND_BOOL_PARAM,
        KEY_AFTER_PASS_SEND_PARAM,
        DELAY_AFTER_KEY_ENTRY_BOOL_PARAM,
        DELAY_AFTER_KEY_ENTRY_PARAM,
        INVERTED_SCREEN_AT_BOOT_PARAM,
        MINI_OLED_CONTRAST_CURRENT_PARAM,
        MINI_LED_ANIM_MASK_PARAM,
        MINI_KNOCK_DETECT_ENABLE_PARAM,
        MINI_KNOCK_THRES_PARAM,
        LOCK_UNLOCK_FEATURE_PARAM,
        HASH_DISPLAY_FEATURE_PARAM,
        RANDOM_INIT_PIN_PARAM,
        RESERVED_BLE,
        PROMPT_ANIMATION_PARAM,
        BOOT_ANIMATION_PARAM,
        DEVICE_LANGUAGE,
        USER_LANGUAGE,
        KEYBOARD_USB_LAYOUT,
        KEYBOARD_BT_LAYOUT,
        DEVICE_LOCK_USB_DISC,
        PIN_SHOWN_ON_BACK,
        PIN_SHOW_ON_ENTRY,
        DISABLE_BLE_ON_CARD_REMOVE,
        DISABLE_BLE_ON_LOCK,
        NB_20MINS_TICKS_FOR_LOCK,
        SWITCH_OFF_AFTER_USB_DISC,
        INFORMATION_TIME_DELAY,
        BLUETOOTH_SHORTCUTS,
        SCREEN_SAVER_ID
    };
    Q_ENUM(Param)
};

#endif // MOOLTIPASSCMDS_H

