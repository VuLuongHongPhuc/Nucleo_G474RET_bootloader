/*
 * flash.h
 *
 *  Created on: Dec 14, 2024
 *      Author: admin
 *
 *************************************************************
 *
 * STM32G474RE
 * 128k RAM
 * 512k FLASH
 *
 * For now it work only for Dual mode bank
 * DBANK: Dual mode
 *
 * 20K FLASH for BOOT reserved
 *
 */

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

typedef struct {
	uint8_t   type;
	uint8_t   size;
	uint32_t  address;
	int16_t   count;
	uint8_t*  pData;
}FlashFrame_TypeDef;

//void FLASH_PageErase(void); -> stm32g4xx_hal_flash_ex.h


void FLASH_M_Initialize(FlashFrame_TypeDef* const ptr);
void FLASH_M_ErasePage(void);
void FLASH_M_Write(const FlashFrame_TypeDef* const pFrame);
void FLASH_M_Read(const FlashFrame_TypeDef* const pFrame);
void FLASH_M_ReadTest(void);

#endif /* INC_FLASH_H_ */
