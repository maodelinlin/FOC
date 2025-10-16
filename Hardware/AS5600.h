#ifndef __AS5600_H
#define __AS5600_H

#include <stdint.h>

// ==================== 硬件配置 ====================
// AS5600 I2C 地址（7位）
#define AS5600_ADDR         0x36

// AS5600 寄存器地址
#define AS5600_REG_ZMCO     0x00  // ZMCO - 编程次数（只读）
#define AS5600_REG_ZPOS_H   0x01  // 零位高字节
#define AS5600_REG_ZPOS_L   0x02  // 零位低字节
#define AS5600_REG_MPOS_H   0x03  // 最大位置高字节
#define AS5600_REG_MPOS_L   0x04  // 最大位置低字节
#define AS5600_REG_MANG_H   0x05  // 最大角度高字节
#define AS5600_REG_MANG_L   0x06  // 最大角度低字节
#define AS5600_REG_CONF_H   0x07  // 配置高字节
#define AS5600_REG_CONF_L   0x08  // 配置低字节
#define AS5600_REG_RAW_H    0x0C  // 原始角度高字节
#define AS5600_REG_RAW_L    0x0D  // 原始角度低字节
#define AS5600_REG_ANGLE_H  0x0E  // 滤波后角度高字节
#define AS5600_REG_ANGLE_L  0x0F  // 滤波后角度低字节
#define AS5600_REG_STATUS   0x0B  // 状态寄存器
#define AS5600_REG_AGC      0x1A  // 自动增益控制
#define AS5600_REG_MAGN_H   0x1B  // 磁场强度高字节
#define AS5600_REG_MAGN_L   0x1C  // 磁场强度低字节

// ==================== 常量定义 ====================
#define AS5600_RESOLUTION   4096    // 12位分辨率
#define AS5600_MAX_ANGLE    4095    // 最大角度值
#define PI                  3.14159265358979f

// 状态寄存器位定义
#define AS5600_STATUS_MD    (1 << 5)  // 检测到磁铁
#define AS5600_STATUS_ML    (1 << 4)  // 磁铁太弱
#define AS5600_STATUS_MH    (1 << 3)  // 磁铁太强

// 磁场强度参考值（经验值，需根据实际调整）
#define AS5600_MAG_MIN      100     // 磁场强度最小值
#define AS5600_MAG_MAX      900     // 磁场强度最大值
#define AS5600_MAG_IDEAL    500     // 理想磁场强度

// 返回值定义
#define AS5600_OK           0       // 操作成功
#define AS5600_ERROR        1       // 通信错误
#define AS5600_NO_MAGNET    2       // 无磁铁
#define AS5600_MAG_WEAK     3       // 磁铁太弱
#define AS5600_MAG_STRONG   4       // 磁铁太强

// ==================== 数据结构 ====================
/**
 * @brief AS5600 完整状态结构体
 */
typedef struct {
    uint16_t raw_angle;       // 原始角度（0-4095）
    uint16_t angle;           // 滤波角度（0-4095）
    float angle_deg;          // 角度（度）
    float angle_rad;          // 角度（弧度）
    int32_t speed_rpm;        // 转速（RPM，需调用计算函数）
    uint16_t magnitude;       // 磁场强度
    uint8_t agc;              // 自动增益控制
    uint8_t status;           // 状态寄存器
    uint8_t error_code;       // 错误代码
} AS5600_Data_t;

// ==================== 基础函数 ====================
/**
 * @brief  AS5600 初始化
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_Init(void);

/**
 * @brief  检测 AS5600 是否存在
 * @retval 1: 存在, 0: 不存在
 */
uint8_t AS5600_IsConnected(void);

// ==================== 角度读取函数 ====================
/**
 * @brief  读取原始角度（快速，无滤波，FOC推荐）
 * @param  angle: 角度值指针（0-4095）
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_GetRawAngle(uint16_t *angle);

/**
 * @brief  读取滤波后角度（平滑，低速推荐）
 * @param  angle: 角度值指针（0-4095）
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_GetAngle(uint16_t *angle);

// ==================== 角度转换函数 ====================
/**
 * @brief  将原始角度值转换为度数
 * @param  raw_angle: 原始角度（0-4095）
 * @retval 角度（度，0-360）
 */
float AS5600_RawToDegree(uint16_t raw_angle);

/**
 * @brief  将原始角度值转换为弧度
 * @param  raw_angle: 原始角度（0-4095）
 * @retval 角度（弧度，0-2π）
 */
float AS5600_RawToRadian(uint16_t raw_angle);

/**
 * @brief  计算两个角度的差值（处理跳变）
 * @param  angle1: 角度1（0-4095）
 * @param  angle2: 角度2（0-4095）
 * @retval 角度差（-2048 到 2047）
 */
int16_t AS5600_GetAngleDiff(uint16_t angle1, uint16_t angle2);

// ==================== 速度计算 ====================
/**
 * @brief  计算电机转速（需要周期调用）
 * @param  current_angle: 当前角度（0-4095）
 * @param  dt_us: 距离上次调用的时间间隔（微秒）
 * @retval 转速（RPM，正值为正转，负值为反转）
 */
int32_t AS5600_CalculateSpeed(uint16_t current_angle, uint32_t dt_us);

/**
 * @brief  获取累计圈数
 * @param  无
 * @retval 累计圈数（正值为正转，负值为反转）
 */
int32_t AS5600_GetTotalTurns(void);

/**
 * @brief  获取累计角度（浮点数）
 * @param  无
 * @retval 累计角度（圈数，浮点数）
 */
float AS5600_GetTotalAngle(void);

// ==================== 状态检测函数 ====================
/**
 * @brief  读取状态寄存器
 * @param  status: 状态值指针
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_GetStatus(uint8_t *status);

/**
 * @brief  检测磁铁状态（综合判断）
 * @retval AS5600_OK/NO_MAGNET/MAG_WEAK/MAG_STRONG
 */
uint8_t AS5600_CheckMagnetStatus(void);

/**
 * @brief  检测是否有磁铁
 * @retval 1: 检测到, 0: 未检测到
 */
uint8_t AS5600_IsMagnetDetected(void);

/**
 * @brief  磁铁位置是否正确（强度合适）
 * @retval 1: 正确, 0: 不正确
 */
uint8_t AS5600_IsMagnetOK(void);

// ==================== 诊断函数 ====================
/**
 * @brief  读取磁场强度
 * @param  magnitude: 磁场强度指针（0-4095）
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_GetMagnitude(uint16_t *magnitude);

/**
 * @brief  读取 AGC（自动增益控制）
 * @param  agc: AGC值指针（0-255）
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_GetAGC(uint8_t *agc);

/**
 * @brief  读取所有传感器数据（一次性读取）
 * @param  data: 数据结构指针
 * @retval AS5600_OK: 成功, 其他: 错误代码
 */
uint8_t AS5600_ReadAll(AS5600_Data_t *data);

/**
 * @brief  获取错误描述字符串
 * @param  error_code: 错误代码
 * @retval 错误描述字符串
 */
const char* AS5600_GetErrorString(uint8_t error_code);

#endif
