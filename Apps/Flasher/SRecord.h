#pragma once

#include <cstdint>
#include <list>
#include <string>

#include "SerialCom.h"

// https://en.wikipedia.org/wiki/SREC_(file_format)

/*
Motorola S-Record
S | Type | Count | Address | Data | Checksum
S[0-9]
Type (1 hex digits)
Count (2 hex digits)
Address (4-8 hex digits)
Data
Checksum (2 hex digits)
*/

#define TYPE_DATA_COUNT 1
#define COUNT_DATA_COUNT 1
#define ADDRESS_DATA_COUNT 4
#define MAX_DATA_COUNT 8 // on this case
#define CHECKSUM_COUNT 1
#define MAX_SRECORD_DATA_LENGTH		(TYPE_DATA_COUNT + COUNT_DATA_COUNT + ADDRESS_DATA_COUNT + MAX_DATA_COUNT + CHECKSUM_COUNT)

struct SRecordStruct 
{
	int Type;
	uint16_t Count;
	uint32_t Address;
	std::list<uint8_t> Data;
	uint16_t CRC;

	SRecordStruct() : Type(0), Count(0), Address(0), CRC(0)
	{
	}
};

class SRecord
{
public:
	//SRecord() = default;
	
	int LoadFile(const std::wstring& filepath);
	std::list<SRecordStruct> GetList() const { return m_SequenceList; }
	void Flash(SerialCom& serial);

// Functions
private:
	int ExtractLine(const std::string& line);
	int ExtractAddress(std::string& line, struct SRecordStruct& SRecord);
	int ExtractAddressExt(std::string& line, struct SRecordStruct& SRecord, int count);
	int ExtractData(std::string& line, struct SRecordStruct& SRecord);
	uint8_t CalculChecksum(const struct SRecordStruct& SRecord);

	int FlashConnectionCheck(SerialCom& serial);
	int FlashSwitchMode(SerialCom& serial);
	int FlashWaitEndErasePage(SerialCom& serial);
	void FlashFirmware(SerialCom& serial);

// Attributs
private:
	std::list<SRecordStruct> m_SequenceList;

};

