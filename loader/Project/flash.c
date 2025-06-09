/*
 * flash.c
 *
 *  Created on: Dec 14, 2024
 *      Author: Phuc VU
 *
 */

/* @brief STM32G474RE 128 RAM / 512 FLASH
 * */


/*** Includes ******************************************************/
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_flash.h"
#include "flash.h"

/*** MACRO *********************************************************/
#define START_TEST_ADDRESS    0x8005000

#define SINGLE_START_PAGE_ERASE (5u)   /* 20K = 5 * page size = 5 * 4K */
#define DUAL_START_PAGE_ERASE   (10u)  /* 20K = 10 * page size = 10 * 2K */
#define FLASH_BUFFER_SIZE        128u


/*** Private variables *********************************************/
//static uint8_t _flash_buffer[FLASH_BUFFER_SIZE + 1] = {0};


/*** Private prototype *********************************************/
static uint8_t GetBankMode();


/*** Function implementation ***************************************/
void FLASH_M_Initialize(FlashFrame_TypeDef * const ptr)
{
	//ptr->pData = _flash_buffer;
	//GetBankMode();
}

/*
 * @brief Get DBANK mode Single/Dual
 * @note Dual Bank : p.95
 *   page = 2k
 *   bank1 = 128 x page = page[0..127]
 *   bank2 = 128 x page = page[0..127]
 * @note Single Bank : p.96
 *   page = 4k
 *   page[0..127]
 * */
static uint8_t GetBankMode()
{
	static uint8_t mode = 0xFF;

	if (mode != 0xFF)
		return mode;


	uint32_t* ptr = (uint32_t*)0x1FFF7800;
	uint32_t UserOPT = *ptr;
	if (UserOPT & (1UL << 22))
	{
		/* Dual : default */
		mode = 1;
	}
	else
	{
		/* Single */
		mode = 0;
	}

	return mode;
}

static uint32_t EraseSingleBank(void)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PageError = 0;
	uint32_t errorCode = 0;
	uint32_t maskError;


	HAL_FLASH_Unlock();

	// Configure erase parameters for FLASH_BANK_1
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;                       // Page erase
	EraseInitStruct.Banks = FLASH_BANK_1;                                    // Choose bank (FLASH_BANK_1 or FLASH_BANK_2)
	EraseInitStruct.Page = SINGLE_START_PAGE_ERASE;                          // Page number to erase
	EraseInitStruct.NbPages = FLASH_BUFFER_SIZE-SINGLE_START_PAGE_ERASE;     // Number of pages to erase

	// Erase the page
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
	{
		// Handle error
		errorCode = HAL_FLASH_GetError();

		// Implement error handling code here

		// check and clear all error flag
		maskError = FLASH->SR & 0xC3FB; // except FLASH->SR.BSY
		FLASH->SR |= maskError;
	}

	HAL_FLASH_Lock();

	return errorCode;
}

/*
 * @brief Erase sequence for DBANK = Dual
 * @note 20K FLASH reserved for BOOT program
 * */
static uint32_t EraseDualBank(void)
{


	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PageError = 0;
	uint32_t errorCode = 0;
	uint32_t maskError;

	HAL_FLASH_Unlock();

	// Configure erase parameters for FLASH_BANK_1
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;                    // Page erase
	EraseInitStruct.Banks = FLASH_BANK_1;                                 // Choose bank (FLASH_BANK_1 or FLASH_BANK_2)
	EraseInitStruct.Page = DUAL_START_PAGE_ERASE;                         // Page number to erase
	EraseInitStruct.NbPages = FLASH_BUFFER_SIZE - DUAL_START_PAGE_ERASE;  // Number of pages to erase

	// Erase the page
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
	{
		// Handle error
		errorCode = HAL_FLASH_GetError();
		// Implement error handling code here

		// check and clear all error flag
		maskError = FLASH->SR & 0xC3FB; // except FLASH->SR.BSY
		FLASH->SR |= maskError;
	}

	if (errorCode)
	{
		HAL_FLASH_Lock();
		return errorCode;
	}

	// Configure erase parameters for FLASH_BANK_2
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES; // Page erase
	EraseInitStruct.Banks = FLASH_BANK_2;              // Choose bank (FLASH_BANK_1 or FLASH_BANK_2)
	EraseInitStruct.Page = 0;                          // Page number to erase
	EraseInitStruct.NbPages = FLASH_BUFFER_SIZE;       // Number of pages to erase

	// Erase the page
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
	{
	    // Handle error
	    errorCode = HAL_FLASH_GetError();
	    // Implement error handling code here

	    // check and clear all error flag
	    maskError = FLASH->SR & 0xC3FB; // except FLASH->SR.BSY
	    FLASH->SR |= maskError;
	}

	HAL_FLASH_Lock();

	return errorCode;
}


