#include <iostream>
#include <stdint.h>
//#include <conio.h>  // _getch()
#include <windows.h> // Sleep
#include <string>




//#version 100 core


#include "SerialCom.h"
#include "SRecord.h"

#define __TEST_SERIAL__    0

int main()
{    
    SerialCom serial("ToDoLater");
    if (serial.Open() == -1)
    {
        std::cout << "ERROR Open COM\n";
    }
    else
    {
        //TODO: find file with extension .srec

        SRecord file;
        if (file.LoadFile("power_manager.srec") != -1)
        {
            file.Flash(serial);
        }
    }

#if __TEST_SERIAL__
    uint8_t RxBuf[8];
    uint8_t TxBuf[8] = {0};
    uint32_t key;
    std::string input;
    char c = 0;
    do
    {
        std::cin >> input;

        const char * ch = input.c_str();
        std::cout << ch << '\n';
        
        c = input.at(0);

        switch (c)
        {
        case '0': // test com
            key = 0x120033F2;
            TxBuf[0] = (uint8_t)(key >> 24);
            TxBuf[1] = (uint8_t)(key >> 16);
            TxBuf[2] = (uint8_t)(key >> 8);
            TxBuf[3] = (uint8_t)(key);
            serial.Write(TxBuf, 3);
            Sleep(20);
            serial.Read(RxBuf, 8);
            break;

        case '1': // swith to flash mode
            key = 0x12003301;
            TxBuf[0] = (uint8_t)(key >> 24);
            TxBuf[1] = (uint8_t)(key >> 16);
            TxBuf[2] = (uint8_t)(key >> 8);
            TxBuf[3] = (uint8_t)(key);
            serial.Write(TxBuf, 4);
            Sleep(20);
            serial.Read(RxBuf, 8);
            break;

        case '2':
        case '3':
            serial.Write(c);
            Sleep(10);
            serial.Read(RxBuf, 8);
            break;

        default: break;
        }

    } while (c != 'q');
#endif

    return EXIT_SUCCESS;
}

