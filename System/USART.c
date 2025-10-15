#include "USART.h"
#include "stm32f10x.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ==================== 私有变量 ====================
static uint32_t current_baudrate = USART_BAUDRATE_115200;

// ==================== 初始化与配置 ====================

/**
  * @brief  USART1 初始化
  * @note   使用 USART1: PA9(TX), PA10(RX), 默认115200-8-N-1
  * @param  baudrate: 波特率（如 115200）
  * @retval 无
  */
void USART1_Init(uint32_t baudrate)
{
	// 1. 开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	// 2. 配置 GPIO
	GPIO_InitTypeDef GPIO_InitStruct;
	
	// PA9: TX - 复用推挽输出
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	// PA10: RX - 浮空输入 或 上拉输入
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	// 3. 配置 USART1
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = baudrate;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;    // 8位数据
	USART_InitStruct.USART_StopBits = USART_StopBits_1;         // 1位停止位
	USART_InitStruct.USART_Parity = USART_Parity_No;            // 无校验
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  // 无硬件流控
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;  // 收发模式
	USART_Init(USART1, &USART_InitStruct);
	
	// 4. 使能 USART1
	USART_Cmd(USART1, ENABLE);
	
	// 保存当前波特率
	current_baudrate = baudrate;
}

/**
  * @brief  USART1 重新配置波特率
  * @param  baudrate: 新的波特率
  * @retval 无
  */
void USART1_SetBaudrate(uint32_t baudrate)
{
	USART_InitTypeDef USART_InitStruct;
	
	// 禁用 USART1
	USART_Cmd(USART1, DISABLE);
	
	// 重新配置
	USART_InitStruct.USART_BaudRate = baudrate;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &USART_InitStruct);
	
	// 重新使能
	USART_Cmd(USART1, ENABLE);
	
	current_baudrate = baudrate;
}

// ==================== 底层发送函数 ====================

/**
  * @brief  发送一个字节
  * @param  data: 要发送的数据
  * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
  */
uint8_t USART1_SendByte(uint8_t data)
{
	uint32_t timeout = USART_TIMEOUT;
	
	// 等待发送数据寄存器空
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
	{
		if(--timeout == 0) return USART_FAIL;
	}
	
	// 发送数据
	USART_SendData(USART1, data);
	
	return USART_SUCCESS;
}

/**
  * @brief  发送多个字节
  * @param  data: 数据缓冲区指针
  * @param  len: 数据长度
  * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
  */
uint8_t USART1_SendData(uint8_t *data, uint16_t len)
{
	for(uint16_t i = 0; i < len; i++)
	{
		if(USART1_SendByte(data[i]) != USART_SUCCESS)
		{
			return USART_FAIL;
		}
	}
	
	return USART_SUCCESS;
}

/**
  * @brief  发送字符串（以 '\0' 结尾）
  * @param  str: 字符串指针
  * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
  */
uint8_t USART1_SendString(const char *str)
{
	while(*str)
	{
		if(USART1_SendByte(*str++) != USART_SUCCESS)
		{
			return USART_FAIL;
		}
	}
	
	return USART_SUCCESS;
}

// ==================== 底层接收函数 ====================

/**
  * @brief  接收一个字节（阻塞，带超时）
  * @param  data: 数据接收指针
  * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
  */
uint8_t USART1_ReceiveByte(uint8_t *data)
{
	uint32_t timeout = USART_TIMEOUT;
	
	// 等待接收数据寄存器非空
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
	{
		if(--timeout == 0) return USART_FAIL;
	}
	
	// 读取数据
	*data = USART_ReceiveData(USART1);
	
	return USART_SUCCESS;
}

/**
  * @brief  接收一个字节（非阻塞）
  * @param  data: 数据接收指针
  * @retval USART_SUCCESS(1): 接收到数据, USART_FAIL(0): 无数据
  */
