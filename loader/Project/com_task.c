

/*** Includes ******************************************************/
#include "com_task.h"
#include "usart.h"
#include "flash.h"


/*** MACRO *********************************************************/
#define DEF_TIMEOUT_RX         100u   /* unit in millisecond */
#define DEF_TIMEOUT_TX         100u   /* unit in millisecond */
#define DEF_TIMEOUT_RX_FRAME   1000u  /* unit in millisecond */

typedef enum
{
	STATE_SYNC = 0,
	STATE_WAIT_CMD,
	STATE_RECEIVE_FRAME,
	STATE_FLASH,
	STATE_JUMP2APP,
	STATE_MAX
}FSM_COM_TypeDef;

/*** Private variables *********************************************/
static uint8_t _buffer_rx[32] = {0};
static uint8_t _buffer_tx[32] = {0};
static FSM_COM_TypeDef _state;
static FlashFrame_TypeDef _frame_t;


/*** Private functions *********************************************/
static void ReceiveFrame(void);
static void FlashFrame(void);
static void Jump2APP(void);

/*** Function implementation ***************************************/

void COM_Initialize(void)
{
	_state = STATE_SYNC;

	FLASH_M_Initialize(&_frame_t);
}

void COM_Task(void)
{
	uint32_t key;

	/* Elapse time operation */
	//HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_RESET);
	//HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_RESET);

	switch(_state)
	{
	case STATE_SYNC:
		if(HAL_UART_Receive(&hlpuart1, _buffer_rx, 4, DEF_TIMEOUT_RX)==HAL_OK)
		{
			/* Check key */
			key  = (uint32_t)(_buffer_rx[0] << 24);
			key |= (uint32_t)(_buffer_rx[1] << 16);
			key |= (uint32_t)(_buffer_rx[2] << 8);
			key |= (uint32_t)(_buffer_rx[3]);

			if (0x120033F2 == key)
			{
				_buffer_tx[0] = 'K';
				HAL_UART_Transmit(&hlpuart1, _buffer_tx, 1, DEF_TIMEOUT_TX);

				_state = STATE_WAIT_CMD;
			}
			else
			{
				HAL_UART_Transmit(&hlpuart1, _buffer_rx, 4, DEF_TIMEOUT_TX);
			}
		}
		break;

	case STATE_WAIT_CMD:
		if(HAL_UART_Receive(&hlpuart1, _buffer_rx, 4, DEF_TIMEOUT_RX)==HAL_OK)
		{
			/* Check key */
			key  = (uint32_t)(_buffer_rx[0] << 24);
			key |= (uint32_t)(_buffer_rx[1] << 16);
			key |= (uint32_t)(_buffer_rx[2] << 8);
			key |= (uint32_t)(_buffer_rx[3]);

			if (0x12003301 == key)
			{
				_buffer_tx[0] = 'K';
				HAL_UART_Transmit(&hlpuart1, _buffer_tx, 1, DEF_TIMEOUT_TX);

				_state = STATE_RECEIVE_FRAME;

				// Erase pages - App area
				FLASH_M_ErasePage();
				HAL_UART_Transmit(&hlpuart1, _buffer_tx, 1, DEF_TIMEOUT_TX);
			}
			else
			{
				HAL_UART_Transmit(&hlpuart1, _buffer_rx, 4, DEF_TIMEOUT_TX);
			}
		}
		break;

	case STATE_RECEIVE_FRAME:
		ReceiveFrame();
#if 0
		if(HAL_UART_Receive(&hlpuart1, _buffer_rx, 1, DEF_TIMEOUT_RX)==HAL_OK)
		{
			_buffer_tx[0] = _buffer_rx[0];
			HAL_UART_Transmit(&hlpuart1, _buffer_tx, 1, DEF_TIMEOUT_TX);
		}
#endif
		break;

	case STATE_FLASH:
		FlashFrame();
		break;

	case STATE_JUMP2APP:
		Jump2APP();
		break;

	default:
		break;
	}
}

static void ReceiveFrame(void)
{
#if 0
	if(HAL_UART_Receive(&hlpuart1, _buffer_rx, 14, DEF_TIMEOUT_RX_FRAME)==HAL_OK)
	{
		_frame_t.type = _buffer_rx[0];
		_frame_t.size = _buffer_rx[1];
		_frame_t.address = *(uint32_t*)&_buffer_rx[2];
		_frame_t.count = _frame_t.size - 4;             /* -4 => size of address S3 */
		_frame_t.pData = (uint8_t*)&_buffer_rx[6];       /* Data */

		if (_frame_t.type == 3)
		{
			_state = STATE_FLASH;
		}
		else if (_frame_t.type == 7)
		{
			_state = STATE_JUMP2APP;
		}
		else
		{
			_state = STATE_MAX;
		}
	}
#else
	if(HAL_UART_Receive(&hlpuart1, _buffer_rx, 2, DEF_TIMEOUT_RX_FRAME)==HAL_OK)
	{
		_frame_t.type = _buffer_rx[0];
		_frame_t.size = _buffer_rx[1];

		if(HAL_UART_Receive(&hlpuart1, _buffer_rx, (uint16_t)_frame_t.size, DEF_TIMEOUT_RX_FRAME)==HAL_OK)
		{
			_frame_t.address = *(uint32_t*)&_buffer_rx[0];
			_frame_t.count = _frame_t.size - 4;             /* -4 => size of address S3 */
			_frame_t.pData = (uint8_t*)&_buffer_rx[4];       /* Data */

			if (_frame_t.type == 3)
			{
				_state = STATE_FLASH;
			}
			else if (_frame_t.type == 7)
			{
				_state = STATE_JUMP2APP;
			}
			else
			{
				_state = STATE_MAX;
			}
		}
	}
#endif
}

static void FlashFrame(void)
{
	FLASH_M_Write(&_frame_t);

	// Response end flash frame
	_buffer_tx[0] = 'F';
	HAL_UART_Transmit(&hlpuart1, _buffer_tx, 1, DEF_TIMEOUT_TX);

	_state = STATE_RECEIVE_FRAME;
}

// start @ APP
#define	APP_ADDR_START		((uint32_t)0x5000u) // 20K
typedef void (*pFunction)(void);

static void Jump2APP(void)
{
	uint32_t JumpAddress;
	pFunction Jump_To_Application;

	// TODO: DeInitialize


	//jump to the application
	JumpAddress = *(uint32_t*) (APP_ADDR_START + 4);
	Jump_To_Application = (pFunction) JumpAddress;

	//initialize application's stack pointer
	__set_MSP(*(uint32_t*) APP_ADDR_START);

	Jump_To_Application();
}

void COM_BT_JumpToApp(void)
{
	Jump2APP();
}
