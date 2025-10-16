#include "stm32f10x.h"
#include "Delay.h"
#include "FOC.h"
#include "AS5600.h"
#include "USART.h"

/**
  * @brief  FOC智能车控制程序
  * @note   实现开环FOC控制，无ADC电流采样
  */
int main(void)
{
	// ========== 初始化 ==========
	
	// 1. 初始化延时系统
	Delay_Init();
	
	// 2. 初始化串口调试
	USART1_Init(115200);
	USART1_Printf("FOC System Starting...\r\n");
	
	// 3. 初始化FOC系统
	FOC_Init();
	USART1_Printf("FOC System Initialized!\r\n");
	
	// 4. 初始化AS5600位置传感器
	if (AS5600_Init() != AS5600_OK) {
		USART1_Printf("AS5600 Init Failed!\r\n");
		while(1);
	}
	
	// 检测AS5600连接
	if (!AS5600_IsConnected()) {
		USART1_Printf("AS5600 Not Connected!\r\n");
		while(1);
	}
	
	USART1_Printf("AS5600 Connected Successfully!\r\n");
	
	// 5. 使能FOC控制
	FOC_Enable();
	USART1_Printf("FOC Control Enabled!\r\n");
	
	// 6. 设置控制参数（先用低转速测试）
	FOC_SetControl(100.0f, 0);  // 100RPM转速，正转
	USART1_Printf("FOC Control Parameters Set: 100 RPM\r\n");
	
	USART1_Printf("System Ready! Starting FOC Control...\r\n\r\n");
	
	// ========== 主循环：FOC控制 ==========
	uint32_t last_time = 0;
	uint16_t angle = 0;
	float speed_rpm = 0.0f;
	
	while(1)
	{
		uint32_t current_time = Delay_GetTick();
		
		// 1ms控制周期
		if (current_time - last_time >= 1)
		{
			// 读取位置
			AS5600_GetRawAngle(&angle);
			
			// 计算速度（简化版）
			static uint16_t last_angle = 0;
			int16_t angle_diff = AS5600_GetAngleDiff(angle, last_angle);
			speed_rpm = (float)angle_diff * 60.0f * 1000.0f / 4096.0f;  // RPM
			last_angle = angle;
			
			// FOC主控制循环
			FOC_MainLoop(angle, speed_rpm);
			
			// 更新计时
			last_time = current_time;
		}
		
		// 串口输出调试信息（每100ms）
		static uint32_t debug_time = 0;
		if (current_time - debug_time >= 100)
		{
			FOC_Control_t* status = FOC_GetControlStatus();
			
			USART1_Printf("=== FOC Debug Info ===\r\n");
			USART1_Printf("Angle: %d, Speed: %.1f RPM, Ref: %.1f RPM\r\n", 
						   angle, speed_rpm, status->speed_ref);
			USART1_Printf("Voltage: %.2f V, Enable: %d\r\n", 
						   status->voltage_ref, status->enable);
			USART1_Printf("PWM: A=%d, B=%d, C=%d\r\n", 
						   status->pwm_a, status->pwm_b, status->pwm_c);
			USART1_Printf("Theta: %.3f rad, Valpha: %.3f, Vbeta: %.3f\r\n", 
						   status->theta, status->valpha, status->vbeta);
			USART1_Printf("=====================\r\n\r\n");
			
			debug_time = current_time;
		}
	}
}
