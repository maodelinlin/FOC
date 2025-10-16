#include "stm32f10x.h"
#include "Delay.h"

// SysTick计数器
volatile uint32_t systick_count = 0;

/**
  * @brief  SysTick初始化
  * @param  无
  * @retval 无
  */
void Delay_Init(void)
{
	// 配置SysTick为1ms中断
	SysTick_Config(SystemCoreClock / 1000);
}

/**
  * @brief  微秒级延时
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  */
void Delay_us(uint32_t xus)
{
	SysTick->LOAD = 72 * xus;				//设置定时器重装值
	SysTick->VAL = 0x00;					//清空当前计数值
	SysTick->CTRL = 0x00000005;				//设置时钟源为HCLK，启动定时器
	while(!(SysTick->CTRL & 0x00010000));	//等待计数到0
	SysTick->CTRL = 0x00000004;				//关闭定时器
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}
 
/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
}

/**
  * @brief  获取系统滴答计数
  * @param  无
  * @retval 系统滴答计数值（ms）
  */
uint32_t Delay_GetTick(void)
{
	return systick_count;
}

