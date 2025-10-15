#include "stm32f10x.h"
#include "Delay.h"
#include "USART.h"
#include "AS5600.h"

/**
  * @brief  AS5600 功能验证程序
  * @note   通过串口输出传感器数据，验证功能是否正常
  */
int main(void)
{
	AS5600_Data_t sensor_data;
	uint8_t init_status;
	uint32_t loop_count = 0;
	
	// ========== 初始化 ==========
	
	// 1. 初始化串口（115200波特率）
	USART1_Init(115200);
	Delay_ms(100);  // 等待串口稳定
	
	// 2. 打印启动信息
	USART1_SendString("\r\n");
	USART1_SendString("========================================\r\n");
	USART1_SendString("  AS5600 Magnetic Encoder Test v1.0\r\n");
	USART1_SendString("  STM32F103C8 FOC Project\r\n");
	USART1_SendString("========================================\r\n");
	USART1_SendNewLine();
	
	// 3. 初始化 AS5600
	USART1_SendString("[INFO] Initializing AS5600...\r\n");
	init_status = AS5600_Init();
	
	if(init_status == AS5600_OK)
	{
		USART1_SendString("[OK]   AS5600 initialized successfully!\r\n");
	}
	else
	{
		USART1_SendString("[ERROR] AS5600 initialization failed!\r\n");
		USART1_SendString("[ERROR] Please check:\r\n");
		USART1_SendString("        1. I2C connections (PB6=SCL, PB7=SDA)\r\n");
		USART1_SendString("        2. Pull-up resistors (4.7k ohm)\r\n");
		USART1_SendString("        3. Power supply (3.3V or 5V)\r\n");
		USART1_SendString("        4. AS5600 chip is working\r\n");
		USART1_SendNewLine();
		USART1_SendString("System halted.\r\n");
		while(1);  // 停止程序
	}
	
	// 4. 检测磁铁状态
	USART1_SendString("[INFO] Checking magnet status...\r\n");
	Delay_ms(100);
	
	uint8_t magnet_status = AS5600_CheckMagnetStatus();
	
	switch(magnet_status)
	{
		case AS5600_OK:
			USART1_SendString("[OK]   Magnet detected and position is GOOD!\r\n");
			break;
			
		case AS5600_NO_MAGNET:
			USART1_SendString("[WARN] No magnet detected!\r\n");
			USART1_SendString("[WARN] Please place a magnet above the sensor.\r\n");
			break;
			
		case AS5600_MAG_WEAK:
			USART1_SendString("[WARN] Magnet too WEAK (too far)!\r\n");
			USART1_SendString("[WARN] Please move magnet closer to sensor.\r\n");
			break;
			
		case AS5600_MAG_STRONG:
			USART1_SendString("[WARN] Magnet too STRONG (too close)!\r\n");
			USART1_SendString("[WARN] Please move magnet away from sensor.\r\n");
			break;
			
		default:
			USART1_SendString("[ERROR] Unknown magnet status!\r\n");
			break;
	}
	
	USART1_SendNewLine();
	USART1_SendString("========================================\r\n");
	USART1_SendString("  Starting continuous monitoring...\r\n");
	USART1_SendString("  Update rate: 10Hz (every 100ms)\r\n");
	USART1_SendString("========================================\r\n");
	USART1_SendNewLine();
	
	// 打印数据表头
	USART1_SendString("Time(s) | Angle(raw) | Angle(deg) | Mag  | AGC | Status | Info\r\n");
	USART1_SendString("--------|------------|------------|------|-----|--------|----------------\r\n");
	
	// ========== 主循环 ==========
	while(1)
	{
		// 读取所有传感器数据
		if(AS5600_ReadAll(&sensor_data) == AS5600_OK)
		{
			// 格式化输出数据（表格形式）
			USART1_Printf("%7.1f | %10d | %10.2f | %4d | %3d | 0x%02X   | ",
			             loop_count * 0.1f,           // 时间（秒）
			             sensor_data.raw_angle,       // 原始角度
			             sensor_data.angle_deg,       // 角度（度）
			             sensor_data.magnitude,       // 磁场强度
			             sensor_data.agc,             // AGC
			             sensor_data.status);         // 状态
			
			// 输出状态信息
			if(sensor_data.error_code == AS5600_OK)
			{
				// 检查磁场强度范围
				if(sensor_data.magnitude < 200)
					USART1_SendString("Mag LOW");
				else if(sensor_data.magnitude > 800)
					USART1_SendString("Mag HIGH");
				else if(sensor_data.magnitude >= 450 && sensor_data.magnitude <= 550)
					USART1_SendString("PERFECT!");
				else
					USART1_SendString("OK");
			}
			else
			{
				USART1_SendString(AS5600_GetErrorString(sensor_data.error_code));
			}
			
			USART1_SendNewLine();
			
			// 每10次输出后，打印一次详细诊断信息
			if(loop_count % 100 == 0 && loop_count > 0)
			{
				USART1_SendNewLine();
				USART1_SendString("--- Diagnostic Info ---\r\n");
				USART1_Printf("Total rotations: Can be calculated from angle changes\r\n");
				USART1_Printf("Magnet status bits: MD=%d ML=%d MH=%d\r\n",
				             (sensor_data.status & 0x20) ? 1 : 0,  // MD
				             (sensor_data.status & 0x10) ? 1 : 0,  // ML
				             (sensor_data.status & 0x08) ? 1 : 0); // MH
				USART1_Printf("Magnitude range: %s\r\n",
				             (sensor_data.magnitude >= 100 && sensor_data.magnitude <= 900) 
				             ? "GOOD (100-900)" : "OUT OF RANGE");
				USART1_SendString("-----------------------\r\n");
				USART1_SendNewLine();
				
				// 重新打印表头
				USART1_SendString("Time(s) | Angle(raw) | Angle(deg) | Mag  | AGC | Status | Info\r\n");
				USART1_SendString("--------|------------|------------|------|-----|--------|----------------\r\n");
			}
		}
		else
		{
			USART1_SendString("[ERROR] Failed to read AS5600!\r\n");
		}
		
		loop_count++;
		Delay_ms(100);  // 100ms更新一次（10Hz）
	}
}
