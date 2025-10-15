#include "AS5600.h"
#include "MYI2C.h"

// ==================== 静态变量（用于速度计算） ====================
static uint16_t last_angle = 0;      // 上次角度值
static int32_t total_turns = 0;       // 总圈数（累计）

// ==================== 基础函数 ====================

/**
  * @brief  AS5600 初始化
  * @note   调用 MYI2C_Init() 初始化 I2C 总线
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_Init(void)
{
	// 初始化 I2C 总线
	MYI2C_Init();
	
	// 检测设备是否存在
	if(!AS5600_IsConnected())
	{
		return AS5600_ERROR;
	}
	
	// 读取初始角度（用于速度计算）
	uint16_t angle;
	if(AS5600_GetRawAngle(&angle) == AS5600_OK)
	{
		last_angle = angle;
	}
	
	return AS5600_OK;
}

/**
  * @brief  检测 AS5600 是否存在
  * @retval 1: 存在, 0: 不存在
  */
uint8_t AS5600_IsConnected(void)
{
	uint8_t status;
	// 尝试读取状态寄存器
	return (AS5600_GetStatus(&status) == AS5600_OK) ? 1 : 0;
}

// ==================== 角度读取函数 ====================

/**
  * @brief  读取原始角度（快速，无滤波，FOC推荐）
  * @param  angle: 角度值指针（0-4095）
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_GetRawAngle(uint16_t *angle)
{
	uint8_t data[2];
	
	if(I2C_Read(AS5600_ADDR, AS5600_REG_RAW_H, data, 2) != I2C_SUCCESS)
	{
		return AS5600_ERROR;
	}
	
	*angle = ((uint16_t)data[0] << 8) | data[1];
	*angle &= 0x0FFF;  // 只保留低 12 位
	
	return AS5600_OK;
}

/**
  * @brief  读取滤波后角度（平滑，低速推荐）
  * @param  angle: 角度值指针（0-4095）
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_GetAngle(uint16_t *angle)
{
	uint8_t data[2];
	
	if(I2C_Read(AS5600_ADDR, AS5600_REG_ANGLE_H, data, 2) != I2C_SUCCESS)
	{
		return AS5600_ERROR;
	}
	
	*angle = ((uint16_t)data[0] << 8) | data[1];
	*angle &= 0x0FFF;  // 只保留低 12 位
	
	return AS5600_OK;
}

// ==================== 角度转换函数 ====================

/**
  * @brief  将原始角度值转换为度数
  * @param  raw_angle: 原始角度（0-4095）
  * @retval 角度（度，0-360）
  */
float AS5600_RawToDegree(uint16_t raw_angle)
{
	return (float)raw_angle * 360.0f / AS5600_RESOLUTION;
}

/**
  * @brief  将原始角度值转换为弧度
  * @param  raw_angle: 原始角度（0-4095）
  * @retval 角度（弧度，0-2π）
  */
float AS5600_RawToRadian(uint16_t raw_angle)
{
	return (float)raw_angle * 2.0f * PI / AS5600_RESOLUTION;
}

/**
  * @brief  计算两个角度的差值（处理跳变）
  * @note   处理 0→4095 和 4095→0 的跳变情况
  * @param  angle1: 角度1（新角度，0-4095）
  * @param  angle2: 角度2（旧角度，0-4095）
  * @retval 角度差（-2048 到 2047）
  * 
  * 示例：
  *   angle1=10, angle2=4090 → 返回-4080 → 实际差值=+16（正转）
  *   angle1=4090, angle2=10 → 返回+4080 → 实际差值=-16（反转）
  */
int16_t AS5600_GetAngleDiff(uint16_t angle1, uint16_t angle2)
{
	int32_t diff = (int32_t)angle1 - (int32_t)angle2;
	
	// 处理跳变：如果差值超过半圈，说明发生了跳变
	if(diff > (AS5600_RESOLUTION / 2))
	{
		diff -= AS5600_RESOLUTION;  // 实际是反向转动
	}
	else if(diff < -(AS5600_RESOLUTION / 2))
	{
		diff += AS5600_RESOLUTION;  // 实际是正向转动
	}
	
	return (int16_t)diff;
}

// ==================== 速度计算 ====================

/**
  * @brief  计算电机转速（需要周期调用）
  * @note   内部会累计总圈数，可以计算多圈旋转
  * @param  current_angle: 当前角度（0-4095）
  * @param  dt_us: 距离上次调用的时间间隔（微秒）
  * @retval 转速（RPM，正值为正转，负值为反转）
  * 
  * 使用示例：
  *   uint16_t angle;
  *   AS5600_GetRawAngle(&angle);
  *   int32_t rpm = AS5600_CalculateSpeed(angle, 1000);  // 1ms调用一次
  */
int32_t AS5600_CalculateSpeed(uint16_t current_angle, uint32_t dt_us)
{
	// 计算角度差（处理跳变）
	int16_t angle_diff = AS5600_GetAngleDiff(current_angle, last_angle);
	
	// 检测是否跨过零点（累计圈数）
	if(angle_diff > (AS5600_RESOLUTION / 2))
	{
		total_turns--;  // 反向跨过零点
	}
	else if(angle_diff < -(AS5600_RESOLUTION / 2))
	{
		total_turns++;  // 正向跨过零点
	}
	
	// 更新上次角度
	last_angle = current_angle;
	
	// 计算角速度（角度/秒）
	// angle_diff: 角度变化量（单位：1/4096圈）
	// dt_us: 时间间隔（微秒）
	// 转换为 RPM = (angle_diff / 4096) * (1000000 / dt_us) * 60
	
	if(dt_us == 0) return 0;
	
	// 优化计算（避免浮点运算）
	// RPM = angle_diff * 60 * 1000000 / (4096 * dt_us)
	//     = angle_diff * 14648.4375 / dt_us
	//     ≈ angle_diff * 14648 / dt_us
	
	int32_t rpm = ((int32_t)angle_diff * 14648L) / (int32_t)dt_us;
	
	return rpm;
}

