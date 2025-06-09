#include <iostream>
#include <stdint.h>
//#include <conio.h>  // _getch()
#include <windows.h> // Sleep
#include <string>
#include <tchar.h>



//#version 100 core


#include "SerialCom.h"
#include "SRecord.h"

#define __TEST_SERIAL__    0

#define EXECUTABLE_FOLDER_W         _T("./mcu_executable_file/")
#define EXECUTABLE_EXTENSION        _T("./mcu_executable_file/*.srec")


int main()
{    

#ifndef _DEBUG
    //--- Create folder for executable file ---
    LPCWSTR folderName = L"mcu_executable_file";
    DWORD attribs = GetFileAttributesW(folderName);
    if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY)) 
    {
        std::wcout << L"Folder already exists: " << folderName << std::endl;
    }
    else 
    {
        if (CreateDirectoryW(folderName, NULL)) 
        {
            std::wcout << L"Folder created: " << folderName << std::endl;
        }
        else 
        {
            std::wcerr << L"Failed to create folder. Error: " << GetLastError() << std::endl;
        }
    }
#endif



    //--- Find executable file ---
    WIN32_FIND_DATA findFileData;    
    HANDLE hFind = FindFirstFile(EXECUTABLE_EXTENSION, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        bool found = false;

        do
        {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::wcout << findFileData.cFileName << std::endl;
                found = true;
                break;
            }
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);

        if (found)
        {
            SRecord file;
            std::wstring fileName(EXECUTABLE_FOLDER_W);
            fileName += findFileData.cFileName;
            if (file.LoadFile(fileName) != -1)
            {
                SerialCom serial("ToDoLater");
                if (serial.Open() == -1)
                {
                    std::cout << "ERROR Open COM\n";
                }
                else
                {
                    file.Flash(serial);
                }
            }
        }
    }
    


#if 0
    SerialCom serial("ToDoLater");
    if (serial.Open() == -1)
    {
        std::cout << "ERROR Open COM\n";
    }
    else
    {
        //TODO: find file with extension .srec

        SRecord file;
        if (file.LoadFile("./mcu_executable_file/power_manager.srec") != -1)
        {
            file.Flash(serial);
        }
    }
#endif

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

