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
#define MP_MAX_PACKET_LENGTH    64

class MPCmd: public QObject
{
    Q_OBJECT
public:

    enum Command
    {
        EXPORT_FLASH_START    = 0x8A,
        EXPORT_FLASH          = 0x8B,
        EXPORT_FLASH_END      = 0x8C,
        IMPORT_FLASH_BEGIN    = 0x8D,
        IMPORT_FLASH          = 0x8E,
        IMPORT_FLASH_END      = 0x8F,
        EXPORT_EEPROM_START   = 0x90,
        EXPORT_EEPROM         = 0x91,
        EXPORT_EEPROM_END     = 0x92,
        IMPORT_EEPROM_BEGIN   = 0x93,
        IMPORT_EEPROM         = 0x94,
        IMPORT_EEPROM_END     = 0x95,
        ERASE_EEPROM          = 0x96,
        ERASE_FLASH           = 0x97,
        ERASE_SMC             = 0x98,
        DRAW_BITMAP           = 0x99,
        SET_FONT              = 0x9A,
        USB_KEYBOARD_PRESS    = 0x9B,
        STACK_FREE            = 0x9C,
        CLONE_SMARTCARD       = 0x9D,
        DEBUG                 = 0xA0,
        PING                  = 0xA1,
        VERSION               = 0xA2,
        CONTEXT               = 0xA3,
        GET_LOGIN             = 0xA4,
        GET_PASSWORD          = 0xA5,
        SET_LOGIN             = 0xA6,
        SET_PASSWORD          = 0xA7,
        CHECK_PASSWORD        = 0xA8,
        ADD_CONTEXT           = 0xA9,
        SET_BOOTLOADER_PWD    = 0xAA,
        JUMP_TO_BOOTLOADER    = 0xAB,
        GET_RANDOM_NUMBER     = 0xAC,
        START_MEMORYMGMT      = 0xAD,
        IMPORT_MEDIA_START    = 0xAE,
        IMPORT_MEDIA          = 0xAF,
        IMPORT_MEDIA_END      = 0xB0,
        SET_MOOLTIPASS_PARM   = 0xB1,
        GET_MOOLTIPASS_PARM   = 0xB2,
        RESET_CARD            = 0xB3,
        READ_CARD_LOGIN       = 0xB4,
        READ_CARD_PASS        = 0xB5,
        SET_CARD_LOGIN        = 0xB6,
        SET_CARD_PASS         = 0xB7,
        ADD_UNKNOWN_CARD      = 0xB8,
        MOOLTIPASS_STATUS     = 0xB9,
        FUNCTIONAL_TEST_RES   = 0xBA,
        SET_DATE              = 0xBB,
        SET_UID               = 0xBC,
        GET_UID               = 0xBD,
        SET_DATA_SERVICE      = 0xBE,
        ADD_DATA_SERVICE      = 0xBF,
        WRITE_32B_IN_DN       = 0xC0,
        READ_32B_IN_DN        = 0xC1,
        GET_CUR_CARD_CPZ      = 0xC2,
        CANCEL_USER_REQUEST   = 0xC3,
        PLEASE_RETRY          = 0xC4,
        READ_FLASH_NODE       = 0xC5,
        WRITE_FLASH_NODE      = 0xC6,
        GET_FAVORITE          = 0xC7,
        SET_FAVORITE          = 0xC8,
        GET_STARTING_PARENT   = 0xC9,
        SET_STARTING_PARENT   = 0xCA,
        GET_CTRVALUE          = 0xCB,
        SET_CTRVALUE          = 0xCC,
        ADD_CARD_CPZ_CTR      = 0xCD,
        GET_CARD_CPZ_CTR      = 0xCE,
        CARD_CPZ_CTR_PACKET   = 0xCF,
        GET_30_FREE_SLOTS     = 0xD0,
        GET_DN_START_PARENT   = 0xD1,
        SET_DN_START_PARENT   = 0xD2,
        END_MEMORYMGMT        = 0xD3,
        SET_USER_CHANGE_NB    = 0xD4,
        GET_DESCRIPTION       = 0xD5,
        GET_USER_CHANGE_NB    = 0xD6,
        SET_DESCRIPTION       = 0xD8,
        LOCK_DEVICE           = 0xD9,
        GET_SERIAL            = 0xDA,
    };
    Q_ENUM(Command)

    static Command from(char c);
    static bool isUserRequired(Command c);
    static QString toHexString(Command c);
    static QString printCmd(const QByteArray &ba);
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
    };
    Q_ENUM(Param)
};

#endif // MOOLTIPASSCMDS_H

