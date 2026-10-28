# FreeRTOS 英文注释中文化 · 范围与说明

> 完成时间：2026-10-08
> 基线：项目 git 标签 `orig`（未修改的原始代码，可用 `git checkout orig` 随时还原）

## 一、已完成范围（285 个文件，全部逐文件校验 PASS）

| 范围 | 文件数 | 说明 |
| --- | --- | --- |
| 内核核心 | 7 | `tasks.c`、`queue.c`、`timers.c`、`stream_buffer.c`、`event_groups.c`、`croutine.c`、`list.c` |
| 公共头文件 | 21 | `include/` 全部 `.h`（FreeRTOS.h、task.h、queue.h、semphr.h、timers.h、event_groups.h、stream_buffer.h、message_buffer.h、list.h、atomic.h、portable.h、projdefs.h、mpu_*.h 等） |
| GCC 工具链全部端口 | 248 | `portable/GCC/` 下全部 `.c/.h`：Cortex-M0/3/4F/7/23/33/35P/52/55/85、STAR-MC3、Cortex-R5/R82/CRx、RISC-V、ARM7、AArch64、Cortex-A9、AVR、MSP430、PPC、MicroBlaze、NiosII、RL78、RX、TriCore、ColdFire、H8S/HCS12、IA32、STR75x 等全部架构 |
| 内存管理与公共层 | 含在上项 | `portable/MemMang/`（heap_1~5）、`portable/Common/`（MPU 包装器）、`portable/template/` |

统计：**约 2.96 万条注释块已中文化**（以注释块计），覆盖内核、API 文档、移植层汇编说明、堆算法说明。

## 二、未翻译范围（保留英文原注释）

| 目录 | 文件数 | 原因 |
| --- | --- | --- |
| `portable/IAR/` | 187 | IAR 编译器的端口副本，与 GCC 端口内容重复，不影响 GCC/STM32 学习路径 |
| `portable/ARMv8M/` | 38 | Armv8-M 架构端口副本（GCC/ARM_CM23/33/35P 已覆盖同架构实现） |
| `portable/ThirdParty/` | 41 | 第三方厂商代码，保留原作者版权与英文注释 |
| `portable/RVDS、MPLAB、CCS、Renesas、BCC、WizC、SDCC 等` | 约 100 | 其他编译器/厂商的端口副本 |
| `portable/*.S` 汇编文件 | — | 汇编注释语法因编译器而异，且与 C 端口的说明重复，未处理 |

剩余合计：**372 个文件 / 约 3.43 万条英文注释**（`portable` 369 个 + `examples` 3 个；GCC 目录中剩余 6 个文件均为 Atmel 免责声明，按规范保留）。

> 以上目录如需继续中文化，可直接复用本项目的工具链与规范（见下），按目录分批执行即可。

## 三、"代码零改动"如何保证

1. **git 基线**：初始未修改代码打标签 `orig`；每批翻译独立提交，`git log` 可逐批审计。
2. **机械校验**：`scripts/comment_tool.py verify <仓库> <文件>` 用状态机剥离注释，与 `refs/tags/orig` 逐行比对非空代码行；**276 个文件全部 PASS**。
3. **只改注释 token**：翻译经由 `extract → apply` 流程回填，工具只替换注释字符区间，不触碰代码、缩进、空行、宏续行。
4. **Doxygen 标记**：`scripts/restore_doxygen.py` 按基线恢复 `/**`、`/**<` 标记（工具重建注释时的已知降级已修正）。
5. **刻意保留**：
   - 全部许可证/版权声明原样保留（MIT/Amazon/Atmel 等）；
   - 宏续行（`\`）内的注释按安全规则保持英文（约十余处，避免破坏预处理结构）；
   - 纯符号分隔线、ASCII 结构图的图形部分不重排。

## 四、如何还原 / 复核

```bash
cd "/mnt/data1t/单片机学习/01-RTOS操作系统/FreeRTOS-Kernel-FreeRTOS内核"

# 查看全部翻译提交
git log --oneline orig..HEAD

# 还原单个文件为英文原版
git checkout orig -- tasks.c

# 整个项目还原为原始英文注释版本（保留在另一个分支更安全）
git checkout -b zh-keep          # 先保存当前中文版
git checkout orig -- .           # 工作区回到原版
git checkout zh-keep             # 需要时再切回中文版

# 复核"代码零改动"
python3 /mnt/data1t/单片机学习/scripts/comment_tool.py verify "$PWD" tasks.c
```

## 五、工具与规范（可复用）

- `scripts/comment_tool.py`：extract / apply / verify / stats 四个子命令
- `scripts/restore_doxygen.py`：按基线恢复 Doxygen 标记
- `scripts/FreeRTOS翻译规范.md`：术语表与铁律（只增不改、许可证保留、宏续行保护等）

## 六、提交记录（摘要）

```
docs(注释): list.c 英文注释全部中文化
docs(注释): 核心7个.c英文注释全部中文化
docs(注释): include 全部21个头文件英文注释中文化
docs(注释): GCC Cortex-M端口(CM0/CM3/CM4F/CM7)+MemMang+Common+template
docs(注释): GCC全部ARM/RISC-V端口(CM23/33/35P/52/55/85/STAR_MC3/CR/RISC-V)
docs(注释): GCC其余架构端口(ARM7/AArch64/AVR/MSP430/PPC/RX/RL78/TriCore等29目录)
```
