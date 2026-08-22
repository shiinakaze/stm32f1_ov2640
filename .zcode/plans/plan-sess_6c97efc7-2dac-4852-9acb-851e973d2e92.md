# OV2640 驱动规范化优化方案

## 原则

- **行为保持不变**：所有寄存器最终写入值零改动（现有配置在硬件上已验证可用），只重构结构、命名、注释与错误处理
- 注释依据两份参考文档标注出处（DS V1.6 寄存器语义、App Notes 1.03 §13.2 配置来源）
- 只改动现有文件，不增删文件（避免改 Keil 工程文件）
- 编码风格对齐项目约定：snake_case、4 空格缩进、中文注释、SPL 库

## 1. `drivers/sccb.c` / `sccb.h` — SCCB 传输层

- `sw_sccb_write_reg()` 改为返回状态：`uint8_t`（0=成功，1=失败），聚合 3 字节写的 X 位应答
- `sw_sccb_read_reg()` 改为出参形式 `uint8_t sw_sccb_read_reg(uint8_t id, uint8_t sub, uint8_t *data)`，解决"数据 0xFF 与错误哨兵 0xFF 无法区分"问题
- 字节级函数（`sw_sccb_start/stop/write_byte/read_byte`、`set_input/output`）改为 `static`，头文件只暴露 `sw_sccb_init/write_reg/read_reg`
- 删除死代码：硬件 I2C2 版 `sccb_init()`（sccb.c:155-177）及 sccb.h 中注释掉的原型
- 缩进由 Tab 统一为 4 空格

## 2. `drivers/ov2640_config.c` / `ov2640_config.h` — 寄存器配置表

- **数值逐项保持不变**（SVGA 表已核对与应用笔记 §13.2 "RGB 565 Reference Setting, MCLK 24MHz SVGA 25fps" 完全一致）
- SVGA 表头部注明出处（App Notes 1.03 §13.2）+ 分组分节注释：软复位 / 传感器核心（AEC/AGC/AWB/伽马）/ 去频闪组（0x4F/0x50/0x5A/0x0C，引用 App Notes §5.3）/ 开窗 / DSP 伽马曲线（0x91/0x93/0x97）/ AWB / 尺寸缩放（0xC0/0xC1/0x51/0x52/0x5A-0x5C）/ DVP（0xD3）/ 输出格式（0xDA=0x08 RGB565）
- JPEG 表：修正误导性注释 `0xA0*2=320`（DS 表 12 说明该组寄存器为 real/4 单位），标注"实测可用的既有配置，保持原值"，并注明后续 `ov2640_set_output_size()` 会覆盖 ZMOW/ZMOH
- 头文件 extern 去掉硬编码长度 `[177]`/`[20]`，改为不定长数组 extern + 配套 `_len` 常量（在 .c 中用 `sizeof` 定义），消除长度失同步隐患
- 寄存器名宏保留（作为文档化的寄存器映射），补充分组说明注释

## 3. `drivers/ov2640.c` / `ov2640.h` — 驱动核心

**API 重构（ov2640.h）**：
- 新增错误码 `typedef enum { OV2640_OK, OV2640_ERR_SCCB, OV2640_ERR_PID }`
- `ov2640_init()` 返回错误码；`ov2640_get_pid/mid` 改为出参+状态形式
- 删除未使用的 `#include <stdio.h>`；`delay.h`/`sccb.h` 下移到 .c；删除 `SCCB_Write/SCCB_Read` 函数式宏（下放为 .c 内静态函数）
- 头文件完整声明公开 API，Doxygen 风格 `@brief/@param/@retval`

**ov2640.c 内部结构**：
- 新增静态底层封装 `ov2640_write_reg(bank, reg, val)` / `ov2640_read_reg(bank, reg, *val)`：内部处理 0xFF（RA_DLMT）寄存器组切换（DS §8），替代散落的裸写
- `ov2640_init_config` 改名 `ov2640_load_config` 并转为 static
- `ov2640_init()` 流程规范化：
  1. `io_init`（含显式拉低 PWDN，不再依赖上电默认值）→ 硬件复位 → SCCB 初始化
  2. **先读 PID 校验**（PIDH==0x26，带重试；DS 表 13 规定 PID=0x26/0x41，MID=0x7F/0xA2）——失败立即返回，不再对空总线做 197 次无用写入
  3. 下载 SVGA 基础表 → JPEG 模式表
  4. 尾部写寄存器改用宏（`OV2640_SENSOR_CLKRC` 删除冗余重复写、`OV2640_DSP_R_DVP_SP`）并注释 0x64 的原因：手动分频降低 PCLK 以匹配 GPIO 轮询采集速度（覆盖表中的 0x82 自动模式，此为有意行为）
  5. 末尾回读 0xDA 校验 ==0x10（JPEG 模式生效哨兵检查）
  - 删除死代码：`ov2640_set_power_down_mode`、注释掉的 `atk_mc2640_*` 行
  - `RCC_APB_OV2640` 移除无人使用的 GPIOC（已全项目 grep 确认），io_init 合并同类 GPIO_Init 调用；注明 PB3/PB4 依赖 i2c.c 中的 JTAG 禁用重映射（现存顺序依赖，只文档化不搬移）
- `ov2640_capture()` 拆分为三个静态函数：VSYNC 同步等待（带超时，约 1~2 秒量级的轮询计数上限，超时置空帧返回——仅改变"摄像头拔掉时永久挂死"的故障场景行为）、帧字节读出、JPEG SOI/EOI 标记定位
- 输出尺寸函数注释标注 DS 表 12 的 real/4 单位语义及 ZMHH 位域布局

## 4. `app/main.c` — 适配调用点

- `ov2640_init()` 失败时 OLED 显示错误信息并停机（现状：无传感器时继续跑，流出垃圾数据）
- PID/MID 显示改用新出参 API，逻辑不变

## 5. `middleware/delay.c` — 定时常数修正（配套 Bug 修复）

- `MCU_SYSCLK` 128 → 72（系统实为 72MHz：`SYSCLK_FREQ_72MHz` 已定义，main.c:36 注释与 docs/app-manual.md 均确认），当前所有延时偏慢 1.78 倍
- 修正后 SCCB 时钟约 100kHz，仍远低于 DS 表 9 规定的 400kHz 上限，安全

## 验证

1. 定位本机 Keil UV4（注册表/常见路径搜索），命令行 `UV4 -b stm32f1_ov2640.uvprojx -j0` 编译，确保 0 Error 0 Warning 级别回归
2. grep 复核无残留的旧 API 引用（`SCCB_Write`、`ov2640_init_config` 等）
3. 无硬件实测条件 → 这就是采用"寄存器值零改动"保守策略的原因，最终行为回归依赖你上板验证

## 已知问题（仅记录，不在本次修改）

- JPEG 表 HSIZE/VSIZE（0x51=0xA0/0x52=0x78）与 DS "real/4" 单位描述不一致（按手册应为 0x50/0x3C，且 set_output_size 只覆盖 ZMO 不覆盖 HSIZE），但该配置实测可用——保持原值并在注释与总结中说明，供后续上板实验
- `drivers/sht30.c` 为 0 字节空文件仍在编译（超出本次范围）