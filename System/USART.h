#ifndef __USART_H
#define __USART_H

#include <stdint.h>

// ==================== 配置参数 ====================
// USART1 默认配置：PA9(TX), PA10(RX), 115200-8-N-1

// 波特率定义
#define USART_BAUDRATE_9600     9600
#define USART_BAUDRATE_115200   115200
#define USART_BAUDRATE_256000   256000
#define USART_BAUDRATE_460800   460800

// 操作返回值定义
#define USART_SUCCESS   1
#define USART_FAIL      0

// 接收超时时间定义（循环次数）
#define USART_TIMEOUT   0xFFFF

// ==================== 初始化与配置 ====================
/**
 * @brief  USART1 初始化
 * @note   使用 USART1: PA9(TX), PA10(RX)
 * @param  baudrate: 波特率（如 115200）
 * @retval 无
 */
void USART1_Init(uint32_t baudrate);

/**
 * @brief  USART1 重新配置波特率
 * @param  baudrate: 新的波特率
 * @retval 无
 */
void USART1_SetBaudrate(uint32_t baudrate);

// ==================== 底层发送函数 ====================
/**
 * @brief  发送一个字节
 * @param  data: 要发送的数据
 * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
 */
uint8_t USART1_SendByte(uint8_t data);

/**
 * @brief  发送多个字节
 * @param  data: 数据缓冲区指针
 * @param  len: 数据长度
 * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
 */
uint8_t USART1_SendData(uint8_t *data, uint16_t len);

/**
 * @brief  发送字符串（以 '\0' 结尾）
 * @param  str: 字符串指针
 * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
 */
uint8_t USART1_SendString(const char *str);

// ==================== 底层接收函数 ====================
/**
 * @brief  接收一个字节（阻塞）
 * @param  data: 数据接收指针
 * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
 */
uint8_t USART1_ReceiveByte(uint8_t *data);

/**
 * @brief  接收一个字节（非阻塞）
 * @param  data: 数据接收指针
 * @retval USART_SUCCESS(1): 接收到数据, USART_FAIL(0): 无数据
 */
uint8_t USART1_ReceiveByteNonBlocking(uint8_t *data);

/**
 * @brief  接收多个字节（阻塞，带超时）
 * @param  data: 数据接收缓冲区
 * @param  len: 要接收的字节数
 * @retval USART_SUCCESS(1) 或 USART_FAIL(0)
 */
uint8_t USART1_ReceiveData(uint8_t *data, uint16_t len);

// ==================== 高层应用函数 ====================
/**
 * @brief  发送十六进制数据（调试用）
 * @param  data: 数据缓冲区
 * @param  len: 数据长度
 * @retval 无
 * @note   格式: "01 02 03 0A FF"
 */
void USART1_SendHex(uint8_t *data, uint16_t len);

/**
 * @brief  发送换行符
 * @retval 无
 */
void USART1_SendNewLine(void);

/**
 * @brief  格式化输出（类似 printf）
 * @param  format: 格式化字符串
 * @param  ...: 可变参数
 * @retval 无
 */
void USART1_Printf(const char *format, ...);

// ==================== 状态查询函数 ====================
/**
 * @brief  检查发送是否完成
 * @retval 1: 完成, 0: 忙碌
 */
uint8_t USART1_IsTxComplete(void);

/**
 * @brief  检查是否有数据可读
 * @retval 1: 有数据, 0: 无数据
 */
uint8_t USART1_IsRxDataAvailable(void);

/**
 * @brief  清空接收缓冲区
 * @retval 无
 */
void USART1_FlushRx(void);

// ==================== 中断相关（可选） ====================
/**
 * @brief  使能接收中断
 * @retval 无
 */
void USART1_EnableRxInterrupt(void);

/**
 * @brief  禁用接收中断
 * @retval 无
 */
void USART1_DisableRxInterrupt(void);

/**
 * @brief  USART1 中断回调函数（用户实现）
 * @param  data: 接收到的数据
 * @retval 无
 * @note   在 stm32f10x_it.c 中调用
 */
void USART1_RxCallback(uint8_t data);

#endif

