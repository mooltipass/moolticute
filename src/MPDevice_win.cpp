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
#include "MPDevice_win.h"
#include "HIDLoader.h"

// Windows GUID object
static GUID IClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30} };
const QString MPDevice_win::BT_GATT_SERVICE_GUID = "00001812-0000-1000-8000-00805f9b34fb";

MPDevice_win::MPDevice_win(QObject *parent, const MPPlatformDef &p):
    MPDevice(parent),
    platformDef(std::move(p))
{
    HID.load();

    oNotifier = new QWinOverlappedIoNotifier(this);
    connect(oNotifier, SIGNAL(notified(quint32,quint32,OVERLAPPED*)),
            this, SLOT(ovlpNotified(quint32,quint32,OVERLAPPED*)));
    connect(this, SIGNAL(platformWriteFinished()), this, SLOT(writeDataFinished()));

    if (p.isBLE)
    {
        deviceType = DeviceType::BLE;
#if defined(Q_OS_WIN)
        isBluetooth = platformDef.isBluetooth;
#endif
    }
    setupMessageProtocol();

    if (!openPath())
    {
        qWarning() << "Error opening device";
        QTimer::singleShot(50, [this]()
        {
            if (openPath())
            {
                platformRead();
                sendInitMessages();
            }
            else
            {
                qCritical() << "Device open failed after retry.";
            }
        });
    }
    else
    {
        platformRead();
        sendInitMessages();
    }
}

MPDevice_win::~MPDevice_win()
{
    oNotifier->setEnabled(false);
    delete oNotifier;
    CancelIo(platformDef.devHandle);
    CloseHandle(platformDef.devHandle);
}

QString MPDevice_win::getLastError(DWORD err)
{
    LPWSTR bufPtr = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   err,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR)&bufPtr,
                   0,
                   NULL);
    const QString result =
            (bufPtr) ? QString::fromUtf16((const ushort*)bufPtr).trimmed() :
                       QString("Unknown Error %1").arg(err);
    LocalFree(bufPtr);
    return result;
}

HANDLE MPDevice_win::openDevice(QString path, bool exlusive /* =false */)
{
    DWORD sharedOptRW = exlusive ? 0 : FILE_SHARE_READ | FILE_SHARE_WRITE;
    HANDLE h = CreateFileA(qToChar(path),
                           GENERIC_WRITE | GENERIC_READ,
                           sharedOptRW,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED,
                           nullptr);

    if (GetLastError() == ERROR_ACCESS_DENIED)
        h = CreateFileA(qToChar(path),
                        GENERIC_READ,
                        sharedOptRW & FILE_SHARE_READ,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED,
                        nullptr);

    return h;
}

bool MPDevice_win::openPath()
{
    platformDef.devHandle = openDevice(platformDef.path, true);

    if (platformDef.devHandle == INVALID_HANDLE_VALUE)
    {
        qWarning() << "Failed to open device path " << platformDef.path;
        qWarning() << getLastError(GetLastError());
        return false;
    }

    //Get input/output report length
    PHIDP_PREPARSED_DATA pdata = nullptr;
    bool ret = HID.HidD_GetPreparsedData(platformDef.devHandle, &pdata);
    if (!ret)
    {
        qWarning() << "Failed to read reports info for " << platformDef.path;
        qWarning() << getLastError(GetLastError());
        return false;
    }

    HIDP_CAPS caps;
    NTSTATUS nret = HID.HidP_GetCaps(pdata, &caps);
    if (nret != HIDP_STATUS_SUCCESS)
    {
        qWarning() << "Failed to read caps info for " << platformDef.path;
        qWarning() << getLastError(GetLastError());
        HID.HidD_FreePreparsedData(pdata);
        return false;
    }

    platformDef.inReportLen = caps.InputReportByteLength;
    platformDef.outReportLen = caps.OutputReportByteLength;

    HID.HidD_FreePreparsedData(pdata);

    oNotifier->setHandle(platformDef.devHandle);
    oNotifier->setEnabled(true);

    return true;
}

void MPDevice_win::platformWrite(const QByteArray &data)
{

    if (m_writeQueue.isEmpty())
    {
        platformWriteToDevice(data);
    }
    m_writeQueue.append(data);
}

void MPDevice_win::platformWriteToDevice(const QByteArray &data)
{
    char reportByte = ZERO_BYTE;
    QByteArray ba;
    if (isBluetooth)
    {
        reportByte = static_cast<char>(0x03);
    }
    ba.append(reportByte); //add report id (even if not used). windows requires that
    ba.append(data);

    //resize array to fit windows requirements (at least outReportLen size)
    if (ba.size() < platformDef.outReportLen)
    {
        Common::fill(ba, platformDef.outReportLen - ba.size(), ZERO_BYTE);
    }

    ::ZeroMemory(&writeOverlapped, sizeof(writeOverlapped));

    bool ret = WriteFile(platformDef.devHandle,
                         ba.constData(),
                         ba.size(),
                         nullptr,
                         &writeOverlapped);

    int err = GetLastError();

    if (!ret &&
        err != ERROR_IO_PENDING &&
        err != ERROR_SUCCESS)
    {
        qWarning() << "Failed to write data to device! " << err;
        qWarning() << getLastError(err);
    }
}

