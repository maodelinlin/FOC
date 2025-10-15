# USART1 驱动使用说明

## 📋 概述

这是一个用于STM32F1的USART1串口驱动，设计风格类似于MYI2C驱动，提供了完整的发送、接收和调试功能。

---

## 🔌 硬件连接

### 引脚定义
| 功能 | STM32引脚 | 说明 |
|------|----------|------|
| TX（发送） | PA9 | 连接到USB转串口的RX |
| RX（接收） | PA10 | 连接到USB转串口的TX |
| GND | GND | 共地 |

### 连接示例
```
STM32F103C8          USB转TTL模块
  PA9  (TX) ---------> RX
  PA10 (RX) <--------- TX
  GND       <--------> GND
```

---

## ⚙️ Keil工程配置

### 1. 添加文件到工程
在Keil中右键项目 → Add Existing Files：
- `System/USART.c`
- `System/USART.h`

### 2. 配置编译器（用于printf支持）

#### 方法1：Keil MDK（推荐）
1. 在工程中勾选 **Use MicroLIB**
   - 右键工程 → Options for Target
   - Target → 勾选 **Use MicroLIB**

2. 在 `USART.c` 中已实现 `fputc()`，直接使用即可

#### 方法2：标准库
如果不使用MicroLIB，需要在 `main.c` 添加：
```c
#pragma import(__use_no_semihosting)
struct __FILE { int handle; };
FILE __stdout;
void _sys_exit(int x) { (void)x; }
```

---

## 📖 API 函数说明

### 初始化函数
```c
void USART_Init(uint32_t baudrate);
```
- **功能**: 初始化USART1
- **参数**: `baudrate` - 波特率（如115200）
- **示例**: `USART_Init(115200);`

---

### 发送函数

#### 1. 发送单个字节
```c
uint8_t USART_SendByte(uint8_t data);
```
- **返回**: `USART_SUCCESS` 或 `USART_FAIL`
- **示例**: `USART_SendByte('A');`

#### 2. 发送字符串
```c
uint8_t USART_SendString(const char *str);
```
- **示例**: `USART_SendString("Hello World\r\n");`

#### 3. 发送数组
```c
uint8_t USART_SendData(uint8_t *data, uint16_t len);
```
- **示例**: 
```c
uint8_t buffer[] = {0x01, 0x02, 0x03};
USART_SendData(buffer, 3);
```

#### 4. 格式化输出（推荐）
```c
void USART_Printf(const char *format, ...);
```
- **示例**: 
```c
int value = 123;
float temp = 25.5f;
USART_Printf("Value: %d, Temp: %.2f\r\n", value, temp);
```

#### 5. 发送十六进制（调试用）
```c
void USART_SendHex(uint8_t *data, uint16_t len);
```
- **示例**: `USART_SendHex(buffer, 3);  // 输出: "01 02 03"`

---

### 接收函数

#### 1. 阻塞接收
```c
uint8_t USART_ReceiveByte(uint8_t *data);
```
- **说明**: 等待直到接收到数据（有超时）
- **示例**:
```c
uint8_t rx_data;
if(USART_ReceiveByte(&rx_data) == USART_SUCCESS) {
    // 处理接收到的数据
}
```

#### 2. 非阻塞接收（推荐）
```c
uint8_t USART_ReceiveByteNonBlocking(uint8_t *data);
```
- **说明**: 立即返回，不等待
- **示例**:
```c
uint8_t rx_data;
if(USART_ReceiveByteNonBlocking(&rx_data) == USART_SUCCESS) {
    // 有数据
} else {
    // 无数据，继续做其他事情
}
```

---

### 状态查询函数

```c
uint8_t USART_IsTxComplete(void);       // 检查发送完成
uint8_t USART_IsRxDataAvailable(void);  // 检查有无数据
void USART_FlushRx(void);               // 清空接收缓冲
```

---

## 🎯 典型使用场景

### 场景1: 简单打印调试信息
```c
#include "USART.h"

int main(void)
{
    USART_Init(115200);
    
    USART_SendString("System Start!\r\n");
    
    while(1)
    {
        USART_Printf("Running... %d\r\n", counter++);
        Delay_ms(1000);
    }
}
```

### 场景2: AS5600传感器调试
```c
#include "USART.h"
#include "AS5600.h"

int main(void)
{
    USART_Init(115200);
    AS5600_Init();
    
    AS5600_Data_t data;
    
    while(1)
    {
        if(AS5600_ReadAll(&data) == AS5600_OK)
        {
            USART_Printf("Angle: %.2f deg, Mag: %d\r\n", 
                         data.angle_deg, 
                         data.magnitude);
        }
        Delay_ms(100);
    }
}
```

