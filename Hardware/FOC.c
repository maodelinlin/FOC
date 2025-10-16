#include "FOC.h"
#include "MS8313.h"
#include "AS5600.h"

// ==================== 静态变量 ====================
static FOC_Control_t foc_control;
static uint8_t foc_initialized = 0;

// ==================== 初始化函数 ====================

/**
 * @brief  FOC系统初始化
 * @retval 无
 */
void FOC_Init(void)
{
    // 初始化控制结构体
    foc_control.angle = 0;
    foc_control.speed_rpm = 0.0f;
    foc_control.speed_ref = 0.0f;
    foc_control.voltage_ref = 0.0f;
    foc_control.theta = 0.0f;
    foc_control.valpha = 0.0f;
    foc_control.vbeta = 0.0f;
    foc_control.vd = 0.0f;
    foc_control.vq = 0.0f;
    foc_control.pwm_a = 0;
    foc_control.pwm_b = 0;
    foc_control.pwm_c = 0;
    foc_control.enable = 0;
    foc_control.direction = 0;
    
    // 初始化速度环PI控制器
    FOC_PI_Init(&foc_control.speed_pi, PI_SPEED_KP, PI_SPEED_KI, PI_SPEED_MAX, PI_SPEED_MIN);
    
    // 初始化MS8313
    MS8313_Init();
    
    // 标记已初始化
    foc_initialized = 1;
}

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
void FOC_Clarke_Transform(float va, float vb, float vc, float *valpha, float *vbeta)
{
    // Clarke变换公式
    *valpha = va;
    *vbeta = SQRT3_INV * (va + 2.0f * vb);
}

/**
 * @brief  Park变换（静止坐标系 → 旋转坐标系）
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @param  theta: 电角度（弧度）
 * @param  vd: d轴电压指针
 * @param  vq: q轴电压指针
 * @retval 无
 */
void FOC_Park_Transform(float valpha, float vbeta, float theta, float *vd, float *vq)
{
    float cos_theta = cosf(theta);
    float sin_theta = sinf(theta);
    
    // Park变换公式
    *vd = valpha * cos_theta + vbeta * sin_theta;
    *vq = -valpha * sin_theta + vbeta * cos_theta;
}

/**
 * @brief  逆Park变换（旋转坐标系 → 静止坐标系）
 * @param  vd: d轴电压
 * @param  vq: q轴电压
 * @param  theta: 电角度（弧度）
 * @param  valpha: α轴电压指针
 * @param  vbeta: β轴电压指针
 * @retval 无
 */
void FOC_InvPark_Transform(float vd, float vq, float theta, float *valpha, float *vbeta)
{
    float cos_theta = cosf(theta);
    float sin_theta = sinf(theta);
    
    // 逆Park变换公式
    *valpha = vd * cos_theta - vq * sin_theta;
    *vbeta = vd * sin_theta + vq * cos_theta;
}

// ==================== SVPWM函数 ====================

/**
 * @brief  SVPWM扇区判断
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @retval 扇区号（1-6）
 */
