# 按 AN-SDK-001 规范重组 stm32f1_ov2640 目录结构

## 目标结构（映射自规范 `arch/chip/boards/drivers/{peripheral,device}/middleware/app`）

```
stm32f1_ov2640/
├── arch/arm/cortex-m3/            core_cm3.c/.h                          ← drivers/cmsis/core
├── chip/stm32f103/
│   ├── stm32f10x.h                                                        ← drivers/cmsis/device
│   ├── startup/startup_stm32f10x_md.s                                     ← .../device/startup
│   └── system/system_stm32f10x.c/.h                                       ← .../device
├── boards/bluepill/
│   └── board_config.h             新建：从驱动头文件抽出的引脚/外设选择宏
├── drivers/
│   ├── peripheral/
│   │   ├── spl/{inc,src}          ST 标准外设库（vendor，原样整体移动）
│   │   ├── uart/                  uart.c/.h（USART1+DMA1_CH4 传输）
│   │   └── i2c/                   i2c.c/.h + sccb.c/.h（软件 I2C / SCCB 总线实现）
│   └── device/
│       ├── camera/sensor/ov2640/  ov2640.c/.h + ov2640_config.c/.h
│       ├── display/oled/          oled.c/.h + oled_font.c/.h
│       └── sensor/sht30/          sht30.c/.h（空占位桩）
├── middleware/
│   ├── utils/                     delay.c/.h                              ← middleware 根目录
│   └── usb/{inc,src,uvc}          不动（ST USB 库 + UVC 类层）
├── app/                           不动（main、stm32f10x_it、stm32f10x_conf.h）
├── examples/vendor/               Projects/ + Utilities/（厂商参考代码整体迁入，不参与编译）
├── tools/                         不动
├── docs/                          不动（内容中的路径需更新）
└── build/                         Keil 输出重定向：build/obj + build/map（gitignore）
```

`kernel/`、`tests/`、`examples/`（自身示例）本工程暂无内容，不创建空占位目录，在 README 中说明预留。

## 实施步骤

1. **文件移动（全部用 `git mv` 保留历史）**：按上表迁移 CMSIS/SPL/驱动/中间件/厂商参考共约 60 个编译内文件与 `Projects/`、`Utilities/` 两棵参考树。
2. **boards 抽取**：新建 `boards/bluepill/board_config.h`，把 `uart.h`、`i2c.h`、`ov2640.h`、`sccb.h`、`sht30.h` 中的板级宏（RCC/GPIO 端口/引脚/DMA 通道/总线速率选择）迁入；各驱动头文件改为 `#include "board_config.h"`。派生用法宏（如 `SW_I2C_SCL()`、`OV2640_READ_DATA()`）留在驱动内，纯宏搬移、值不变。
3. **更新 Keil 工程 `stm32f1_ov2640.uvprojx`**：
   - IncludePath 改为：`.\app;.\boards\bluepill;.\arch\arm\cortex-m3;.\chip\stm32f103;.\chip\stm32f103\system;.\chip\stm32f103\startup;.\drivers\peripheral\spl\inc;.\drivers\peripheral\uart;.\drivers\peripheral\i2c;.\drivers\device\camera\sensor\ov2640;.\drivers\device\display\oled;.\drivers\device\sensor\sht30;.\middleware\utils;.\middleware\usb\inc;.\middleware\usb\uvc`
   - 全部 40 个 `<FilePath>` 按新路径改写；Group 重组为 app / boards / arch / chip / drivers\peripheral\\* / drivers\device\\* / middleware\\* 镜像新结构
   - 输出目录 `.\Objects\`→`.\build\obj\`，`.\Listings\`→`.\build\map\`
4. **同步更新 `stm32f1_ov2640.uvoptx`**：所有 `<PathWithFileName>` 与 `<GroupName>` 一致改写（调试器配置不动）。
5. **外围配置**：`.vscode/c_cpp_properties.json` 的 includePath 镜像更新；`tools/memory_analyzer.py` 的 `search_dirs` 增加 `build/obj`、`build/map`；`.gitignore` 以 `build/` 替换 `Objects`/`Listings`（保留 DebugConfig、RTE）。
6. **文档更新**：`README.md`、`README.zh-CN.md`、`docs/plan.md`、`docs/app-manual.md` 中的目录树与文件路径引用改为新结构，并补一段与新分层的对应说明。
7. **验证**：
   - 命令行编译：`"C:/Users/shiinakaze/AppData/Local/Keil_v5/UV4/UV4.exe" -b stm32f1_ov2640.uvprojx -j0 -o build.log`，要求 0 Error、告警数不高于现状，产物落在 `build/obj/`
   - 全仓库 grep 旧路径残留（`drivers/cmsis`、`drivers/spl`、`middleware/delay`、`middleware/usb/Projects` 等）
   - `git status` 确认改动仅为预期的 rename 与编辑

## 说明与边界

- 源码 `#include` 均为平铺搜索名，靠 IncludePath 解析，移动后无需改 include 语句（boards 抽取除外）。
- `middleware/usb` 的 ST 库（inc/src/uvc）保持 vendor 布局不动。
- 默认**不做 git commit**，重组完成并编译验证后汇报，由你决定提交。
- 风险点：uvprojx/uvoptx 为 XML，改写时保持结构有效；boards 宏抽取可被编译完全验证，宏值原样搬移不影响运行行为。