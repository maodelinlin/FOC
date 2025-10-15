#include "MYI2C.h"
#include "stm32f10x.h"

// 超时时间定义
#define I2C_TIMEOUT  0xFFFF

/**
  * @brief  I2C 初始化函数
  * @note   使用 I2C1: PB6(SCL), PB7(SDA), 速率 400kHz
  * @param  无
  * @retval 无
  */
void MYI2C_Init(void)
{
	// 开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	
	// 配置 GPIO：PB6(SCL), PB7(SDA) - 复用开漏输出
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	// 配置 I2C1
	I2C_InitTypeDef I2C_InitStruct;
	I2C_DeInit(I2C1);
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;           // 占空比 2:1
	I2C_InitStruct.I2C_OwnAddress1 = 0x00;                     // 主机地址（任意）
	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;                   // 使能应答
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_ClockSpeed = 400000;                    // 400kHz 快速模式
	I2C_Init(I2C1, &I2C_InitStruct);
	I2C_Cmd(I2C1, ENABLE);
}

/**
  * @brief  I2C 软件复位（用于总线挂死恢复）
  * @param  无
  * @retval 无
  */
void MYI2C_Reset(void)
{
	I2C_Cmd(I2C1, DISABLE);
	I2C_SoftwareResetCmd(I2C1, ENABLE);
	I2C_SoftwareResetCmd(I2C1, DISABLE);
	I2C_Cmd(I2C1, ENABLE);
}

// ==================== 底层操作函数 ====================

/**
  * @brief  发送 START 信号
  * @param  无
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_Start(void)
{
	uint32_t timeout = I2C_TIMEOUT;
	
	I2C1->CR1 |= I2C_CR1_START;
	while(!(I2C1->SR1 & I2C_SR1_SB))
	{
		if(--timeout == 0) return I2C_FAIL;
	}
	
	return I2C_SUCCESS;
}

/**
  * @brief  发送 STOP 信号
  * @param  无
  * @retval 无
  */
void I2C_Stop(void)
{
	I2C1->CR1 |= I2C_CR1_STOP;
}