uint8_t FOC_SVPWM_GetSector(float valpha, float vbeta)
{
    uint8_t sector = 0;
    
    if (vbeta >= 0.0f) {
        if (valpha >= 0.0f) {
            if (vbeta <= SQRT3 * valpha) {
                sector = 1;
            } else {
                sector = 2;
            }
        } else {
            if (vbeta <= -SQRT3 * valpha) {
                sector = 2;
            } else {
                sector = 3;
            }
        }
    } else {
        if (valpha >= 0.0f) {
            if (vbeta >= -SQRT3 * valpha) {
                sector = 6;
            } else {
                sector = 5;
            }
        } else {
            if (vbeta >= SQRT3 * valpha) {
                sector = 4;
            } else {
                sector = 5;
            }
        }
    }
    
    return sector;
}

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
                              float *t1, float *t2, float *t0)
{
    float x, y, z;
    
    // 计算中间变量
    x = vbeta;
    y = (SQRT3 * valpha - vbeta) * 0.5f;
    z = (-SQRT3 * valpha - vbeta) * 0.5f;
    
    // 根据扇区计算时间
    switch (sector) {
        case 1:
            *t1 = z;
            *t2 = y;
            break;
        case 2:
            *t1 = y;
            *t2 = -x;
            break;
        case 3:
            *t1 = -z;
            *t2 = x;
            break;
        case 4:
            *t1 = -x;
            *t2 = z;
            break;
        case 5:
            *t1 = x;
            *t2 = -y;
            break;
        case 6:
            *t1 = -y;
            *t2 = -z;
            break;
        default:
            *t1 = 0;
            *t2 = 0;
            break;
    }
    
    // 计算零矢量时间
    *t0 = FOC_PWM_PERIOD - *t1 - *t2;
    
    // 时间限制
    if (*t0 < 0) {
        float scale = FOC_PWM_PERIOD / (*t1 + *t2);
        *t1 *= scale;
        *t2 *= scale;
        *t0 = 0;
    }
}

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
                          uint16_t *pwm_a, uint16_t *pwm_b, uint16_t *pwm_c)
{
    float ta, tb, tc;
    
    // 根据扇区计算PWM时间
    switch (sector) {
        case 1:
            ta = (t1 + t2 + t0) * 0.5f;
            tb = (t2 + t0) * 0.5f;
            tc = t0 * 0.5f;
            break;
        case 2:
            ta = (t1 + t0) * 0.5f;
            tb = (t1 + t2 + t0) * 0.5f;
            tc = t0 * 0.5f;
            break;
        case 3:
            ta = t0 * 0.5f;
            tb = (t1 + t2 + t0) * 0.5f;
            tc = (t2 + t0) * 0.5f;
            break;
        case 4:
            ta = t0 * 0.5f;
            tb = (t1 + t0) * 0.5f;
            tc = (t1 + t2 + t0) * 0.5f;
            break;
        case 5:
            ta = (t2 + t0) * 0.5f;
            tb = t0 * 0.5f;
            tc = (t1 + t2 + t0) * 0.5f;
            break;
        case 6:
            ta = (t1 + t2 + t0) * 0.5f;
            tb = t0 * 0.5f;
            tc = (t1 + t0) * 0.5f;
            break;
        default:
            ta = FOC_PWM_PERIOD * 0.5f;
            tb = FOC_PWM_PERIOD * 0.5f;
            tc = FOC_PWM_PERIOD * 0.5f;
            break;
    }
    
    // 转换为PWM值
    *pwm_a = (uint16_t)ta;
    *pwm_b = (uint16_t)tb;
    *pwm_c = (uint16_t)tc;
}

/**
 * @brief  SVPWM主函数
 * @param  valpha: α轴电压
 * @param  vbeta: β轴电压
 * @retval 无
 */
void FOC_SVPWM_Generate(float valpha, float vbeta)
{
    uint8_t sector;
    float t1, t2, t0;
    uint16_t pwm_a, pwm_b, pwm_c;
    
    // 1. 扇区判断
    sector = FOC_SVPWM_GetSector(valpha, vbeta);
    
    // 2. 矢量时间计算
    FOC_SVPWM_CalculateTimes(valpha, vbeta, sector, &t1, &t2, &t0);
    
    // 3. PWM生成
    FOC_SVPWM_GeneratePWM(sector, t1, t2, t0, &pwm_a, &pwm_b, &pwm_c);
    
    // 4. 输出PWM
    MS8313_SetThreePhaseDuty(pwm_a, pwm_b, pwm_c);
    
    // 5. 更新控制结构体
    foc_control.pwm_a = pwm_a;
    foc_control.pwm_b = pwm_b;
    foc_control.pwm_c = pwm_c;
}

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
void FOC_PI_Init(PI_Controller_t *pi, float kp, float ki, float output_max, float output_min)
{
    pi->kp = kp;
    pi->ki = ki;
    pi->integral = 0.0f;
    pi->output_max = output_max;
    pi->output_min = output_min;
    pi->last_error = 0.0f;
}

/**
 * @brief  PI控制器计算
 * @param  pi: PI控制器结构体指针
 * @param  error: 误差值
 * @retval PI控制器输出
 */
float FOC_PI_Calculate(PI_Controller_t *pi, float error)
{
    float output;
    
    // 比例项
    output = pi->kp * error;
    
    // 积分项
    pi->integral += pi->ki * error;
    
    // 积分限制
    if (pi->integral > pi->output_max) {
        pi->integral = pi->output_max;
    } else if (pi->integral < pi->output_min) {
        pi->integral = pi->output_min;
    }
    
    // 总输出
    output += pi->integral;
    
    // 输出限制
    if (output > pi->output_max) {
        output = pi->output_max;
    } else if (output < pi->output_min) {
        output = pi->output_min;
    }
    
    // 保存上次误差
    pi->last_error = error;
    
    return output;
}

