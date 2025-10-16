#ifndef __MS8313_H
#define __MS8313_H

#include <stdint.h>

// ==================== 硬件配置 ====================
#define MS8313_PWM_FREQ     16000   // PWM频率（Hz）
#define MS8313_PWM_PERIOD   1000    // PWM周期值（ARR）

// PWM通道定义（3个半H桥）
#define MS8313_PWM_A        TIM2->CCR1  // A相PWM
#define MS8313_PWM_B        TIM2->CCR2  // B相PWM
#define MS8313_PWM_C        TIM2->CCR3  // C相PWM

// 相别定义
#define MS8313_PHASE_A      0
#define MS8313_PHASE_B      1
#define MS8313_PHASE_C      2

// 使能引脚定义
#define MS8313_EN_PORT      GPIOA
#define MS8313_EN_PIN       GPIO_Pin_3

// ==================== 函数声明 ====================

/**
 * @brief  MS8313 PWM初始化
 * @note   配置TIM1和TIM8输出6路PWM
 * @retval 无
 */
void MS8313_Init(void);

/**
 * @brief  设置PWM占空比
 * @param  phase: 相别（0=A, 1=B, 2=C）
 * @param  duty: 占空比（0-1000）
 * @retval 无
 */
void MS8313_SetDutyCycle(uint8_t phase, uint16_t duty);

/**
 * @brief  设置三相PWM占空比
 * @param  duty_a: A相占空比（0-1000）
 * @param  duty_b: B相占空比（0-1000）
 * @param  duty_c: C相占空比（0-1000）
 * @retval 无
 */
void MS8313_SetThreePhaseDuty(uint16_t duty_a, uint16_t duty_b, uint16_t duty_c);

/**
 * @brief  使能PWM输出
 * @retval 无
 */
void MS8313_EnableOutput(void);

/**
 * @brief  禁用PWM输出
 * @retval 无
 */
void MS8313_DisableOutput(void);

/**
 * @brief  设置PWM频率
 * @param  freq: PWM频率（Hz）
 * @retval 无
 */
void MS8313_SetFrequency(uint32_t freq);

/**
 * @brief  停止所有PWM输出
 * @retval 无
 */
void MS8313_StopAll(void);


/**
 * @brief  获取PWM状态
 * @retval 1: PWM输出使能, 0: PWM输出禁用
 */
uint8_t MS8313_GetOutputStatus(void);

/**
 * @brief  PWM测试函数
 * @note   输出固定占空比的PWM用于测试
 * @retval 无
 */
void MS8313_TestPWM(void);

/**
 * @brief  强制PWM输出测试
 * @note   直接操作寄存器输出PWM
 * @retval 无
 */
void MS8313_ForcePWMTest(void);

/**
 * @brief  GPIO输出测试
 * @note   直接操作GPIO输出测试引脚
 * @retval 无
 */
void MS8313_GPIO_Test(void);

#endif