void MPDevice_win::platformRead()
{
    readBuffer.clear();
    readBuffer.resize(platformDef.inReportLen);
    ::ZeroMemory(&readOverlapped, sizeof(readOverlapped));

    bool ret = ReadFile(platformDef.devHandle,
                        readBuffer.data(),
                        readBuffer.size(),
                        nullptr,
                        &readOverlapped);

    int err = GetLastError();
    if (!ret && err != ERROR_IO_PENDING)
    {
        qWarning() << "Failed to read data from device!";
        qWarning() << getLastError(err);
    }
}

QList<MPPlatformDef> MPDevice_win::enumerateDevices()
{
    HID.load();

    QList<MPPlatformDef> devlist;

    SP_DEVINFO_DATA devinfo_data;
    SP_DEVICE_INTERFACE_DATA dev_data;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *dev_detail_data = NULL;
    HDEVINFO dev_info_set = INVALID_HANDLE_VALUE;
    int idx = 0;

    ::ZeroMemory(&devinfo_data, sizeof(devinfo_data));
    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
    dev_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    //Get all devices from HID class
    dev_info_set = SetupDiGetClassDevsA(&IClassGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    while (SetupDiEnumDeviceInterfaces(dev_info_set,
                                       nullptr,
                                       &IClassGuid,
                                       idx,
                                       &dev_data))
    {
        DWORD required_size = 0;

        //first call is to get the required_size
        bool ret = SetupDiGetDeviceInterfaceDetailA(dev_info_set,
                                                    &dev_data,
                                                    nullptr,
                                                    0,
                                                    &required_size,
                                                    nullptr);

        if (0 == required_size)
        {
            qCritical() << "Invalid DeviceInterfaceDetailData buffer size";
            continue;
        }

        //alloc data
        dev_detail_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*) malloc(required_size);
        if (!dev_detail_data)
        {
            qCritical() << "Allocating SP_DEVICE_INTERFACE_DETAIL_DATA_A data failed.";
            break;
        }
        dev_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        //Get device info now
        ret = SetupDiGetDeviceInterfaceDetailA(dev_info_set,
                                               &dev_data,
                                               dev_detail_data,
                                               required_size,
                                               nullptr,
                                               nullptr);

        ++idx;
        if (!ret)
        {
            free(dev_detail_data);
            continue;
        }

        QString path = QString(dev_detail_data->DevicePath);
        free(dev_detail_data);

        bool isBLE = false;
        bool isBluetooth = false;
        if (!checkDevice(path, isBLE, isBluetooth))
        {
            continue;
        }
        qDebug() << "Found mooltipass: " << path;

        //TODO: extract interface number from path string and check it

        devlist << getPlatDef(path, isBLE, isBluetooth);
    }

    SetupDiDestroyDeviceInfoList(dev_info_set);

    return devlist;
}

bool MPDevice_win::checkDevice(QString path, bool &isBLE /* out */, bool &isBluetooth /* out */)
{
    HIDD_ATTRIBUTES attrib;
    //Get vendorid/productid
    attrib.Size = sizeof(HIDD_ATTRIBUTES);
    HANDLE h = openDevice(path);
    if (h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    HID.HidD_GetAttributes(h, &attrib);
    CloseHandle(h);

    if ((attrib.VendorID != MOOLTIPASS_VENDORID && attrib.VendorID != MOOLTIPASS_BLE_VENDORID) ||
        (attrib.ProductID != MOOLTIPASS_PRODUCTID && attrib.ProductID != MOOLTIPASS_BLE_PRODUCTID))
    {
        return false;
    }

    isBLE = attrib.VendorID == MOOLTIPASS_BLE_VENDORID;
    if (isBLE && path.contains(BT_GATT_SERVICE_GUID))
    {
        qDebug() << "BT HID connected";
        isBluetooth = true;
    }
    return true;
}

MPPlatformDef MPDevice_win::getPlatDef(QString path, bool isBLE, bool isBluetooth)
{
    MPPlatformDef def;
    def.path = path;
    def.id = path; //use path for ID
    def.isBLE = isBLE;
    def.isBluetooth = isBluetooth;
    return def;
}

void MPDevice_win::ovlpNotified(quint32 numberOfBytes, quint32 errorCode, OVERLAPPED *overlapped)
{
    //this slots is triggered when an I/O operation happend on our device
    //we can then compare which OVERLAPPED this operation is

    bool fail = true;
    if (errorCode == ERROR_SUCCESS ||
        errorCode == ERROR_MORE_DATA ||
        errorCode == ERROR_IO_PENDING)
        fail = false;

    if (overlapped == &writeOverlapped) //write op
    {
        if (fail)
        {
            qWarning() << "Failed to write data to device!";
            qWarning() << getLastError(errorCode);
            return;
        }

        emit platformWriteFinished();
    }
    else if (overlapped == &readOverlapped) //read op
    {
        if (fail)
        {
            qWarning() << "Failed to read data from device!";
            qWarning() << getLastError(errorCode);
            return;
        }

        readBuffer.resize(numberOfBytes);
        emit platformDataRead(readBuffer.right(readBuffer.length() - 1));

        platformRead();
    }
}

void MPDevice_win::writeDataFinished()
{
    if (m_writeQueue.isEmpty())
    {
        // WriteQueue is empty, platformWrite was called directly
        return;
    }
    m_writeQueue.dequeue();
    if (!m_writeQueue.isEmpty())
    {
        // If there are more data to write in the queue, write the next one
        platformWriteToDevice(m_writeQueue.head());
    }
}
