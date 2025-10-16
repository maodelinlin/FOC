#include "MS8313.h"
#include "stm32f10x.h"

// ==================== 静态变量 ====================
static uint8_t output_enabled = 0;  // PWM输出状态

// ==================== 私有函数声明 ====================
static void MS8313_GPIO_Init(void);
static void MS8313_TIM2_Init(void);

// ==================== 私有函数实现 ====================

/**
 * @brief  GPIO初始化
 * @note   配置3路PWM输出引脚和使能引脚
 */
static void MS8313_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置TIM2 PWM输出引脚（3个半H桥）
    // PA0: TIM2_CH1 (A相PWM)
    // PA1: TIM2_CH2 (B相PWM)
    // PA2: TIM2_CH3 (C相PWM)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 注意：TIM2的PWM输出引脚需要特殊配置
    // 对于STM32F103，TIM2的PWM输出引脚配置为复用推挽输出
    
    // 配置使能引脚
    // PA3: MS8313使能信号
    GPIO_InitStructure.GPIO_Pin = MS8313_EN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MS8313_EN_PORT, &GPIO_InitStructure);
    
    // 初始状态：禁用输出
    GPIO_ResetBits(MS8313_EN_PORT, MS8313_EN_PIN);
}

/**
 * @brief  TIM2初始化（3个半H桥）
 * @note   配置通用定时器输出3路PWM
 */
static void MS8313_TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 使能TIM2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 配置时基
    TIM_TimeBaseStructure.TIM_Period = MS8313_PWM_PERIOD - 1;  // PWM周期
    TIM_TimeBaseStructure.TIM_Prescaler = 4 - 1;  // 预分频器：72MHz/4 = 18MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 配置PWM输出通道（3个半H桥）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;  // 初始占空比为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    // 配置3个PWM通道
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    
    // 使能预加载寄存器
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    
    // 启动定时器
    TIM_Cmd(TIM2, ENABLE);
}


// ==================== 公共函数实现 ====================

/**
 * @brief  MS8313 PWM初始化
 * @note   配置TIM1输出3路PWM（3个半H桥）
 * @retval 无
 */
void MS8313_Init(void)
{
    // 1. GPIO初始化
    MS8313_GPIO_Init();
    
    // 2. 定时器初始化
    MS8313_TIM2_Init();
    
    // 3. 初始状态：禁用PWM输出
    MS8313_DisableOutput();
    
    // 4. 设置初始占空比为0
    MS8313_StopAll();
}

/**
 * @brief  设置PWM占空比
 * @param  phase: 相别（0=A, 1=B, 2=C）
 * @param  duty: 占空比（0-1000）
 * @retval 无
 */
void MS8313_SetDutyCycle(uint8_t phase, uint16_t duty)
{
    // 限制占空比范围
    if(duty > MS8313_PWM_PERIOD)
    {
        duty = MS8313_PWM_PERIOD;
    }
    
    switch(phase)
    {
        case MS8313_PHASE_A:  // A相
            MS8313_PWM_A = duty;
            break;
            
        case MS8313_PHASE_B:  // B相
            MS8313_PWM_B = duty;
            break;
            
        case MS8313_PHASE_C:  // C相
            MS8313_PWM_C = duty;
            break;
            
        default:
            break;
    }
}

/**
 * @brief  设置三相PWM占空比
 * @param  duty_a: A相占空比（0-1000）
 * @param  duty_b: B相占空比（0-1000）
 * @param  duty_c: C相占空比（0-1000）
 * @retval 无
 */
void MS8313_SetThreePhaseDuty(uint16_t duty_a, uint16_t duty_b, uint16_t duty_c)
{
    MS8313_SetDutyCycle(MS8313_PHASE_A, duty_a);
    MS8313_SetDutyCycle(MS8313_PHASE_B, duty_b);
    MS8313_SetDutyCycle(MS8313_PHASE_C, duty_c);
}

/**
 * @brief  使能PWM输出
 * @retval 无
 */
void MS8313_EnableOutput(void)
{
    // 使能PWM输出
    TIM_Cmd(TIM2, ENABLE);
    
    // 使能MS8313芯片
    GPIO_SetBits(MS8313_EN_PORT, MS8313_EN_PIN);
    
    output_enabled = 1;
}

/**
 * @brief  禁用PWM输出
 * @retval 无
 */
void MS8313_DisableOutput(void)
{
    // 禁用PWM输出
    TIM_Cmd(TIM2, DISABLE);
    
    // 禁用MS8313芯片
    GPIO_ResetBits(MS8313_EN_PORT, MS8313_EN_PIN);
    
    output_enabled = 0;
}

/**
 * @brief  设置PWM频率
 * @param  freq: PWM频率（Hz）
 * @retval 无
 */
void MS8313_SetFrequency(uint32_t freq)
{
    uint32_t period;
    
    // 计算PWM周期值
    // 定时器频率 = 72MHz / 4 = 18MHz
    // 周期值 = 18MHz / PWM频率
    period = 18000000 / freq;
    
    // 限制周期值范围
    if(period < 100) period = 100;
    if(period > 65535) period = 65535;
    
    // 更新TIM2周期
    TIM2->ARR = period - 1;
}

/**
 * @brief  停止所有PWM输出
 * @retval 无
 */
void MS8313_StopAll(void)
{
    // 设置所有相占空比为0
    MS8313_SetThreePhaseDuty(0, 0, 0);
}


/**
 * @brief  获取PWM状态
 * @retval 1: PWM输出使能, 0: PWM输出禁用
 */
uint8_t MS8313_GetOutputStatus(void)
{
    return output_enabled;
}

/**
 * @brief  PWM测试函数
 * @note   输出固定占空比的PWM用于测试
 * @retval 无
 */
void MS8313_TestPWM(void)
{
    // 设置测试PWM占空比
    MS8313_SetThreePhaseDuty(500, 300, 700);  // A:50%, B:30%, C:70%
    
    // 确保定时器运行
    TIM_Cmd(TIM2, ENABLE);
    
    // 使能MS8313芯片
    GPIO_SetBits(MS8313_EN_PORT, MS8313_EN_PIN);
    
    // 更新状态
    output_enabled = 1;
}

/**
 * @brief  强制PWM输出测试
 * @note   直接操作寄存器输出PWM
 * @retval 无
 */
void MS8313_ForcePWMTest(void)
{
    // 直接设置PWM比较值
    TIM2->CCR1 = 500;  // A相50%
    TIM2->CCR2 = 300;  // B相30%
    TIM2->CCR3 = 700;  // C相70%
    
    // 确保定时器运行
    TIM2->CR1 |= TIM_CR1_CEN;  // 使能定时器
    
    // 使能MS8313芯片
    GPIO_SetBits(MS8313_EN_PORT, MS8313_EN_PIN);
    
    // 更新状态
    output_enabled = 1;
}

/**
 * @brief  GPIO输出测试
 * @note   直接操作GPIO输出测试引脚
 * @retval 无
 */
void MS8313_GPIO_Test(void)
{
    // 先将PWM引脚配置为普通GPIO输出
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置PA0, PA1, PA2为普通GPIO输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 输出测试信号
    GPIO_SetBits(GPIOA, GPIO_Pin_0);   // PA0 = 1
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);  // PA1 = 0
    GPIO_SetBits(GPIOA, GPIO_Pin_2);    // PA2 = 1
    
    // 使能MS8313芯片
    GPIO_SetBits(MS8313_EN_PORT, MS8313_EN_PIN);
    
    // 更新状态
    output_enabled = 1;
}
