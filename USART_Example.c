/**
  ******************************************************************************
  * @file    USART_Example.c
  * @brief   USART1 串口使用示例（仅供参考，不要加入工程）
  * @note    将此代码复制到 main.c 中使用
  * @version V1.0
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "Delay.h"
#include "USART.h"
#include "AS5600.h"
#include <stdio.h>

// ==================== 示例1: 基础发送（Hello World） ====================
void Example1_BasicSend(void)
{
	// 初始化串口（115200波特率）
	USART_Init(115200);
	
	while(1)
	{
		// 方法1: 发送字符串
		USART_SendString("Hello World!\r\n");
		
		// 方法2: 发送单个字节
		USART_SendByte('A');
		USART_SendNewLine();
		
		// 方法3: 发送数组
		uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
		USART_SendData(data, 4);
		USART_SendNewLine();
		
		// 方法4: 发送十六进制（调试用）
		USART_SendHex(data, 4);  // 输出: "01 02 03 04"
		USART_SendNewLine();
		
		Delay_ms(1000);
	}
}

// ==================== 示例2: Printf 格式化输出 ====================
void Example2_Printf(void)
{
	USART_Init(115200);
	
	int count = 0;
	float temperature = 25.5f;
	
	while(1)
	{
		// 使用 USART_Printf（自定义函数）
		USART_Printf("Count: %d, Temp: %.2f C\r\n", count, temperature);
		
		// 也可以使用标准 printf（需要重定向）
		printf("System running: %d seconds\r\n", count);
		
		count++;
		temperature += 0.1f;
		
		Delay_ms(1000);
	}
}

// ==================== 示例3: 接收数据（阻塞） ====================
void Example3_Receive(void)
{
	USART_Init(115200);
	USART_SendString("Please send data...\r\n");
	
	uint8_t rx_data;
	
	while(1)
	{
		// 阻塞等待接收一个字节
		if(USART_ReceiveByte(&rx_data) == USART_SUCCESS)
		{
			// 回显接收到的数据
			USART_Printf("Received: 0x%02X ('%c')\r\n", rx_data, rx_data);
		}
	}
}

// ==================== 示例4: 接收数据（非阻塞） ====================
void Example4_ReceiveNonBlocking(void)
{
	USART_Init(115200);
	
	uint8_t rx_data;
	
	while(1)
	{
		// 非阻塞检查是否有数据
		if(USART_ReceiveByteNonBlocking(&rx_data) == USART_SUCCESS)
		{
			USART_Printf("Got: %c\r\n", rx_data);
		}
		
		// 可以在这里做其他事情
		Delay_ms(10);
	}
}

// ==================== 示例5: AS5600 数据打印（FOC调试必备） ====================
void Example5_AS5600_Debug(void)
{
	AS5600_Data_t sensor_data;
	
	// 初始化串口和传感器
	USART_Init(115200);
	
	if(AS5600_Init() != AS5600_OK)
	{
		USART_SendString("ERROR: AS5600 Init Failed!\r\n");
		while(1);
	}
	
	USART_SendString("AS5600 Sensor Monitor\r\n");
	USART_SendString("=====================\r\n");
	
	Delay_ms(100);
	
	while(1)
	{
		// 读取所有传感器数据
		if(AS5600_ReadAll(&sensor_data) == AS5600_OK)
		{
			// 打印角度信息
			USART_Printf("Angle: %4d (%.2f deg, %.3f rad)\r\n", 
			             sensor_data.raw_angle,
			             sensor_data.angle_deg,
			             sensor_data.angle_rad);
			
			// 打印磁场信息
			USART_Printf("Magnitude: %4d, AGC: %3d\r\n", 
			             sensor_data.magnitude,
			             sensor_data.agc);
			
			// 打印状态信息
			USART_Printf("Status: 0x%02X [%s]\r\n", 
			             sensor_data.status,
			             AS5600_GetErrorString(sensor_data.error_code));
			
			// 分隔线
			USART_SendString("---------------------\r\n");
		}
		else
		{
			USART_SendString("ERROR: Read Failed!\r\n");
		}
		
		Delay_ms(500);  // 每500ms打印一次
	}
}

// ==================== 示例6: 速度监控（FOC必需） ====================
void Example6_SpeedMonitor(void)
{
	uint16_t angle;
	int32_t speed_rpm;
	uint32_t dt_us = 10000;  // 10ms
	
	USART_Init(115200);
	AS5600_Init();
	
	USART_SendString("Motor Speed Monitor (RPM)\r\n");
	USART_SendString("========================\r\n");
	
	while(1)
	{
		// 读取角度
		if(AS5600_GetRawAngle(&angle) == AS5600_OK)
		{
			// 计算速度
			speed_rpm = AS5600_CalculateSpeed(angle, dt_us);
			
			// 打印速度和角度
			USART_Printf("Speed: %5ld RPM | Angle: %4d | ", speed_rpm, angle);
			
			// 绘制简单的速度条形图
			int bars = (speed_rpm > 0 ? speed_rpm : -speed_rpm) / 10;
			if(bars > 50) bars = 50;
			
			for(int i = 0; i < bars; i++)
			{
				USART_SendByte(speed_rpm > 0 ? '>' : '<');
			}
			USART_SendNewLine();
		}
		
		Delay_ms(10);
	}
}

// ==================== 示例7: 简单的命令行接口 ====================
void Example7_CommandLine(void)
{
	USART_Init(115200);
	AS5600_Init();
	
	USART_SendString("\r\n=== AS5600 Command Interface ===\r\n");
	USART_SendString("Commands:\r\n");
	USART_SendString("  a - Read Angle\r\n");
	USART_SendString("  s - Read Status\r\n");
	USART_SendString("  m - Read Magnitude\r\n");
	USART_SendString("  h - Show Help\r\n");
	USART_SendString("================================\r\n");
	
	uint8_t cmd;
	uint16_t angle;
	uint8_t status;
	uint16_t magnitude;
	
	while(1)
	{
		USART_SendString("\r\n> ");
		
		// 等待命令
		if(USART_ReceiveByte(&cmd) == USART_SUCCESS)
		{
			USART_SendByte(cmd);  // 回显
			USART_SendNewLine();
			
			switch(cmd)
			{
				case 'a':
				case 'A':
					if(AS5600_GetRawAngle(&angle) == AS5600_OK)
					{
						USART_Printf("Angle: %d (%.2f deg)\r\n", 
						             angle, AS5600_RawToDegree(angle));
					}
					break;
					
				case 's':
				case 'S':
					if(AS5600_GetStatus(&status) == AS5600_OK)
					{
						USART_Printf("Status: 0x%02X\r\n", status);
						USART_Printf("  Magnet Detected: %s\r\n", 
						             (status & 0x20) ? "YES" : "NO");
						USART_Printf("  Too Weak: %s\r\n", 
						             (status & 0x10) ? "YES" : "NO");
						USART_Printf("  Too Strong: %s\r\n", 
						             (status & 0x08) ? "YES" : "NO");
					}
					break;
					
				case 'm':
				case 'M':
					if(AS5600_GetMagnitude(&magnitude) == AS5600_OK)
					{
						USART_Printf("Magnitude: %d\r\n", magnitude);
						if(magnitude < 100)
							USART_SendString("  WARNING: Too weak!\r\n");
						else if(magnitude > 900)
							USART_SendString("  WARNING: Too strong!\r\n");
						else
							USART_SendString("  OK: Good range\r\n");
					}
					break;
					
				case 'h':
				case 'H':
					USART_SendString("Commands:\r\n");
					USART_SendString("  a - Read Angle\r\n");
					USART_SendString("  s - Read Status\r\n");
					USART_SendString("  m - Read Magnitude\r\n");
					USART_SendString("  h - Show Help\r\n");
					break;
					
				default:
					USART_SendString("Unknown command. Press 'h' for help.\r\n");
					break;
			}
		}
	}
}

// ==================== 示例8: 数据记录（CSV格式） ====================
void Example8_DataLogging(void)
{
	AS5600_Data_t data;
	uint32_t timestamp = 0;
	
	USART_Init(115200);
	AS5600_Init();
	
	// 打印CSV表头
	USART_SendString("Time(ms),Angle_Raw,Angle_Deg,Magnitude,AGC,Status\r\n");
	
	while(1)
	{
		if(AS5600_ReadAll(&data) == AS5600_OK)
		{
			// CSV格式输出（便于Excel分析）
			USART_Printf("%lu,%d,%.2f,%d,%d,0x%02X\r\n",
			             timestamp,
			             data.raw_angle,
			             data.angle_deg,
			             data.magnitude,
			             data.agc,
			             data.status);
		}
		
		timestamp += 100;
		Delay_ms(100);
	}
}

// ==================== 中断接收回调示例 ====================
void USART_RxCallback(uint8_t data)
{
	// 当使用中断模式时，这个函数会被自动调用
	// 示例：收到 'L' 点亮LED，收到 'l' 熄灭LED
	if(data == 'L')
	{
		// GPIO_SetBits(GPIOC, GPIO_Pin_13);  // 点亮LED
		USART_SendString("LED ON\r\n");
	}
	else if(data == 'l')
	{
		// GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // 熄灭LED
		USART_SendString("LED OFF\r\n");
	}
}

void Example9_InterruptMode(void)
{
	USART_Init(115200);
	
	// 使能接收中断
	USART_EnableRxInterrupt();
	
	USART_SendString("Interrupt Mode Ready!\r\n");
	USART_SendString("Send 'L' to turn on LED\r\n");
	USART_SendString("Send 'l' to turn off LED\r\n");
	
	while(1)
	{
		// 主循环可以做其他事情
		// 接收数据由中断处理
		Delay_ms(1000);
	}
}

// ==================== 主函数 ====================
int main(void)
{
	// 选择一个示例运行：
	
	// Example1_BasicSend();            // 基础发送
	// Example2_Printf();               // Printf输出
	// Example3_Receive();              // 阻塞接收
	// Example4_ReceiveNonBlocking();   // 非阻塞接收
	Example5_AS5600_Debug();         // AS5600调试（推荐）
	// Example6_SpeedMonitor();         // 速度监控
	// Example7_CommandLine();          // 命令行接口
	// Example8_DataLogging();          // 数据记录
	// Example9_InterruptMode();        // 中断模式
	
	while(1);
}

/**
 * 注意：如果使用中断模式（Example9），需要在 stm32f10x_it.c 中添加：
 * 
 * void USART1_IRQHandler(void)
 * {
 *     if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
 *     {
 *         uint8_t data = USART_ReceiveData(USART1);
 *         USART_RxCallback(data);
 *     }
 * }
 */

