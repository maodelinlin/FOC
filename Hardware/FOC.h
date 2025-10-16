#ifndef __FOC_H
#define __FOC_H

#include <stdint.h>
#include <math.h>

// ==================== FOC配置参数 ====================
#define FOC_CONTROL_FREQ       1000    // FOC控制频率（Hz）
#define FOC_PWM_FREQ           16000    // PWM频率（Hz）
#define FOC_PWM_PERIOD         1000    // PWM周期值

// 数学常量
#define PI                     3.14159265358979f
#define SQRT3                 1.73205080756888f
#define SQRT3_INV             0.57735026918963f

// FOC控制参数
#define FOC_MAX_VOLTAGE        12.0f   // 最大电压（V）
#define FOC_MIN_VOLTAGE        0.0f    // 最小电压（V）
#define FOC_MAX_SPEED          3000.0f // 最大转速（RPM）
#define FOC_MIN_SPEED          0.0f    // 最小转速（RPM）

// PI控制器参数
#define PI_SPEED_KP            0.1f    // 速度环比例增益
#define PI_SPEED_KI            0.01f   // 速度环积分增益
#define PI_SPEED_MAX           10.0f   // 速度环输出限制
#define PI_SPEED_MIN           -10.0f  // 速度环输出限制

// ==================== 数据结构 ====================
/**
 * @brief PI控制器结构体
 */
typedef struct {
    float kp;                   // 比例增益
    float ki;                   // 积分增益
    float integral;             // 积分项
    float output_max;           // 输出上限
    float output_min;           // 输出下限
    float last_error;           // 上次误差
} PI_Controller_t;

/**
 * @brief FOC控制结构体
 */
typedef struct {
    // 输入参数
    uint16_t angle;             // 位置角度（0-4095）
    float speed_rpm;            // 实际转速（RPM）
    float speed_ref;            // 转速参考值（RPM）
    float voltage_ref;          // 电压参考值
    
    // 坐标变换
    float theta;                // 电角度（弧度）
    float valpha;               // α轴电压
    float vbeta;                // β轴电压
    float vd;                   // d轴电压
    float vq;                   // q轴电压
    
    // PWM输出
    uint16_t pwm_a;             // A相PWM
    uint16_t pwm_b;             // B相PWM
    uint16_t pwm_c;             // C相PWM
    
    // 控制状态
    uint8_t enable;             // 使能标志
    uint8_t direction;          // 方向（0=正转，1=反转）
    
    // PI控制器
    PI_Controller_t speed_pi;   // 速度环PI控制器
} FOC_Control_t;

// ==================== 函数声明 ====================

// ==================== 初始化函数 ====================
/**
 * @brief  FOC系统初始化
 * @retval 无
 */
void FOC_Init(void);

// ==================== 坐标变换函数 ====================
/**
 * @brief  Clarke变换（三相 → 两相）
 * @param  va: A相电压
 * @param  vb: B相电压
 * @param  vc: C相电压
 * @param  valpha: α轴电压指针
 * @param  vbeta: β轴电压指针
 * @retval 无
 */
void FOC_Clarke_Transform(float va, float vb, float vc, float *valpha, float *vbeta);

/**
 * @brief  Park变换（静止坐标系 → 旋转坐标系）
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @param  theta: 电角度（弧度）
 * @param  vd: d轴电压指针
 * @param  vq: q轴电压指针
 * @retval 无
 */
void FOC_Park_Transform(float valpha, float vbeta, float theta, float *vd, float *vq);

/**
 * @brief  逆Park变换（旋转坐标系 → 静止坐标系）
 * @param  vd: d轴电压
 * @param  vq: q轴电压
 * @param  theta: 电角度（弧度）
 * @param  valpha: α轴电压指针
 * @param  vbeta: β轴电压指针
 * @retval 无
 */
void FOC_InvPark_Transform(float vd, float vq, float theta, float *valpha, float *vbeta);

