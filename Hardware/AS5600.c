#include "AS5600.h"
#include "stm32f10x.h"                  // Device header



void AS5600_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitSturct;
	GPIO_InitSturct.GPIO_Mode=GPIO_Mode_AF_OD;
	GPIO_InitSturct.GPIO_Pin=GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitSturct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitSturct);
	
	I2C_InitTypeDef I2C_InitStruct;
	I2C_DeInit(I2C1);
	I2C_InitStruct.I2C_Mode=I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle=I2C_DutyCycle_2; //占空比
	I2C_InitStruct.I2C_OwnAddress1=0;  //本机地址
	I2C_InitStruct.I2C_Ack=I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress=I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_ClockSpeed=400000;
	I2C_Init(I2C1,&I2C_InitStruct);
	I2C_Cmd(I2C1,ENABLE);
	
}

