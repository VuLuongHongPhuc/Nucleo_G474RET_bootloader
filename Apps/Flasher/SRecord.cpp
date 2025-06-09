#include <iostream>
#include <fstream>
#include <array>
#include "SRecord.h"


#include <windows.h> // Sleep

/*
* - SRecordStruct enregistre les data successif
* - new SRecordStruct si adresse discontinue
*/


static uint32_t StringToUInt32(const std::string& hexText)
{
    char* end;

    unsigned long value = std::strtoul(hexText.c_str(), &end, 16); // Convert to long with base 16
    if (*end == '\0')
    { // Check if entire string was converted
        //std::cout << "The decimal value of hex " << hexText << " is: " << value << std::endl;
        //std::cout << "The hex value is: 0x" << std::hex << value << std::endl;
    }
    else
    {
        std::cout << "Invalid hex string: " << hexText << std::endl;
    }

    return static_cast<uint32_t>(value);
}

int atoi(char c)
{
    if (c < 'A')
    {
        return (c - '0');
    }
    else
    {
        return ((c < 'a') ? (c - 'A') : (c - 'a'));
    }
}


int SRecord::ExtractLine(const std::string& line)
{
    if (line[0] != 'S')
    {
        // Error - Type not present
        std::cout << "ERROR SRecord - No type. ";
        std::cout << "Line: " << __LINE__ << "\n";
        return -1;
    }

    if (line.length() % 2)
    {
        std::cout << "ERROR SRecord - Number of char odd. ";
        std::cout << "Line: " << __LINE__ << "\n";
        return -1;
    }

    struct SRecordStruct SRecord;

    SRecord.Type = line[1] - '0';
    if (SRecord.Type > 9)
    {
        std::cout << "ERROR SRecord - Type S. ";
        std::cout << "Line: " << __LINE__ << "\n";
        return -1;
    }

    int ret = 0;

    switch (SRecord.Type)
    {
    case 0:// Header record
        break;

    case 3:// Data record with 32-bit address.
    case 7:// Termination records S3
    {
        std::string strCountHex = line.substr(2, 2);
        uint32_t count = StringToUInt32(strCountHex);
        SRecord.CRC = static_cast<uint8_t>(count);

        std::string remainText = line.substr(4, line.length() - 4);

        ExtractAddress(remainText, SRecord);

        // Extract Data & CRC check
        ExtractData(remainText, SRecord);

        ret = (SRecord.Count == count) ? 0 : -1;

        m_SequenceList.emplace_back(SRecord);
    }break;

    default:
        std::cout << "ERROR SRecord - Type S not for STM32G474\n";
        ret = -1;
        break;
    }

    return ret;
}


int SRecord::ExtractAddress(std::string& line, struct SRecordStruct& SRecord)
{
    int ret = 0;

    switch (SRecord.Type)
    {
    case 0:   // Header record (often contains metadata like file name).
        ret = ExtractAddressExt(line, SRecord, 4);
        break;

    case 1:   // Data record with 16-bit address.
        ret = ExtractAddressExt(line, SRecord, 4);
        break;

    case 2:   // Data record with 24-bit address.
        ret = ExtractAddressExt(line, SRecord, 6);
        break;

    case 3:   // Data record with 32-bit address.
        ret = ExtractAddressExt(line, SRecord, 8);
        break;

    case 5:   // Record count (optional, number of data records). 16-bits count
        ret = ExtractAddressExt(line, SRecord, 4);
        break;

    case 6:   // Record count (optional, number of data records). 24-bits count
        ret = ExtractAddressExt(line, SRecord, 6);
        break;

    case 7:   // Termination records S3 (entry point for execution or end of data).
        ret = ExtractAddressExt(line, SRecord, 8);
        break;

    case 8:   // Termination records S2 (entry point for execution or end of data).
        ret = ExtractAddressExt(line, SRecord, 6);
        break;

    case 9:   // Termination records S1 (entry point for execution or end of data).
        ret = ExtractAddressExt(line, SRecord, 4);
        break;

    default:
        break;
    }

    return ret;
}