// ==================== 状态检测函数 ====================

/**
  * @brief  读取状态寄存器
  * @param  status: 状态值指针
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_GetStatus(uint8_t *status)
{
	if(I2C_ReadByte(AS5600_ADDR, AS5600_REG_STATUS, status) != I2C_SUCCESS)
	{
		return AS5600_ERROR;
	}
	return AS5600_OK;
}

/**
  * @brief  检测磁铁状态（综合判断）
  * @retval AS5600_OK: 磁铁正常
  *         AS5600_NO_MAGNET: 无磁铁
  *         AS5600_MAG_WEAK: 磁铁太弱
  *         AS5600_MAG_STRONG: 磁铁太强
  */
uint8_t AS5600_CheckMagnetStatus(void)
{
	uint8_t status;
	
	if(AS5600_GetStatus(&status) != AS5600_OK)
	{
		return AS5600_ERROR;
	}
	
	// 检测是否有磁铁
	if(!(status & AS5600_STATUS_MD))
	{
		return AS5600_NO_MAGNET;
	}
	
	// 检测磁铁强度
	if(status & AS5600_STATUS_ML)
	{
		return AS5600_MAG_WEAK;
	}
	
	if(status & AS5600_STATUS_MH)
	{
		return AS5600_MAG_STRONG;
	}
	
	return AS5600_OK;
}

/**
  * @brief  检测是否有磁铁
  * @retval 1: 检测到, 0: 未检测到
  */
uint8_t AS5600_IsMagnetDetected(void)
{
	uint8_t status;
	if(AS5600_GetStatus(&status) != AS5600_OK)
	{
		return 0;
	}
	return (status & AS5600_STATUS_MD) ? 1 : 0;
}

/**
  * @brief  磁铁位置是否正确（强度合适）
  * @retval 1: 正确, 0: 不正确
  */
uint8_t AS5600_IsMagnetOK(void)
{
	return (AS5600_CheckMagnetStatus() == AS5600_OK) ? 1 : 0;
}

// ==================== 诊断函数 ====================

/**
  * @brief  读取磁场强度
  * @param  magnitude: 磁场强度指针（0-4095）
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_GetMagnitude(uint16_t *magnitude)
{
	uint8_t data[2];
	
	if(I2C_Read(AS5600_ADDR, AS5600_REG_MAGN_H, data, 2) != I2C_SUCCESS)
	{
		return AS5600_ERROR;
	}
	
	*magnitude = ((uint16_t)data[0] << 8) | data[1];
	*magnitude &= 0x0FFF;  // 只保留低 12 位
	
	return AS5600_OK;
}

/**
  * @brief  读取 AGC（自动增益控制）
  * @param  agc: AGC值指针（0-255）
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_GetAGC(uint8_t *agc)
{
	if(I2C_ReadByte(AS5600_ADDR, AS5600_REG_AGC, agc) != I2C_SUCCESS)
	{
		return AS5600_ERROR;
	}
	return AS5600_OK;
}

/**
  * @brief  读取所有传感器数据（一次性读取，高效）
  * @param  data: 数据结构指针
  * @retval AS5600_OK: 成功, 其他: 错误代码
  */
uint8_t AS5600_ReadAll(AS5600_Data_t *data)
{
	// 读取角度
	if(AS5600_GetRawAngle(&data->raw_angle) != AS5600_OK)
	{
		data->error_code = AS5600_ERROR;
		return AS5600_ERROR;
	}
	
	if(AS5600_GetAngle(&data->angle) != AS5600_OK)
	{
		data->error_code = AS5600_ERROR;
		return AS5600_ERROR;
	}
	
	// 转换角度
	data->angle_deg = AS5600_RawToDegree(data->raw_angle);
	data->angle_rad = AS5600_RawToRadian(data->raw_angle);
	
	// 读取状态
	if(AS5600_GetStatus(&data->status) != AS5600_OK)
	{
		data->error_code = AS5600_ERROR;
		return AS5600_ERROR;
	}
	
	// 读取磁场强度
	if(AS5600_GetMagnitude(&data->magnitude) != AS5600_OK)
	{
		data->error_code = AS5600_ERROR;
		return AS5600_ERROR;
	}
	
	// 读取 AGC
	if(AS5600_GetAGC(&data->agc) != AS5600_OK)
	{
		data->error_code = AS5600_ERROR;
		return AS5600_ERROR;
	}
	
	// 检查磁铁状态
	data->error_code = AS5600_CheckMagnetStatus();
	
	// 速度需要外部周期调用 AS5600_CalculateSpeed() 计算
	data->speed_rpm = 0;
	
	return AS5600_OK;
}

/**
  * @brief  获取错误描述字符串
  * @param  error_code: 错误代码
  * @retval 错误描述字符串
  */
const char* AS5600_GetErrorString(uint8_t error_code)
{
	switch(error_code)
	{
		case AS5600_OK:         return "OK";
		case AS5600_ERROR:      return "I2C Communication Error";
		case AS5600_NO_MAGNET:  return "No Magnet Detected";
		case AS5600_MAG_WEAK:   return "Magnet Too Weak";
		case AS5600_MAG_STRONG: return "Magnet Too Strong";
		default:                return "Unknown Error";
	}
}