void FLASH_M_ErasePage(void)
{
	if (GetBankMode() == 1)
	{
		/* Erase - mode dual page */
		EraseDualBank();
	}
	else
	{
		/* Erase - mode single page */
		EraseSingleBank();
	}
}

/*
 * @brief Write in flash
 * @param[in] pBlock - a block to write
 * */
void FLASH_M_Write(const FlashFrame_TypeDef* const pFrame)
{
	/* NOTE p.103 RM0440
	 * It is only possible to program double word (2 x 32-bit data)
	 */

	uint32_t errorCode;
	uint32_t maskError;
	uint64_t u64value = 0;

	uint32_t address = pFrame->address;
	int16_t count    = pFrame->count;
	uint8_t* ptr     = (uint8_t*)pFrame->pData;

	if (count == 0)
		return;


	HAL_FLASH_Unlock();

	do
	{
		if (count == 4 )
		{
			// p.104 must be completed with erase value 0xFFFFFFFF00000000
			u64value = 0xFFFFFFFF00000000 + *(uint32_t*)ptr;
		}
		else
		{
#if 1
			/* little-endian */
			u64value = (uint64_t)ptr[0];
			u64value |= (uint64_t)ptr[1] << 8u;
			u64value |= (uint64_t)ptr[2] << 16u;
			u64value |= (uint64_t)ptr[3] << 24u;
			u64value |= (uint64_t)ptr[4] << 32u;
			u64value |= (uint64_t)ptr[5] << 40u;
			u64value |= (uint64_t)ptr[6] << 48u;
			u64value |= (uint64_t)ptr[7] << 56u;
#else
			u64value = *(uint64_t*)ptr;// don't want to work anymore
#endif
		}

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address , u64value) != HAL_OK)
		{
			// Handle error
			errorCode = HAL_FLASH_GetError();
			// Implement error handling code here
			(void)errorCode;

			// Check and clear all error flag
			maskError = FLASH->SR & 0xC3FB; // except FLASH->SR.BSY
			FLASH->SR |= maskError;

			break;
		}

		//if (FLASH_SR.EOP == 1){FLASH_SR.EOP == 0;}

		address += 8; // sizeof(uint64_t) = 8
		ptr += 8;
		count -= 8;

		if (count <= 0)
			break;

	}while(count > 0);

	//FLASH_SR.FSTPG = 0

	HAL_FLASH_Lock();
}

/**
 * @brief Read flash data
 * @param[in/out] pBlock - count = number of byte size
 * */
void FLASH_M_Read(const FlashFrame_TypeDef* const pFrame)
{
	uint16_t i;
	uint32_t address = pFrame->address;
	int16_t count    = pFrame->count;
	uint8_t* ptr     = (uint8_t*)pFrame->pData;

	if (count == 0)
		return;

	for(i=0; i<count; i++)
	{
		*ptr = *(uint8_t*)address;
		ptr++;
	}
}




#define BANK2_PAGE127_START 0x807F800 // last block

void FLASH_ReadTest(void)
{
	uint64_t readData = *(uint64_t *)BANK2_PAGE127_START;
	if (readData != 0)
	{
	    // Handle verification failure
	}
}

/*EOF*/