int SRecord::ExtractAddressExt(std::string& line, struct SRecordStruct& SRecord, int count)
{
    if (line.length() < count)
        return -1;

    // get address string
    std::string address = line.substr(0, count);
    SRecord.Address = StringToUInt32(address);

    // remove address from line
    line = line.substr(count, line.length() - count);

    // CRC
    int n = count / 2;

    for (int i = 0; i < n; i++)
    {
        SRecord.CRC += static_cast<uint8_t>((SRecord.Address >> 8) & 0xFF);
    }

    SRecord.Count += n;

    return 0;
}

int SRecord::ExtractData(std::string& line, struct SRecordStruct& SRecord)
{
    if (line.length() < 2)
        return -1;

    // n*Data - CRC
    size_t n = (line.length() / 2) - 1;

    if (n < 0)
        return -1;

    for (int i = 0; i < n; i++)
    {
        std::string hexText = line.substr(0, 2);

        uint32_t value = StringToUInt32(hexText);

        SRecord.Data.emplace_back(static_cast<uint8_t>(value));
        SRecord.CRC += static_cast<uint8_t>(value);

        SRecord.Count++;

        // remaining
        line = line.substr(2, line.length() - 2);
    }

    SRecord.Count++;
    
    uint8_t checksum = CalculChecksum(SRecord);
    SRecord.CRC = checksum;
    
    uint32_t check = StringToUInt32(line);

    if (checksum == static_cast<uint8_t>(check & 0xFF))
    {
        std::cout << line << '\n';
    }

    return 0;
}

uint8_t SRecord::CalculChecksum(const struct SRecordStruct& SRecord)
{
    //SRecord.CRC %= 256; // Modulo 256 to get the lower 8 bits    
    //uint8_t checksum = ~static_cast<uint8_t>(SRecord.CRC);// Invert all bits (one's complement)

    //return (0xFF - (SRecord.CRC & 0xFF));
    //return ((SRecord.CRC ^ 0xFF) & 0xFF);
    return (0xFF ^ (SRecord.CRC & 0xFF));
}


int SRecord::LoadFile(const std::string& filepath)
{
    // Create an input file stream
    std::ifstream inputFile(filepath);

    // Check if the file is open
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Could not open the file!" << std::endl;
        return -1;
    }

    // Create table - count the number of each type
    std::array<int, 10> type;
    memset(&type, 0, 10 * sizeof(int));


    // Read the file line by line
    std::string line;
    int count = 0;

    while (std::getline(inputFile, line))
    {
        //std::cout << line << std::endl;        

        if (line[0] != 'S')
        {
            count++;
        }
        else
        {
            ExtractLine(line);

            int n = line[1] - '0';
            if (n > 9)
            {
                std::cout << "Type S out of range\n";
            }
            else
            {
                type[n]++;
            }
        }
    }

    // Display number of line without first caracter 'S'
    std::cout << "First character not S count: " << count << '\n';

    // Display number of index 'S'
    int index = 0;
    for (auto& cnt : type)
    {
        std::cout << 'S' << index++ << " count:" << cnt << '\n';
    }

    // Display size of line - count
    count = 0;
    int count4n = 0;
    int count8n = 0;
    int count16n = 0;
    int type7_cnt = 0;
    for (const auto& s : GetList())
    {
        if (s.Type == 7)
        {
            type7_cnt++;
        }

        switch (s.Data.size())
        {
        case 4:
            count4n++;
            break;

        case 8:
            count8n++;
            break;

        case 16:
            count16n++;
            break;

        default:
            count++;
            std::cout << "Type " << std::dec << s.Type;
            std::cout << "  Address 0x" << std::hex << s.Address;
            std::cout << "  Data size: " << std::dec << s.Data.size() << '\n';
            break;
        }
    }

    std::cout << " count  :" << count << '\n';
    std::cout << " count4 :" << count4n << '\n';
    std::cout << " count8 :" << count8n << '\n';
    std::cout << " count16:" << count16n << '\n';
    std::cout << " type7_cnt:" << type7_cnt << '\n';// a surveiller si plus de 1

    // Close the file
    inputFile.close();

    return 0;
}



