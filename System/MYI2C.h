#ifndef __MYI2C_H
#define __MYI2C_H

#include <stdint.h>

// I2C 操作返回值定义
#define I2C_SUCCESS  1
#define I2C_FAIL     0

// I2C 方向定义
#define I2C_DIRECTION_WRITE  0
#define I2C_DIRECTION_READ   1

// ==================== 初始化与复位 ====================
// I2C 初始化
void MYI2C_Init(void);

// I2C 软件复位
void MYI2C_Reset(void);

// ==================== 底层操作函数 ====================
// 发送 START 信号
uint8_t I2C_Start(void);

// 发送 STOP 信号
void I2C_Stop(void);

// 发送设备地址（包含读写方向）
uint8_t I2C_SendAddress(uint8_t dev_addr, uint8_t direction);

// 发送一个字节数据
uint8_t I2C_SendByte(uint8_t data);

// 接收一个字节数据
uint8_t I2C_ReceiveByte(uint8_t *data, uint8_t ack);

// 等待字节传输完成
uint8_t I2C_WaitBTF(void);

// ==================== 高层应用函数 ====================
// I2C 写入单个字节到指定寄存器
uint8_t I2C_WriteByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

// I2C 写入多个字节到指定寄存器
uint8_t I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len);

// I2C 从指定寄存器读取多个字节
uint8_t I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len);

// I2C 读取单个字节（便捷函数）
uint8_t I2C_ReadByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);

#endif
