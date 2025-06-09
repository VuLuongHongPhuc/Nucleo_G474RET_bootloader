#pragma once

#include <Windows.h>

class SerialCom
{
public:
	
	SerialCom(std::string name);
		

	int Open();
	void Write(const uint8_t& c);
	int32_t Write(const uint8_t* const pBuf, const int lenght);
	int32_t Read(uint8_t* pBuf, uint32_t lenght);
	bool CheckReceiveValue(const uint8_t value);
	void FlushReceiveBuffer();

	bool IsOpen() { return m_IsOpen; };

private:
	void SetConfig();

private:
	std::string m_Name;
	HANDLE m_hUart;
	bool m_IsOpen;

};