int SRecord::FlashConnectionCheck(SerialCom& serial)
{
    uint8_t RxBuf[8];
    uint8_t TxBuf[8] = { 0 };
    uint32_t key = 0x120033F2;

    TxBuf[0] = (uint8_t)(key >> 24);
    TxBuf[1] = (uint8_t)(key >> 16);
    TxBuf[2] = (uint8_t)(key >> 8);
    TxBuf[3] = (uint8_t)(key);
    serial.Write(TxBuf, 4);

    Sleep(20);

    if (serial.Read(RxBuf, 8) == 1)
    {
        if (RxBuf[0] == 'K')
            return 0;
    }

    return -1;
}

int SRecord::FlashSwitchMode(SerialCom& serial)
{
    uint8_t RxBuf[8];
    uint8_t TxBuf[8] = { 0 };
    uint32_t key = 0x12003301;

    TxBuf[0] = (uint8_t)(key >> 24);
    TxBuf[1] = (uint8_t)(key >> 16);
    TxBuf[2] = (uint8_t)(key >> 8);
    TxBuf[3] = (uint8_t)(key);
    serial.Write(TxBuf, 4);

    Sleep(20);

    // retrieve 1 data only
    if (serial.Read(RxBuf, 1) == 1)
    {
        if (RxBuf[0] == 'K')
            return 0;
    }

    return -1;
}

int SRecord::FlashWaitEndErasePage(SerialCom& serial)
{
    uint8_t RxBuf[8];
    int count = 0;

    while (serial.Read(RxBuf, 8) == 0)
    {
        Sleep(100);

        // timeout - 10s
        if (count++ > 10000)
            return -1;
    }

    if (RxBuf[0] == 'K')
        return 0;

    return -1;
}

void SRecord::FlashFirmware(SerialCom& serial)
{
    int count;

    for (const auto& frame : GetList())
    {
        // type
        serial.Write(static_cast<uint8_t>(frame.Type));

        // size
        serial.Write(static_cast<uint8_t>(frame.Data.size() + 4));

        // address - little endian
        serial.Write(static_cast<uint8_t>(frame.Address));
        serial.Write(static_cast<uint8_t>(frame.Address >> 8));
        serial.Write(static_cast<uint8_t>(frame.Address >> 16));
        serial.Write(static_cast<uint8_t>(frame.Address >> 24));

        // data
        for (const auto& d : frame.Data)
        {
            serial.Write(d);
        }


        if (frame.Type == 7)
        {
            // Last frame
            break;
        }

        // response
        count = 0;
        uint8_t checkValue = 'F';
        while (!serial.CheckReceiveValue(checkValue))
        {
            Sleep(1);// in milliseconds

            if (count++ > 1000)
            {
                std::cout << "ERROR FLASH - firmware\n";
                return;
            }
        }

        // let the MCU switch FSM
        Sleep(1);
    }

    // jump to app - S7

}

void SRecord::Flash(SerialCom& serial)
{
    int countMillisecond = 0;

    // Empty Rx buffer to be sure
    serial.FlushReceiveBuffer();

    // check connection
    if (FlashConnectionCheck(serial) == -1)
    {
        std::cout << "ERROR FLASH - Connection with target\n";
        return;
    }

    // switch to flash firmware mode - if successful will erase page for app area
    if (FlashSwitchMode(serial) == -1)
    {
        std::cout << "ERROR FLASH - Switch mode\n";
        return;
    }
    else
    {
        if (FlashWaitEndErasePage(serial) == -1)
        {
            std::cout << "ERROR FLASH - Erase page\n";
            return;
        }
    }
    
    // Flash firmware
    FlashFirmware(serial);

    // End flash
    std::cout << "End flash.\n";
}

/*EOF*/