### 场景3: 简单的命令行接口
```c
int main(void)
{
    USART_Init(115200);
    AS5600_Init();
    
    while(1)
    {
        uint8_t cmd;
        if(USART_ReceiveByteNonBlocking(&cmd) == USART_SUCCESS)
        {
            if(cmd == 'a')  // 读取角度
            {
                uint16_t angle;
                AS5600_GetRawAngle(&angle);
                USART_Printf("Angle: %d\r\n", angle);
            }
            else if(cmd == 's')  // 读取状态
            {
                uint8_t status;
                AS5600_GetStatus(&status);
                USART_Printf("Status: 0x%02X\r\n", status);
            }
        }
        
        // 其他任务
        Delay_ms(10);
    }
}
```

---

## 🔧 高级功能

### 中断接收模式

#### 1. 使能中断
```c
USART_EnableRxInterrupt();
```

#### 2. 在 `stm32f10x_it.c` 中添加中断处理
```c
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t data = USART_ReceiveData(USART1);
        USART_RxCallback(data);  // 调用回调函数
    }
}
```

#### 3. 在 `main.c` 中实现回调
```c
void USART_RxCallback(uint8_t data)
{
    // 处理接收到的数据
    USART_Printf("Received: %c\r\n", data);
}
```

---

## 🐛 调试技巧

### 1. 检查硬件连接
```c
USART_Init(115200);
USART_SendString("Test\r\n");
```
- 如果串口助手收到 "Test"，硬件正常
- 如果收到乱码，检查波特率
- 如果无输出，检查TX/RX是否接反

### 2. 波特率不匹配
```c
// 如果看到乱码，尝试其他波特率
USART_SetBaudrate(9600);   // 改为9600试试
```

### 3. printf不工作
- 确保勾选了 **Use MicroLIB**
- 或者使用 `USART_Printf()` 替代

### 4. 数据打印为十六进制
```c
uint8_t data[] = {0xAA, 0xBB, 0xCC};
USART_SendHex(data, 3);  // 输出: "AA BB CC"
```

---

## 📊 性能参数

| 参数 | 值 |
|------|---|
| 最大波特率 | 460800（取决于时钟） |
| 推荐波特率 | 115200 |
| 发送超时 | 可配置（默认0xFFFF循环） |
| 接收超时 | 可配置（默认0xFFFF循环） |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |

---

## ⚠️ 注意事项

1. **TX/RX接线**: 记得交叉连接（STM32的TX接模块的RX）
2. **共地**: 必须连接GND
3. **波特率**: 双方必须一致
4. **MicroLIB**: 使用printf需要勾选
5. **缓冲区大小**: `USART_Printf()` 缓冲区为256字节
6. **中断优先级**: 如果使用中断模式，注意配置NVIC

---

## 📚 与MYI2C的相似之处

| 特性 | MYI2C | USART |
|------|-------|-------|
| **分层设计** | ✅ | ✅ |
| **错误处理** | SUCCESS/FAIL | SUCCESS/FAIL |
| **超时机制** | ✅ | ✅ |
| **清晰注释** | ✅ | ✅ |
| **易于使用** | ✅ | ✅ |

---

## 🎓 学习建议

1. **先用示例5**: `Example5_AS5600_Debug()` - 验证AS5600数据
2. **理解格式化输出**: 熟悉 `USART_Printf()` 的用法
3. **尝试命令行**: `Example7_CommandLine()` - 交互式调试
4. **数据记录**: `Example8_DataLogging()` - 导出数据到Excel分析

---

## 📞 常见问题

**Q: 为什么没有输出？**
A: 检查TX/RX是否接反，GND是否连接

**Q: 输出乱码？**
A: 检查波特率是否匹配（双方都是115200）

**Q: printf不工作？**
A: 勾选 `Use MicroLIB`，或使用 `USART_Printf()`

**Q: 如何改变波特率？**
A: `USART_SetBaudrate(9600);`

**Q: 如何使用其他串口（如USART2）？**
A: 修改代码中的 `USART1` 为 `USART2`，引脚改为PA2/PA3

---

## ✅ 总结

这个USART驱动提供了：
- ✅ 简单易用的API
- ✅ 完整的错误处理
- ✅ Printf支持（调试神器）
- ✅ 阻塞/非阻塞模式
- ✅ 中断模式（可选）
- ✅ 丰富的示例代码

**现在您可以使用串口调试AS5600传感器，为FOC开发打下基础！** 🚀