// ==================== SVPWM函数 ====================
/**
 * @brief  SVPWM扇区判断
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @retval 扇区号（1-6）
 */
uint8_t FOC_SVPWM_GetSector(float valpha, float vbeta);

/**
 * @brief  SVPWM矢量时间计算
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @param  sector: 扇区号
 * @param  t1: 矢量1时间指针
 * @param  t2: 矢量2时间指针
 * @param  t0: 零矢量时间指针
 * @retval 无
 */
void FOC_SVPWM_CalculateTimes(float valpha, float vbeta, uint8_t sector, 
                              float *t1, float *t2, float *t0);

/**
 * @brief  SVPWM PWM生成
 * @param  sector: 扇区号
 * @param  t1: 矢量1时间
 * @param  t2: 矢量2时间
 * @param  t0: 零矢量时间
 * @param  pwm_a: A相PWM指针
 * @param  pwm_b: B相PWM指针
 * @param  pwm_c: C相PWM指针
 * @retval 无
 */
void FOC_SVPWM_GeneratePWM(uint8_t sector, float t1, float t2, float t0,
                          uint16_t *pwm_a, uint16_t *pwm_b, uint16_t *pwm_c);

/**
 * @brief  SVPWM主函数
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @retval 无
 */
void FOC_SVPWM_Generate(float valpha, float vbeta);

// ==================== PI控制器函数 ====================
/**
 * @brief  PI控制器初始化
 * @param  pi: PI控制器结构体指针
 * @param  kp: 比例增益
 * @param  ki: 积分增益
 * @param  output_max: 输出上限
 * @param  output_min: 输出下限
 * @retval 无
 */
void FOC_PI_Init(PI_Controller_t *pi, float kp, float ki, float output_max, float output_min);

/**
 * @brief  PI控制器计算
 * @param  pi: PI控制器结构体指针
 * @param  error: 误差值
 * @retval PI控制器输出
 */
float FOC_PI_Calculate(PI_Controller_t *pi, float error);

/**
 * @brief  PI控制器重置
 * @param  pi: PI控制器结构体指针
 * @retval 无
 */
void FOC_PI_Reset(PI_Controller_t *pi);

// ==================== 控制函数 ====================
/**
 * @brief  速度环控制（闭环速度控制）
 * @param  speed_ref: 转速参考值（RPM）
 * @param  speed_actual: 实际转速（RPM）
 * @param  voltage_ref: 电压参考值指针
 * @retval 无
 */
void FOC_SpeedControl(float speed_ref, float speed_actual, float *voltage_ref);

/**
 * @brief  FOC主控制循环（闭环）
 * @param  angle: 位置角度（0-4095）
 * @param  speed_rpm: 实际转速（RPM）
 * @retval 无
 */
void FOC_MainLoop(uint16_t angle, float speed_rpm);

/**
 * @brief  设置FOC控制参数
 * @param  speed_ref: 转速参考值（RPM）
 * @param  direction: 方向（0=正转，1=反转）
 * @retval 无
 */
void FOC_SetControl(float speed_ref, uint8_t direction);

/**
 * @brief  使能FOC控制
 * @retval 无
 */
void FOC_Enable(void);

/**
 * @brief  禁用FOC控制
 * @retval 无
 */
void FOC_Disable(void);

// ==================== 工具函数 ====================
/**
 * @brief  角度转换为弧度
 * @param  angle: 角度（0-4095）
 * @retval 弧度值
 */
float FOC_AngleToRadian(uint16_t angle);

/**
 * @brief  限制电压值
 * @param  voltage: 电压值
 * @param  min_val: 最小值
 * @param  max_val: 最大值
 * @retval 限制后的电压值
 */
float FOC_LimitVoltage(float voltage, float min_val, float max_val);

/**
 * @brief  获取FOC控制状态
 * @retval FOC控制结构体指针
 */
FOC_Control_t* FOC_GetControlStatus(void);

#endif
