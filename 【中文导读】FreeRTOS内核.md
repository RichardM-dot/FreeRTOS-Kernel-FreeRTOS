# FreeRTOS-Kernel（FreeRTOS 实时操作系统内核）学习导读

> 目录：`01-RTOS操作系统/FreeRTOS-Kernel-FreeRTOS内核`
> 来源：https://github.com/FreeRTOS/FreeRTOS-Kernel （压缩包：FreeRTOS-Kernel-main.zip）

## 这是什么

FreeRTOS 是世界上使用最广泛的实时操作系统（RTOS）内核，Amazon 负责维护。它只提供"内核"（调度器、任务、队列、信号量等），不提供网络、文件系统（这些在 AWS 的 FreeRTOS+ 仓库里）。STM32、ESP32、RP2040 等几乎所有主流 MCU 的官方 SDK 都内置它。**你手上的 OV-Watch、Watchy、ExpressLRS 等项目全部用到了它，所以它是学习顺序的第一站。**

## 核心文件怎么读（建议顺序）

| 文件 | 内容 | 备注 |
| --- | --- | --- |
| `tasks.c` | 任务创建、调度、延时 | **最核心**，任务就是FreeRTOS的一切 |
| `queue.c` | 队列（任务间通信的根基） | 信号量/互斥量也是用它实现的 |
| `semphr.h` | 信号量、互斥量的宏封装 | 实现在 queue.c 里，理解这点很关键 |
| `timers.c` | 软件定时器 | 回调函数运行在"定时器服务任务"里 |
| `event_groups.c` | 事件组（多事件等待） | 用于"等多个条件同时满足" |
| `stream_buffer.c` / `message_buffer.c` | 流/消息缓冲 | 适合串口收发数据 |
| `croutine.c` | 协程（已少用） | 了解即可，小内存场景才用 |
| `include/` | 全部头文件，`FreeRTOSConfig.h` 模板在 `examples/template_configuration/` | 配置项逐个看注释 |

## 移植相关的目录

- `portable/` 下面按编译器和芯片分：例如 GCC 下的 ARM_CM4F（STM32F4 用）、ARM_CM3（F1/F2 用）、ARM_CM0（F0/树莓派 RP2040 双核之一用）。读其中一个（推荐 `portable/GCC/ARM_CM4F/port.c`），你就懂了 PendSV 中断如何做上下文切换。
- `examples/cmake_example/`：最小的 CMake 编译样例。

## 学习要点

1. **先看配置再看代码**：`FreeRTOSConfig.h` 里的 `configTOTAL_HEAP_SIZE`、`configMAX_PRIORITIES` 决定了系统形态。
2. **调度规则**：相同优先级轮流跑（时间片），高优先级永远先跑；空闲任务里回收被 `vTaskDelete` 删除的任务内存。
3. **经典坑**：中断里必须用 `FromISR` 结尾的 API；优先级翻转要用互斥量（带优先级继承）而不是二值信号量。
4. **动手实验**：不需要开发板——在 Linux 上配合 POSIX port 就能跑（参考官方 `FreeRTOS/FreeRTOS` 主仓库的 POSIX 演示），用两个任务互相发队列打印日志。

## 建议购买/准备

- 什么都不用买，纯源码阅读 + Linux 本机跑通即可。
- 若要上板：任意 STM32 板（Nucleo-F103RE / 蓝pill STM32F103C8T6，约 ¥15-30）或树莓派 Pico（¥20-30）。

## 学习目标（自测）

- [ ] 能说清 task、TCB、栈三者的关系
- [ ] 能解释一次上下文切换发生的完整过程（SysTick → PendSV → 保存寄存器 → 切换栈）
- [ ] 能用队列实现生产者-消费者
- [ ] 知道 mutex 和 binary semaphore 的区别