/**
 * @brief  PI控制器重置
 * @param  pi: PI控制器结构体指针
 * @retval 无
 */
void FOC_PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;
    pi->last_error = 0.0f;
}

// ==================== 控制函数 ====================

/**
 * @brief  速度环控制（闭环速度控制）
 * @param  speed_ref: 转速参考值（RPM）
 * @param  speed_actual: 实际转速（RPM）
 * @param  voltage_ref: 电压参考值指针
 * @retval 无
 */
void FOC_SpeedControl(float speed_ref, float speed_actual, float *voltage_ref)
{
    // 计算速度误差
    float speed_error = speed_ref - speed_actual;
    
    // PI控制器计算
    *voltage_ref = FOC_PI_Calculate(&foc_control.speed_pi, speed_error);
    
    // 限制电压范围
    *voltage_ref = FOC_LimitVoltage(*voltage_ref, FOC_MIN_VOLTAGE, FOC_MAX_VOLTAGE);
}

/**
 * @brief  FOC主控制循环（闭环）
 * @param  angle: 位置角度（0-4095）
 * @param  speed_rpm: 实际转速（RPM）
 * @retval 无
 */
void FOC_MainLoop(uint16_t angle, float speed_rpm)
{
    if (!foc_initialized || !foc_control.enable) {
        return;
    }
    
    // 1. 更新控制参数
    foc_control.angle = angle;
    foc_control.speed_rpm = speed_rpm;
    
    // 2. 计算电角度
    foc_control.theta = FOC_AngleToRadian(angle);
    
    // 3. 速度环控制（闭环）
    FOC_SpeedControl(foc_control.speed_ref, speed_rpm, &foc_control.voltage_ref);
    
    // 4. 生成三相电压指令
    float va = foc_control.voltage_ref * cosf(foc_control.theta);
    float vb = foc_control.voltage_ref * cosf(foc_control.theta - 2.0f * PI / 3.0f);
    float vc = foc_control.voltage_ref * cosf(foc_control.theta + 2.0f * PI / 3.0f);
    
    // 5. Clarke变换
    FOC_Clarke_Transform(va, vb, vc, &foc_control.valpha, &foc_control.vbeta);
    
    // 6. SVPWM生成
    FOC_SVPWM_Generate(foc_control.valpha, foc_control.vbeta);
}

/**
 * @brief  设置FOC控制参数
 * @param  speed_ref: 转速参考值（RPM）
 * @param  direction: 方向（0=正转，1=反转）
 * @retval 无
 */
void FOC_SetControl(float speed_ref, uint8_t direction)
{
    foc_control.speed_ref = speed_ref;
    foc_control.direction = direction;
    
    // 重置PI控制器
    FOC_PI_Reset(&foc_control.speed_pi);
}

/**
 * @brief  使能FOC控制
 * @retval 无
 */
void FOC_Enable(void)
{
    foc_control.enable = 1;
    MS8313_EnableOutput();
}

/**
 * @brief  禁用FOC控制
 * @retval 无
 */
void FOC_Disable(void)
{
    foc_control.enable = 0;
    MS8313_DisableOutput();
}

// ==================== 工具函数 ====================

/**
 * @brief  角度转换为弧度
 * @param  angle: 角度（0-4095）
 * @retval 弧度值
 */
float FOC_AngleToRadian(uint16_t angle)
{
    return (float)angle * 2.0f * PI / 4096.0f;
}

/**
 * @brief  限制电压值
 * @param  voltage: 电压值
 * @param  min_val: 最小值
 * @param  max_val: 最大值
 * @retval 限制后的电压值
 */
float FOC_LimitVoltage(float voltage, float min_val, float max_val)
{
    if (voltage > max_val) return max_val;
    if (voltage < min_val) return min_val;
    return voltage;
}

/**
 * @brief  获取FOC控制状态
 * @retval FOC控制结构体指针
 */
FOC_Control_t* FOC_GetControlStatus(void)
{
    return &foc_control;
}
