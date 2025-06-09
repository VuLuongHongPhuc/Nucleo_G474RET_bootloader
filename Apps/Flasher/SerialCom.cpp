#include <iostream>
#include <string>
#include <sstream> // stringstream
#include <iomanip> // setw

#include "SerialCom.h"

//#define TTL_COM_PORT L"COM9"
#define TTL_COM_PORT L"\\\\.\\COM16"    // when greater than 10, you need to prefix with "\\\\.\\"
#define DEF_RX_BUFFER_SIZE 80

//static HANDLE m_hUart = INVALID_HANDLE_VALUE;


SerialCom::SerialCom(std::string name) 
    : m_Name(name),
    m_hUart(INVALID_HANDLE_VALUE),
    m_IsOpen(false)
{
    
}


int SerialCom::Open()
{
    // open com port
    m_hUart = CreateFile(TTL_COM_PORT, // drive name to open
        GENERIC_READ | GENERIC_WRITE,       // must be opened with exclusive access(No Sharing)
        0,                                  // no access to the drive
        NULL,                               // default security attributes
        OPEN_EXISTING,                      // disposition i.e. if file already exist
        FILE_ATTRIBUTE_NORMAL,              // file attributes
        NULL);                              // do not copy file attributes

    if (INVALID_HANDLE_VALUE == m_hUart)
    {
        //DWORD error = GetLastError();
        // error
        return -1;
    }
    else
    {
        SetConfig();
        FlushReceiveBuffer();

        m_IsOpen = true;
    }

    return 0;
}

void SerialCom::SetConfig()
{
    DCB dcbSerialParams;
    // set com port settings
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(m_hUart, &dcbSerialParams))
    {
        std::cout << "Could not get COM port settings!\n";
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(m_hUart, &dcbSerialParams))
    {
        std::cout << "Could not set COM port settings!\n";
    }

    COMMTIMEOUTS timeouts;
    // set com port timeouts
    timeouts.ReadIntervalTimeout = MAXDWORD; // MAXDWORD -> return immediately with any bytes available
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    // Total read timeout = (number of bytes to read * ReadTotalTimeoutMultiplier ) + ReadTotalTimeoutConstant

    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    // Total write timeout = (number of bytes to write * WriteTotalTimeoutMultiplier) + WriteTotalTimeoutConstant

    if (!SetCommTimeouts(m_hUart, &timeouts))
    {
        std::cout << "Could not set COM port timeouts!\n";
    }
}

void SerialCom::Write(const uint8_t& c)
{
    uint8_t buf[1] = { 0x55 };
    DWORD bytes_write = 0;

    buf[0] = c;

    if (!WriteFile(m_hUart, buf, 1, &bytes_write, NULL))
    {
        DWORD error = GetLastError();
        std::cout << "ERROR CODE - WRITE : 0x" << std::hex << error << "\n";
    }
    else
    {
        std::cout << "W: 0x" << std::hex << (int)buf[0] << '\n';
    }
}

int32_t SerialCom::Write(const uint8_t* const pBuf, const int lenght)
{
    DWORD bytes_write = 0;

    if (!WriteFile(m_hUart, pBuf, lenght, &bytes_write, NULL))
    {
        DWORD error = GetLastError();
        std::cout << "ERROR CODE - WRITE : 0x" << std::hex << error << "\n";
        return -1;
    }

    return static_cast<int32_t>(bytes_write);
}

void SerialCom::FlushReceiveBuffer()
{
    static int loop_cnt = 0;

    char buf[DEF_RX_BUFFER_SIZE + 1] = { 0 };
    DWORD bytes_read = 0;


    if (!ReadFile(m_hUart, buf, DEF_RX_BUFFER_SIZE, &bytes_read, NULL))
    {
        DWORD error = GetLastError();
        std::cout << "ERROR CODE - READ : 0x" << std::hex << error << "\n";
    }
    else
    {
        if (bytes_read == DEF_RX_BUFFER_SIZE)
        {
            // stop recursif
            if (loop_cnt > 10)
                return;

            loop_cnt++;
            FlushReceiveBuffer();
        }
    }    
}


/**
* @brief Read buffer COMx
* @param[out] pBuf - store receive buffer
* @param[in] lenght - pBuf size
* @return number of data read or -1 if error
*/
int32_t SerialCom::Read(uint8_t * pBuf, uint32_t lenght)
{
    DWORD bytes_read = 0;

    if (!ReadFile(m_hUart, pBuf, lenght, &bytes_read, NULL))
    {
        DWORD error = GetLastError();
        std::cout << "ERROR CODE - READ : 0x" << std::hex << error << "\n";
        return -1;
    }
    else
    {
        std::stringstream ss;
        std::string str1_hex = "R";

        for (int i = 0; i < bytes_read; i++)
        {
            ss << " 0x" << std::setw(2) << std::hex << (int)pBuf[i];
        }

        str1_hex += ss.str();

        std::cout << std::setw(2) << std::dec << bytes_read << ":";
        std::cout << str1_hex << '\n';
    }

    return static_cast<int32_t>(bytes_read);
}


bool SerialCom::CheckReceiveValue(const uint8_t value)
{
    uint8_t buf[8] = { 0 };
    DWORD bytes_read = 0;
    bool ret = false;

    if (!ReadFile(m_hUart, buf, 1, &bytes_read, NULL))
    {
        DWORD error = GetLastError();
        std::cout << "ERROR CODE - READ : 0x" << std::hex << error << "\n";
    }
    else
    {
        if (bytes_read == 1)
        {
            std::cout << "R: 0x" << std::hex << (int)buf[0] << '\n';

            if (value == buf[0])
                ret = true;
        }
    }

    return ret;
}

/*EOF*/