/**
  * @brief  发送设备地址
  * @param  dev_addr: 从设备地址（7位，不含读写位）
  * @param  direction: 方向 (I2C_DIRECTION_WRITE 或 I2C_DIRECTION_READ)
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_SendAddress(uint8_t dev_addr, uint8_t direction)
{
	uint32_t timeout = I2C_TIMEOUT;
	
	I2C1->DR = (dev_addr << 1) | direction;
	while(!(I2C1->SR1 & I2C_SR1_ADDR))
	{
		if(--timeout == 0) return I2C_FAIL;
	}
	
	// 清除 ADDR 标志（读 SR2 自动清除）
	(void)I2C1->SR2;
	
	return I2C_SUCCESS;
}

/**
  * @brief  发送一个字节数据
  * @param  data: 要发送的数据
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_SendByte(uint8_t data)
{
	uint32_t timeout = I2C_TIMEOUT;
	
	I2C1->DR = data;
	while(!(I2C1->SR1 & I2C_SR1_TXE))
	{
		if(--timeout == 0) return I2C_FAIL;
	}
	
	return I2C_SUCCESS;
}

/**
  * @brief  接收一个字节数据
  * @param  data: 数据接收指针
  * @param  ack: 是否发送应答 (1: ACK, 0: NACK)
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_ReceiveByte(uint8_t *data, uint8_t ack)
{
	uint32_t timeout = I2C_TIMEOUT;
	
	// 配置 ACK/NACK
	if(ack)
		I2C1->CR1 |= I2C_CR1_ACK;
	else
		I2C1->CR1 &= ~I2C_CR1_ACK;
	
	// 等待接收数据寄存器非空
	while(!(I2C1->SR1 & I2C_SR1_RXNE))
	{
		if(--timeout == 0) return I2C_FAIL;
	}
	
	*data = I2C1->DR;
	return I2C_SUCCESS;
}

/**
  * @brief  等待字节传输完成
  * @param  无
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_WaitBTF(void)
{
	uint32_t timeout = I2C_TIMEOUT;
	
	while(!(I2C1->SR1 & I2C_SR1_BTF))
	{
		if(--timeout == 0) return I2C_FAIL;
	}
	
	return I2C_SUCCESS;
}

// ==================== 高层应用函数 ====================

/**
  * @brief  I2C 写入单个字节到指定寄存器
  * @param  dev_addr: 从设备地址（7位，不含读写位）
  * @param  reg_addr: 寄存器地址
  * @param  data: 要写入的数据
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_WriteByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
	// 1. 发送 START 信号
	if(I2C_Start() != I2C_SUCCESS) return I2C_FAIL;
	
	// 2. 发送从设备地址（写模式）
	if(I2C_SendAddress(dev_addr, I2C_DIRECTION_WRITE) != I2C_SUCCESS) return I2C_FAIL;
	
	// 3. 发送寄存器地址
	if(I2C_SendByte(reg_addr) != I2C_SUCCESS) return I2C_FAIL;
	
	// 4. 发送数据
	if(I2C_SendByte(data) != I2C_SUCCESS) return I2C_FAIL;
	
	// 5. 等待字节传输完成
	if(I2C_WaitBTF() != I2C_SUCCESS) return I2C_FAIL;
	
	// 6. 发送 STOP 信号
	I2C_Stop();
	
	return I2C_SUCCESS;
}

/**
  * @brief  I2C 写入多个字节到指定寄存器
  * @param  dev_addr: 从设备地址（7位，不含读写位）
  * @param  reg_addr: 寄存器起始地址
  * @param  data: 要写入的数据缓冲区
  * @param  len: 数据长度
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	// 1. 发送 START 信号
	if(I2C_Start() != I2C_SUCCESS) return I2C_FAIL;
	
	// 2. 发送从设备地址（写模式）
	if(I2C_SendAddress(dev_addr, I2C_DIRECTION_WRITE) != I2C_SUCCESS) return I2C_FAIL;
	
	// 3. 发送寄存器地址
	if(I2C_SendByte(reg_addr) != I2C_SUCCESS) return I2C_FAIL;
	
	// 4. 等待寄存器地址传输完成
	if(I2C_WaitBTF() != I2C_SUCCESS) return I2C_FAIL;
	
	// 5. 发送数据
	for(uint8_t i = 0; i < len; i++)
	{
		if(I2C_SendByte(data[i]) != I2C_SUCCESS) return I2C_FAIL;
		
		// 最后一个字节需要等待传输完成
		if(i == len - 1)
		{
			if(I2C_WaitBTF() != I2C_SUCCESS) return I2C_FAIL;
		}
	}
	
	// 6. 发送 STOP 信号
	I2C_Stop();
	
	return I2C_SUCCESS;
}

/**
  * @brief  I2C 从指定寄存器读取多个字节
  * @param  dev_addr: 从设备地址（7位，不含读写位）
  * @param  reg_addr: 寄存器起始地址
  * @param  data: 数据接收缓冲区
  * @param  len: 要读取的数据长度
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	// 1. 发送 START 信号
	if(I2C_Start() != I2C_SUCCESS) return I2C_FAIL;
	
	// 2. 发送从设备地址（写模式）- 先写寄存器地址
	if(I2C_SendAddress(dev_addr, I2C_DIRECTION_WRITE) != I2C_SUCCESS) return I2C_FAIL;
	
	// 3. 发送寄存器地址
	if(I2C_SendByte(reg_addr) != I2C_SUCCESS) return I2C_FAIL;
	
	// 4. 等待寄存器地址完全发送
	if(I2C_WaitBTF() != I2C_SUCCESS) return I2C_FAIL;
	
	// 5. 发送重复 START 条件（Repeated START）
	if(I2C_Start() != I2C_SUCCESS) return I2C_FAIL;
	
	// 6. 发送从设备地址（读模式）
	if(I2C_SendAddress(dev_addr, I2C_DIRECTION_READ) != I2C_SUCCESS) return I2C_FAIL;
	
	// 7. 读取数据
	for(uint8_t i = 0; i < len; i++)
	{
		if(i == len - 1)
		{
			// 最后一个字节：NACK + STOP
			I2C_Stop();
			if(I2C_ReceiveByte(&data[i], 0) != I2C_SUCCESS) return I2C_FAIL;
		}
		else if(i == len - 2)
		{
			// 倒数第二个字节：先读取，再配置 NACK
			if(I2C_ReceiveByte(&data[i], 1) != I2C_SUCCESS) return I2C_FAIL;
		}
		else
		{
			// 其他字节：ACK
			if(I2C_ReceiveByte(&data[i], 1) != I2C_SUCCESS) return I2C_FAIL;
		}
	}
	
	// 8. 重新启用 ACK（为下次通信做准备）
	I2C1->CR1 |= I2C_CR1_ACK;
	
	return I2C_SUCCESS;
}

/**
  * @brief  I2C 读取单个字节（便捷函数）
  * @param  dev_addr: 从设备地址（7位，不含读写位）
  * @param  reg_addr: 寄存器地址
  * @param  data: 数据接收指针
  * @retval I2C_SUCCESS(1) 或 I2C_FAIL(0)
  */
uint8_t I2C_ReadByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
	return I2C_Read(dev_addr, reg_addr, data, 1);
}