uint8_t USART1_ReceiveByteNonBlocking(uint8_t *data)
{
	// 检查是否有数据
	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
	{
		*data = USART_ReceiveData(USART1);
		return USART_SUCCESS;
	}
	
	return USART_FAIL;
}

/**
  * @brief  接收多个字节（阻塞，带超时）
  * @param  data: 数据接收缓冲区
  * @param  len: 要接收的字节数
  * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
  */
uint8_t USART1_ReceiveData(uint8_t *data, uint16_t len)
{
	for(uint16_t i = 0; i < len; i++)
	{
		if(USART1_ReceiveByte(&data[i]) != USART_SUCCESS)
		{
			return USART_FAIL;
		}
	}
	
	return USART_SUCCESS;
}

// ==================== 高层应用函数 ====================

/**
  * @brief  发送十六进制数据（调试用）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * @retval 无
  * @note   格式: "01 02 03 0A FF"
  */
void USART1_SendHex(uint8_t *data, uint16_t len)
{
	const char hex_table[] = "0123456789ABCDEF";
	
	for(uint16_t i = 0; i < len; i++)
	{
		// 发送高4位
		USART1_SendByte(hex_table[data[i] >> 4]);
		// 发送低4位
		USART1_SendByte(hex_table[data[i] & 0x0F]);
		
		// 发送空格（最后一个字节不加空格）
		if(i < len - 1)
		{
			USART1_SendByte(' ');
		}
	}
}

/**
  * @brief  发送换行符
  * @retval 无
  */
void USART1_SendNewLine(void)
{
	USART1_SendByte('\r');  // 回车
	USART1_SendByte('\n');  // 换行
}

/**
  * @brief  格式化输出（类似 printf）
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval 无
  */
void USART1_Printf(const char *format, ...)
{
	char buffer[256];  // 临时缓冲区
	va_list args;
	
	// 格式化字符串
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	// 发送字符串
	USART1_SendString(buffer);
}

// ==================== 状态查询函数 ====================

/**
  * @brief  检查发送是否完成
  * @retval 1: 完成, 0: 忙碌
  */
uint8_t USART1_IsTxComplete(void)
{
	return (USART_GetFlagStatus(USART1, USART_FLAG_TC) == SET) ? 1 : 0;
}

/**
  * @brief  检查是否有数据可读
  * @retval 1: 有数据, 0: 无数据
  */
uint8_t USART1_IsRxDataAvailable(void)
{
	return (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) ? 1 : 0;
}

/**
  * @brief  清空接收缓冲区
  * @retval 无
  */
void USART1_FlushRx(void)
{
	uint8_t dummy;
	
	// 读取并丢弃所有数据，直到接收缓冲区为空
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
	{
		dummy = USART_ReceiveData(USART1);
		(void)dummy;  // 避免编译器警告
	}
}

// ==================== 中断相关（可选） ====================

/**
  * @brief  使能接收中断
  * @retval 无
  */
void USART1_EnableRxInterrupt(void)
{
	// 配置 NVIC
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// 使能 USART1 接收中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

/**
  * @brief  禁用接收中断
  * @retval 无
  */
void USART1_DisableRxInterrupt(void)
{
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
}

/**
  * @brief  USART1 中断回调函数（弱定义，用户可重写）
  * @param  data: 接收到的数据
  * @retval 无
  */
__weak void USART1_RxCallback(uint8_t data)
{
	// 默认实现：什么都不做
	// 用户可以在自己的代码中重写此函数
	(void)data;
}

// ==================== printf 重定向支持 ====================

/**
  * @brief  重定向 printf 到 USART1
  * @note   需要在编译器选项中添加 --specs=nosys.specs 或 --specs=nano.specs
  */
#ifdef __GNUC__
// GCC
int _write(int file, char *ptr, int len)
{
	(void)file;
	USART1_SendData((uint8_t*)ptr, len);
	return len;
}
#else
// Keil MDK
int fputc(int ch, FILE *f)
{
	(void)f;
	USART1_SendByte((uint8_t)ch);
	return ch;
}
#endif

