/**
  ******************************************************************************
  * @file    AS5600_Example.c
  * @brief   AS5600 磁编码器使用示例（优化版本）
  * @note    将此代码复制到 main.c 中使用
  * @version V2.0 - 优化版本（2024）
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "Delay.h"
#include "AS5600.h"

// ==================== 示例1: 基础使用（读取角度） ====================
void Example1_BasicUsage(void)
{
	uint16_t angle;
	float angle_deg;
	
	// 初始化 AS5600（会自动初始化 I2C）
	if(AS5600_Init() != AS5600_OK)
	{
		// 初始化失败（I2C通信错误或传感器不存在）
		while(1);  // 停止程序
	}
	
	// 延时等待传感器稳定
	Delay_ms(100);
	
	while(1)
	{
		// 读取原始角度（FOC推荐使用，快速响应）
		if(AS5600_GetRawAngle(&angle) == AS5600_OK)
		{
			// 转换为度数
			angle_deg = AS5600_RawToDegree(angle);
			// 现在可以使用 angle_deg（0-360度）
		}
		
		Delay_ms(10);  // 100Hz 采样率
	}
}

// ==================== 示例2: 完整诊断（检测磁铁状态） ====================
void Example2_DiagnosticCheck(void)
{
	uint8_t magnet_status;
	uint16_t magnitude;
	uint8_t agc;
	
	AS5600_Init();
	Delay_ms(100);
	
	while(1)
	{
		// 检查磁铁状态
		magnet_status = AS5600_CheckMagnetStatus();
		
		switch(magnet_status)
		{
			case AS5600_OK:
				// 磁铁位置正确，可以正常使用
				break;
				
			case AS5600_NO_MAGNET:
				// 未检测到磁铁 - 请检查硬件连接
				break;
				
			case AS5600_MAG_WEAK:
				// 磁铁太弱（距离太远）- 请靠近传感器
				break;
				
			case AS5600_MAG_STRONG:
				// 磁铁太强（距离太近）- 请远离传感器
				break;
				
			case AS5600_ERROR:
				// I2C通信错误
				break;
		}
		
		// 读取磁场强度（用于精确调试）
		if(AS5600_GetMagnitude(&magnitude) == AS5600_OK)
		{
			// magnitude 正常范围: 100-900
			// 理想值约: 500
		}
		
		// 读取 AGC（自动增益控制）
		if(AS5600_GetAGC(&agc) == AS5600_OK)
		{
			// agc 正常范围: 128 左右
		}
		
		Delay_ms(500);  // 每500ms检查一次
	}
}

// ==================== 示例3: 速度测量（FOC需要） ====================
void Example3_SpeedMeasurement(void)
{
	uint16_t angle;
	int32_t speed_rpm;
	uint32_t last_time_us = 0;
	uint32_t current_time_us = 0;
	uint32_t dt_us;
	
	AS5600_Init();
	Delay_ms(100);
	
	while(1)
	{
		// 获取当前时间（微秒）- 这里简化为1ms = 1000us
		current_time_us += 1000;  // 实际应使用定时器
		dt_us = current_time_us - last_time_us;
		
		// 读取角度
		if(AS5600_GetRawAngle(&angle) == AS5600_OK)
		{
			// 计算转速（RPM）
			speed_rpm = AS5600_CalculateSpeed(angle, dt_us);
			
			// speed_rpm 为正：正转
			// speed_rpm 为负：反转
			// 单位：RPM（转/分钟）
		}
		
		last_time_us = current_time_us;
		Delay_ms(1);  // 1kHz 采样率（FOC推荐）
	}
}

// ==================== 示例4: 一次性读取所有数据（高效） ====================
void Example4_ReadAllData(void)
{
	AS5600_Data_t sensor_data;
	
	AS5600_Init();
	Delay_ms(100);
	
	while(1)
	{
		// 一次性读取所有传感器数据
		if(AS5600_ReadAll(&sensor_data) == AS5600_OK)
		{
			// 可以使用的数据:
			// sensor_data.raw_angle    - 原始角度（0-4095）
			// sensor_data.angle        - 滤波角度（0-4095）
			// sensor_data.angle_deg    - 角度（度，0-360）
			// sensor_data.angle_rad    - 角度（弧度，0-2π）
			// sensor_data.magnitude    - 磁场强度
			// sensor_data.agc          - 自动增益控制
			// sensor_data.status       - 状态寄存器
			// sensor_data.error_code   - 错误代码
			
			// 检查是否有错误
			if(sensor_data.error_code != AS5600_OK)
			{
				// 获取错误描述
				const char* error_msg = AS5600_GetErrorString(sensor_data.error_code);
				// 可以通过串口打印 error_msg
			}
		}
		
		Delay_ms(10);
	}
}

// ==================== 示例5: 角度差值计算（处理跳变） ====================
void Example5_AngleDifference(void)
{
	uint16_t angle_old = 0;
	uint16_t angle_new = 0;
	int16_t angle_diff;
	
	AS5600_Init();
	Delay_ms(100);
	
	// 读取初始角度
	AS5600_GetRawAngle(&angle_old);
	
	while(1)
	{
		// 读取新角度
		AS5600_GetRawAngle(&angle_new);
		
		// 计算角度差（自动处理 0↔4095 跳变）
		angle_diff = AS5600_GetAngleDiff(angle_new, angle_old);
		
		// angle_diff 范围: -2048 到 2047
		// 正值: 正转
		// 负值: 反转
		
		// 示例：
		// angle_old=4090, angle_new=10 → angle_diff≈+20（正转过零点）
		// angle_old=10, angle_new=4090 → angle_diff≈-20（反转过零点）
		
		angle_old = angle_new;
		
		Delay_ms(10);
	}
}

// ==================== 主函数 ====================
int main(void)
{
	// 选择一个示例运行：
	
	// Example1_BasicUsage();           // 基础使用
	// Example2_DiagnosticCheck();      // 诊断检查
	// Example3_SpeedMeasurement();     // 速度测量
	Example4_ReadAllData();          // 一次性读取（推荐）
	// Example5_AngleDifference();      // 角度差值计算
	
	while(1);
}

