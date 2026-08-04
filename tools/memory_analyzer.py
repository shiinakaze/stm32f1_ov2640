#!/usr/bin/env python3
"""
STM32 内存分析专业工具 — 解析 Keil MDK 构建输出，生成 HTML 可视化报告。

用法:
    python memory_analyzer.py <build_log.txt> [选项]
    python memory_analyzer.py --project .     # 自动发现项目文件
    python memory_analyzer.py --help

功能:
    - 解析 Keil MDK 构建日志 (Program Size)
    - 解析 .map 文件: 模块级 + 执行区域 (ER_IROM1/RW_IRAM1) + 栈/堆
    - 自动检测 MCU 型号
    - HTML 报告: 深色模式、打印友好、选项卡导航、Chart.js 图表
    - JSON/CSV 导出
    - 可配置告警阈值
"""

import argparse
import csv
import json
import os
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Optional, Dict, List, Any


# ============================================================
#  国际化 (i18n) — 中文 / English
# ============================================================
# 当前界面语言，由 main() 根据 --lang 设置。字符串中的 {...} 占位符由 _tr() 填充。
LANG: str = "zh"

LOCALES: Dict[str, Dict[str, str]] = {
    "zh": {
        "html_lang": "zh-CN",
        "report_title": "STM32 内存分析报告",
        "dark_mode": "深色模式",
        "toggle_dark_mode": "切换深色模式",
        "print": "打印",
        "print_report": "打印报告",
        "export_json": "导出 JSON",
        "export_csv": "导出 CSV",
        "generated_at": "生成时间",
        "alert_thresholds": "告警阈值",
        "flash_usage": "Flash 占用",
        "sram_usage": "SRAM 占用",
        "executable_code": "可执行代码",
        "bss_stack_heap": "BSS + 栈 + 堆",
        "stack": "栈",
        "defined_in_startup": "startup 中定义",
        "heap": "堆",
        "dynamic_memory": "动态内存",
        "overview": "概览",
        "details": "数据明细",
        "modules": "模块",
        "regions": "区域",
        "storage_distribution": "存储分布",
        "memory_details": "内存详细数据",
        "type": "类型",
        "size": "大小",
        "percentage": "百分比",
        "description": "说明",
        "executable_code_flash": "可执行代码，位于 Flash",
        "readonly_data": "只读数据 (const、字符串)",
        "rw_data_dual": "已初始化读写数据，Flash+SRAM 双占用",
        "zi_data_sram": "零初始化数据 (BSS/堆/栈)，仅 SRAM",
        "flash_total": "Flash 总计",
        "sram_total": "SRAM 总计",
        "stack_space": "调用栈空间",
        "heap_space": "动态分配空间",
        "module_flash_top12": "模块 Flash 占用 Top 12",
        "module_detail": "模块内存明细",
        "module": "模块",
        "no_map_module_data": "未提供 .map 文件，无模块级数据。",
        "no_region_data": "未解析到执行区域数据。",
        "sram_breakdown": "SRAM 细分",
        "region": "区域",
        "bss_uninit": ".bss (未初始化全局变量)",
        "data_init": ".data (已初始化全局变量)",
        "stack_call": "Stack (调用栈)",
        "heap_dynamic": "Heap (动态内存)",
        "used": "已用",
        "base_addr": "基地址",
        "size_bytes": "大小 (Bytes)",
        "proportion": "占比",
        "execution_region": "执行区域",
        "others": "其他 {n} 模块",
        "no_module_data": "(无模块数据)",
        "sram_critical": "SRAM 使用率已达 {pct}%，接近硬件极限！请立即优化内存分配。",
        "sram_warning": "SRAM 使用率 {pct}%，建议关注内存余量，避免后续功能无法添加。",
        "flash_critical": "Flash 使用率已达 {pct}%，超出安全范围！",
        "flash_warning": "Flash 使用率 {pct}%，剩余空间有限。",
        # 控制台消息
        "err_read_build_log": "无法读取构建日志",
        "err_read_map": "无法读取 .map 文件",
        "ok_html": "HTML 报告已生成",
        "ok_json": "JSON 导出",
        "ok_csv": "CSV 导出",
        "warn_no_csv": "无模块数据可导出 CSV。",
        "info_parsed": "解析到",
        "info_detected_mcu": "自动检测 MCU",
        "warn_default_mcu": "未指定 MCU，使用默认",
        "err_unknown_mcu": "未知 MCU",
        "err_unknown_mcu_hint": "使用 --list-mcus 查看支持列表。",
        "info_found_map": "自动发现 .map",
        "info_parsed_map": "解析 .map",
        "info_modules": "个模块",
        "info_regions": "个执行区域",
        "warn_parse_map": "无法解析 .map 文件，将跳过模块/区域分析",
        "warn_no_map_csv": "无 .map 数据，无法导出 CSV。",
        "err_need_build_log": "未指定 build_log 且无法在项目中自动发现。",
        "err_need_build_log_arg": "请指定 build_log 文件或 --project 目录。",
        "err_parse_build_log": "无法从 {path} 中解析 Program Size 行。",
        "supported_mcus": "支持的 MCU 型号",
    },
    "en": {
        "html_lang": "en",
        "report_title": "STM32 Memory Analysis Report",
        "dark_mode": "Dark mode",
        "toggle_dark_mode": "Toggle dark mode",
        "print": "Print",
        "print_report": "Print report",
        "export_json": "Export JSON",
        "export_csv": "Export CSV",
        "generated_at": "Generated",
        "alert_thresholds": "Alert thresholds",
        "flash_usage": "Flash Usage",
        "sram_usage": "SRAM Usage",
        "executable_code": "Executable code",
        "bss_stack_heap": "BSS + stack + heap",
        "stack": "Stack",
        "defined_in_startup": "Defined in startup",
        "heap": "Heap",
        "dynamic_memory": "Dynamic memory",
        "overview": "Overview",
        "details": "Details",
        "modules": "Modules",
        "regions": "Regions",
        "storage_distribution": "Storage distribution",
        "memory_details": "Memory detail",
        "type": "Type",
        "size": "Size",
        "percentage": "Percentage",
        "description": "Description",
        "executable_code_flash": "Executable code, in Flash",
        "readonly_data": "Read-only data (const, strings)",
        "rw_data_dual": "Initialized read-write data, occupies Flash + SRAM",
        "zi_data_sram": "Zero-initialized data (BSS/heap/stack), SRAM only",
        "flash_total": "Flash total",
        "sram_total": "SRAM total",
        "stack_space": "Call stack space",
        "heap_space": "Dynamically allocated space",
        "module_flash_top12": "Module Flash usage Top 12",
        "module_detail": "Module memory detail",
        "module": "Module",
        "no_map_module_data": "No .map file provided, no module-level data.",
        "no_region_data": "No execution region data parsed.",
        "sram_breakdown": "SRAM breakdown",
        "region": "Region",
        "bss_uninit": ".bss (uninitialized globals)",
        "data_init": ".data (initialized globals)",
        "stack_call": "Stack (call stack)",
        "heap_dynamic": "Heap (dynamic)",
        "used": "Used",
        "base_addr": "Base address",
        "size_bytes": "Size (Bytes)",
        "proportion": "Share",
        "execution_region": "Execution Region",
        "others": "Others ({n} modules)",
        "no_module_data": "(No module data)",
        "sram_critical": "SRAM usage reached {pct}%, near hardware limit! Optimize memory allocation now.",
        "sram_warning": "SRAM usage {pct}%, watch remaining memory to avoid blocking future features.",
        "flash_critical": "Flash usage reached {pct}%, beyond safe range!",
        "flash_warning": "Flash usage {pct}%, limited space remaining.",
        # 控制台消息
        "err_read_build_log": "Failed to read build log",
        "err_read_map": "Failed to read .map file",
        "ok_html": "HTML report generated",
        "ok_json": "JSON exported",
        "ok_csv": "CSV exported",
        "warn_no_csv": "No module data to export as CSV.",
        "info_parsed": "Parsed",
        "info_detected_mcu": "Auto-detected MCU",
        "warn_default_mcu": "No MCU specified, using default",
        "err_unknown_mcu": "Unknown MCU",
        "err_unknown_mcu_hint": "Use --list-mcus to see the supported list.",
        "info_found_map": "Auto-discovered .map",
        "info_parsed_map": "Parsed .map",
        "info_modules": "modules",
        "info_regions": "execution regions",
        "warn_parse_map": "Could not parse .map file, skipping module/region analysis",
        "warn_no_map_csv": "No .map data, cannot export CSV.",
        "err_need_build_log": "No build_log specified and could not auto-discover in project.",
        "err_need_build_log_arg": "Specify a build_log file or --project directory.",
        "err_parse_build_log": "Could not parse Program Size line from {path}.",
        "supported_mcus": "Supported MCUs",
    },
}


def _tr(key: str, **fmt) -> str:
    """按当前 LANG 取翻译文本，并填充 {…} 占位符。"""
    table = LOCALES.get(LANG, LOCALES["zh"])
    text = table.get(key, key)
    if fmt:
        try:
            return text.format(**fmt)
        except (KeyError, IndexError):
            pass
    return text


# ============================================================
#  MCU 规格数据库
# ============================================================
MCU_DATABASE: Dict[str, Dict[str, Any]] = {
    # --- STM32F1 (Cortex-M3) ---
    "STM32F103C8": {"flash": 64 * 1024, "sram": 20 * 1024, "desc": "STM32F103C8 (Cortex-M3, 64KB/20KB)"},
    "STM32F103CB": {"flash": 128 * 1024, "sram": 20 * 1024, "desc": "STM32F103CB (Cortex-M3, 128KB/20KB)"},
    "STM32F103RB": {"flash": 128 * 1024, "sram": 20 * 1024, "desc": "STM32F103RB (Cortex-M3, 128KB/20KB)"},
    "STM32F103RC": {"flash": 256 * 1024, "sram": 48 * 1024, "desc": "STM32F103RC (Cortex-M3, 256KB/48KB)"},
    "STM32F103RE": {"flash": 512 * 1024, "sram": 64 * 1024, "desc": "STM32F103RE (Cortex-M3, 512KB/64KB)"},
    "STM32F103VE": {"flash": 512 * 1024, "sram": 64 * 1024, "desc": "STM32F103VE (Cortex-M3, 512KB/64KB)"},
    "STM32F103ZE": {"flash": 512 * 1024, "sram": 64 * 1024, "desc": "STM32F103ZE (Cortex-M3, 512KB/64KB)"},
    "STM32F107VC": {"flash": 256 * 1024, "sram": 64 * 1024, "desc": "STM32F107VC (Cortex-M3, 256KB/64KB)"},
    # --- STM32F4 (Cortex-M4) ---
    "STM32F407VG": {"flash": 1024 * 1024, "sram": 128 * 1024, "ccm": 64 * 1024, "desc": "STM32F407VG (Cortex-M4, 1MB/128KB+64KB CCM)"},
    "STM32F407VE": {"flash": 512 * 1024, "sram": 128 * 1024, "ccm": 64 * 1024, "desc": "STM32F407VE (Cortex-M4, 512KB/128KB+64KB CCM)"},
    "STM32F407ZE": {"flash": 512 * 1024, "sram": 128 * 1024, "ccm": 64 * 1024, "desc": "STM32F407ZE (Cortex-M4, 512KB/128KB+64KB CCM)"},
    "STM32F407IG": {"flash": 1024 * 1024, "sram": 128 * 1024, "ccm": 64 * 1024, "desc": "STM32F407IG (Cortex-M4, 1MB/128KB+64KB CCM)"},
    "STM32F429IG": {"flash": 1024 * 1024, "sram": 192 * 1024, "ccm": 64 * 1024, "desc": "STM32F429IG (Cortex-M4, 1MB/192KB+64KB CCM)"},
    "STM32F429VG": {"flash": 1024 * 1024, "sram": 192 * 1024, "ccm": 64 * 1024, "desc": "STM32F429VG (Cortex-M4, 1MB/192KB+64KB CCM)"},
    "STM32F411CE": {"flash": 512 * 1024, "sram": 128 * 1024, "desc": "STM32F411CE (Cortex-M4, 512KB/128KB)"},
    "STM32F411RE": {"flash": 512 * 1024, "sram": 128 * 1024, "desc": "STM32F411RE (Cortex-M4, 512KB/128KB)"},
    "STM32F401CC": {"flash": 256 * 1024, "sram": 64 * 1024, "desc": "STM32F401CC (Cortex-M4, 256KB/64KB)"},
    "STM32F401RE": {"flash": 512 * 1024, "sram": 96 * 1024, "desc": "STM32F401RE (Cortex-M4, 512KB/96KB)"},
    # --- STM32F7 (Cortex-M7) ---
    "STM32F746NG": {"flash": 1024 * 1024, "sram": 320 * 1024, "desc": "STM32F746NG (Cortex-M7, 1MB/320KB)"},
    "STM32F767IG": {"flash": 1024 * 1024, "sram": 512 * 1024, "desc": "STM32F767IG (Cortex-M7, 1MB/512KB)"},
    "STM32F767ZI": {"flash": 2048 * 1024, "sram": 512 * 1024, "desc": "STM32F767ZI (Cortex-M7, 2MB/512KB)"},
    # --- STM32H7 (Cortex-M7) ---
    "STM32H743VI": {"flash": 2048 * 1024, "sram": 1024 * 1024, "desc": "STM32H743VI (Cortex-M7, 2MB/1MB)"},
    "STM32H750VB": {"flash": 128 * 1024, "sram": 1024 * 1024, "desc": "STM32H750VB (Cortex-M7, 128KB/1MB)"},
    # --- STM32G0 (Cortex-M0+) ---
    "STM32G070RB": {"flash": 128 * 1024, "sram": 36 * 1024, "desc": "STM32G070RB (Cortex-M0+, 128KB/36KB)"},
    "STM32G071RB": {"flash": 128 * 1024, "sram": 36 * 1024, "desc": "STM32G071RB (Cortex-M0+, 128KB/36KB)"},
    "STM32G0B1RE": {"flash": 512 * 1024, "sram": 144 * 1024, "desc": "STM32G0B1RE (Cortex-M0+, 512KB/144KB)"},
    # --- STM32L0 (Cortex-M0+) ---
    "STM32L031K6": {"flash": 32 * 1024, "sram": 8 * 1024, "desc": "STM32L031K6 (Cortex-M0+, 32KB/8KB)"},
    "STM32L053R8": {"flash": 64 * 1024, "sram": 8 * 1024, "desc": "STM32L053R8 (Cortex-M0+, 64KB/8KB)"},
    # --- STM32L4 (Cortex-M4) ---
    "STM32L432KC": {"flash": 256 * 1024, "sram": 64 * 1024, "desc": "STM32L432KC (Cortex-M4, 256KB/64KB)"},
    "STM32L476RG": {"flash": 1024 * 1024, "sram": 128 * 1024, "desc": "STM32L476RG (Cortex-M4, 1MB/128KB)"},
}


# ============================================================
#  解析器
# ============================================================

def parse_build_log(path: str) -> Optional[dict]:
    """从 Keil MDK 构建日志提取 Program Size 行。"""
    pattern = re.compile(
        r"Program Size:\s*Code=(\d+)\s+RO-data=(\d+)\s+RW-data=(\d+)\s+ZI-data=(\d+)"
    )
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                m = pattern.search(line)
                if m:
                    return {
                        "code": int(m.group(1)),
                        "ro_data": int(m.group(2)),
                        "rw_data": int(m.group(3)),
                        "zi_data": int(m.group(4)),
                    }
    except (IOError, OSError) as e:
        print(f"[ERROR] {_tr('err_read_build_log')}: {e}", file=sys.stderr)
        return None
    return None


def parse_map_file(path: str) -> Optional[dict]:
    """解析 Keil .map 文件，提取:
    - 模块级数据 (Image component sizes)
    - 执行区域 (ER_IROM1, RW_IRAM1)
    - 栈大小
    """
    if not os.path.isfile(path):
        return None

    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
    except (IOError, OSError) as e:
        print(f"[ERROR] {_tr('err_read_map')}: {e}", file=sys.stderr)
        return None

    lines = content.splitlines()

    result: dict = {
        "modules": [],
        "totals": None,
        "grand_total": None,
        "execution_regions": [],
        "stack_size": 0,
        "heap_size": 0,
        "load_regions": [],
    }

    _parse_image_components(lines, result)
    _parse_memory_regions(lines, result)
    _parse_stack_heap(lines, result)

    if not result["modules"] and not result["execution_regions"]:
        return None

    return result


def _parse_image_components(lines: List[str], result: dict) -> None:
    """解析 Image component sizes 节。"""
    in_section = False
    # 6列: Code, inline_data, RO, RW, ZI, Debug
    module_re = re.compile(
        r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([\w./\\\-+]+(?:\.o|\.lib|\.a))"
    )
    totals_re = re.compile(
        r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+Object Totals"
    )
    grand_re = re.compile(
        r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+Grand Totals"
    )

    for line in lines:
        if not in_section:
            if "Image component sizes" in line:
                in_section = True
            continue

        # Grand Totals 是整节的结束标志
        mg = grand_re.match(line)
        if mg:
            result["grand_total"] = _extract_six(mg)
            break

        # Object Totals 结束模块收集区；之后是 Library 区域，不应再计入模块
        mt = totals_re.match(line)
        if mt:
            result["totals"] = _extract_six(mt)
            continue

        # 仅在 Object Totals 之前收集模块行
        if result["totals"] is not None:
            continue

        mm = module_re.match(line)
        if mm:
            result["modules"].append({
                "name": mm.group(7).strip(),
                "code": int(mm.group(1)),
                "inline_data": int(mm.group(2)),
                "ro_data": int(mm.group(3)),
                "rw_data": int(mm.group(4)),
                "zi_data": int(mm.group(5)),
            })


def _parse_memory_regions(lines: List[str], result: dict) -> None:
    """解析 Load Region 和 Execution Region 信息。"""
    # Load Region:  LR_IROM1 (Base: 0x08000000, Size: 0x00002c98, Max: 0x00010000, ...)
    lr_re = re.compile(
        r"^\s*Load Region (\w+)\s*\(Base: (0x[0-9a-fA-F]+), Size: (0x[0-9a-fA-F]+), Max: (0x[0-9a-fA-F]+)"
    )
    # Execution Region: ER_IROM1 (Exec base: 0x..., Load base: 0x..., Size: 0x..., Max: 0x..., ...)
    er_re = re.compile(
        r"^\s*Execution Region (\w+)\s*\(Exec base: (0x[0-9a-fA-F]+), Load base: (0x[0-9a-fA-F]+), Size: (0x[0-9a-fA-F]+), Max: (0x[0-9a-fA-F]+)"
    )
    # Section 行: 0x08000000   0x08000000   0x000000ec   Data   RO    ...   Section Name   Object
    section_re = re.compile(
        r"^\s*(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+|COMPRESSED|-)\s+(0x[0-9a-fA-F]+)\s+(\w+)\s+(\w+)\s+\d+\s*(.*?)\s+(\S+)$"
    )

    current_region = None

    for line in lines:
        lrm = lr_re.match(line)
        if lrm:
            result["load_regions"].append({
                "name": lrm.group(1),
                "base": int(lrm.group(2), 16),
                "size": int(lrm.group(3), 16),
                "max": int(lrm.group(4), 16),
            })
            continue

        erm = er_re.match(line)
        if erm:
            current_region = {
                "name": erm.group(1),
                "exec_base": int(erm.group(2), 16),
                "load_base": int(erm.group(3), 16),
                "size": int(erm.group(4), 16),
                "max": int(erm.group(5), 16),
                "sections": [],
            }
            result["execution_regions"].append(current_region)
            continue

        if current_region is not None:
            sm = section_re.match(line)
            if sm:
                size = int(sm.group(3), 16)
                if size > 0:
                    current_region["sections"].append({
                        "exec_addr": sm.group(1),
                        "load_addr": sm.group(2),
                        "size": size,
                        "type": sm.group(4),
                        "attr": sm.group(5),
                        "name": sm.group(6).strip(),
                        "object": sm.group(7).strip(),
                    })
            elif line.strip() == "" and current_region["sections"]:
                current_region = None


def _parse_stack_heap(lines: List[str], result: dict) -> None:
    """从 .map 中提取 STACK 和 HEAP 大小。

    兼容两种 Keil 输出格式:
    - 符号统计表: STACK  0x20004638  Section  1024  startup_stm32f10x_md.o(STACK)
    - 区域 section 列表: 0x20004638  -  0x00000400  Zero  RW  421  STACK  startup…o
    """
    # 符号表格式: 名称在前，尺寸紧跟类型之后
    symbol_re = re.compile(
        r"^\s*(STACK|HEAP)\s+0x[0-9a-fA-F]+\s+\S+\s+(\d+)\s+"
    )
    # 区域 section 列表格式: 尺寸是第 3 列 (0x…)，名称靠后
    section_re = re.compile(
        r"^\s*0x[0-9a-fA-F]+\s+\S+\s+(0x[0-9a-fA-F]+)\s+\w+\s+\w+\s+\d+\s+(STACK|HEAP)\b"
    )

    sizes = {"STACK": 0, "HEAP": 0}
    for line in lines:
        m = symbol_re.match(line)
        if m:
            name, size = m.group(1), int(m.group(2))
            sizes[name] = max(sizes[name], size)
            continue
        m = section_re.search(line)
        if m:
            name, size = m.group(2), int(m.group(1), 16)
            sizes[name] = max(sizes[name], size)

    result["stack_size"] = sizes["STACK"]
    result["heap_size"] = sizes["HEAP"]


def _extract_six(m: re.Match) -> dict:
    return {
        "code": int(m.group(1)),
        "inline_data": int(m.group(2)),
        "ro_data": int(m.group(3)),
        "rw_data": int(m.group(4)),
        "zi_data": int(m.group(5)),
    }


# ============================================================
#  辅助函数
# ============================================================

def _setup_console_encoding() -> None:
    """强制 stdout/stderr 使用 UTF-8，避免 Windows 控制台 (GBK) 中文乱码。"""
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8")
        except (AttributeError, ValueError):
            pass


def _size_str(bytes_val: int) -> str:
    if bytes_val >= 1024:
        return f"{bytes_val / 1024:.2f} KB"
    return f"{bytes_val} B"


def _size_str_detailed(bytes_val: int) -> str:
    if bytes_val >= 1024:
        return f"{bytes_val / 1024:.2f} KB ({bytes_val:,} B)"
    return f"{bytes_val:,} B"


def _pct(part: int, total: int) -> float:
    if total == 0:
        return 0.0
    return round(part / total * 100, 2)


def _usage_level(pct: float, thresholds: dict) -> str:
    if pct >= thresholds["critical"]:
        return "critical"
    if pct >= thresholds["warning"]:
        return "warning"
    if pct >= thresholds["caution"]:
        return "caution"
    return "ok"


def _level_color(level: str) -> str:
    return {"critical": "#e74c3c", "warning": "#f39c12", "caution": "#3498db", "ok": "#27ae60"}[level]


def _find_map_file(project_dir: str) -> Optional[str]:
    """自动查找 .map 文件。"""
    search_dirs = ["Listings", "Objects", "Output", "MDK-ARM", ""]
    for d in search_dirs:
        base = os.path.join(project_dir, d) if d else project_dir
        if os.path.isdir(base):
            for f in os.listdir(base):
                if f.endswith(".map") and "uvopt" not in f:
                    return os.path.join(base, f)
    return None


def _auto_detect_mcu(project_dir: str) -> Optional[str]:
    """自动检测 MCU 型号。"""
    # 方法1: .uvprojx 文件
    for f in Path(project_dir).glob("*.uvprojx"):
        try:
            content = f.read_text(encoding="utf-8", errors="replace")
            m = re.search(r'Device="([^"]+)"', content)
            if m:
                device = m.group(1)
                for mcu in MCU_DATABASE:
                    if mcu.lower() in device.lower():
                        return mcu
        except Exception:
            pass

    # 方法2: stm32f10x.h 宏定义
    stm32f10x_h = os.path.join(project_dir, "drivers", "cmsis", "device", "stm32f10x.h")
    if os.path.isfile(stm32f10x_h):
        try:
            with open(stm32f10x_h, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            if "STM32F10X_MD" in content:
                return "STM32F103C8"
            if "STM32F10X_HD" in content:
                return "STM32F103ZE"
            if "STM32F10X_CL" in content:
                return "STM32F107VC"
        except Exception:
            pass

    return None


# ============================================================
#  HTML 报告生成
# ============================================================

def generate_html(
    build_data: dict,
    mcu_name: str,
    mcu_spec: dict,
    map_data: Optional[dict],
    output_path: str,
    thresholds: dict,
) -> None:
    code = build_data["code"]
    ro_data = build_data["ro_data"]
    rw_data = build_data["rw_data"]
    zi_data = build_data["zi_data"]

    flash = mcu_spec["flash"]
    sram = mcu_spec["sram"]
    ccm = mcu_spec.get("ccm", 0)

    flash_used = code + ro_data + rw_data
    ram_used = rw_data + zi_data

    flash_pct = _pct(flash_used, flash)
    ram_pct = _pct(ram_used, sram)
    flash_level = _usage_level(flash_pct, thresholds)
    ram_level = _usage_level(ram_pct, thresholds)

    # 汇总数据源
    if map_data and map_data.get("grand_total"):
        gt = map_data["grand_total"]
        s_code, s_ro, s_rw, s_zi = gt["code"], gt["ro_data"], gt["rw_data"], gt["zi_data"]
    else:
        s_code, s_ro, s_rw, s_zi = code, ro_data, rw_data, zi_data

    # 模块表格
    module_rows, module_chart_labels, module_chart_data = _build_module_data(map_data)

    # 区域数据
    region_sections_html = _build_region_sections(map_data, flash, sram, thresholds)

    # 栈/堆
    stack_size = map_data.get("stack_size", 0) if map_data else 0
    heap_size = map_data.get("heap_size", 0) if map_data else 0

    # 生成时间
    gen_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    html = _render_html_template(
        mcu_name=mcu_name,
        mcu_spec=mcu_spec,
        flash=flash, flash_used=flash_used, flash_pct=flash_pct, flash_level=flash_level,
        sram=sram, ram_used=ram_used, ram_pct=ram_pct, ram_level=ram_level,
        ccm=ccm,
        s_code=s_code, s_ro=s_ro, s_rw=s_rw, s_zi=s_zi,
        code=code, ro_data=ro_data, rw_data=rw_data, zi_data=zi_data,
        stack_size=stack_size, heap_size=heap_size,
        gen_time=gen_time,
        module_rows=module_rows,
        module_chart_labels=module_chart_labels,
        module_chart_data=module_chart_data,
        region_sections_html=region_sections_html,
        map_data=map_data,
        thresholds=thresholds,
    )

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(html)
    print(f"[OK] {_tr('ok_html')}: {output_path}")


def _build_module_data(map_data: Optional[dict]):
    if not map_data or not map_data.get("modules"):
        return "", "[]", "[]"

    mods = sorted(map_data["modules"], key=lambda m: m["code"] + m["ro_data"], reverse=True)
    top = mods[:20]
    others = mods[20:]

    rows = []
    for m in top:
        flash_m = m["code"] + m["ro_data"]
        ram_m = m["rw_data"] + m["zi_data"]
        rows.append(
            f"<tr><td class='name'>{m['name']}</td>"
            f"<td>{m['code']:,}</td><td>{m['ro_data']:,}</td>"
            f"<td>{m['rw_data']:,}</td><td>{m['zi_data']:,}</td>"
            f"<td>{flash_m:,}</td><td>{ram_m:,}</td></tr>"
        )

    if others:
        o_code = sum(m["code"] for m in others)
        o_ro = sum(m["ro_data"] for m in others)
        o_rw = sum(m["rw_data"] for m in others)
        o_zi = sum(m["zi_data"] for m in others)
        rows.append(
            f"<tr style='color:#888;'><td class='name'>{_tr('others', n=len(others))}</td>"
            f"<td>{o_code:,}</td><td>{o_ro:,}</td>"
            f"<td>{o_rw:,}</td><td>{o_zi:,}</td>"
            f"<td>{o_code + o_ro:,}</td><td>{o_rw + o_zi:,}</td></tr>"
        )

    chart_mods = sorted(mods, key=lambda m: m["code"] + m["ro_data"], reverse=True)[:12]
    chart_labels = json.dumps([m["name"].replace(".o", "") for m in chart_mods])
    chart_data = json.dumps([m["code"] + m["ro_data"] for m in chart_mods])

    return "".join(rows), chart_labels, chart_data


def _build_region_sections(map_data: Optional[dict], flash: int, sram: int, thresholds: dict) -> str:
    """构建执行区域详情 HTML。"""
    if not map_data or not map_data.get("execution_regions"):
        return ""

    parts = []
    for region in map_data["execution_regions"]:
        name = region["name"]
        r_size = region["size"]
        r_max = region["max"]
        pct = _pct(r_size, r_max) if r_max > 0 else 0
        level = _usage_level(pct, thresholds)

        # 按 section 聚合
        section_map: dict = {}
        for sec in region["sections"]:
            key = f"{sec['name']} ({sec['type']}/{sec['attr']})"
            section_map[key] = section_map.get(key, 0) + sec["size"]

        sorted_secs = sorted(section_map.items(), key=lambda x: x[1], reverse=True)
        sec_rows = ""
        for sn, sz in sorted_secs:
            sec_pct = _pct(sz, r_size) if r_size > 0 else 0
            sec_rows += (
                f"<tr><td>{sn}</td><td>{sz:,}</td>"
                f"<td>{sec_pct:.1f}%</td></tr>"
            )

        parts.append(f"""
        <div class="panel">
            <h2>{_tr('execution_region')}: {name}</h2>
            <div class="cards" style="margin-bottom:16px;">
                <div class="card">
                    <h3>{_tr('used')}</h3>
                    <div class="value" style="color:{_level_color(level)}">{_size_str(r_size)}</div>
                    <div class="sub-value">{pct:.1f}% of {_size_str(r_max)}</div>
                    <div class="bar-bg"><div class="bar-fill" style="width:{min(pct,100):.1f}%;background:{_level_color(level)}"></div></div>
                </div>
                <div class="card">
                    <h3>{_tr('base_addr')}</h3>
                    <div class="value">0x{region['exec_base']:08X}</div>
                    <div class="sub-value">Load: 0x{region['load_base']:08X}</div>
                </div>
            </div>
            <table>
                <thead><tr><th>Section</th><th>{_tr('size_bytes')}</th><th>{_tr('proportion')}</th></tr></thead>
                <tbody>{sec_rows}</tbody>
            </table>
        </div>""")

    return "".join(parts)


def _empty_panel(msg: str) -> str:
    """生成一个空状态提示面板。用单引号 f-string 避免嵌套表达式里的反斜杠。"""
    return f'<div class="panel"><p style="color:var(--text-secondary);">{msg}</p></div>'


def _render_html_template(**v) -> str:
    """渲染完整 HTML 模板。"""
    has_map = bool(v["map_data"])
    has_regions = bool(v["region_sections_html"])
    has_modules = bool(v["module_rows"])

    # JavaScript 中需要的换行转义符 \n，用变量避免 f-string 内的反斜杠问题
    _BSN = "\\n"

    # 选项卡按钮（预计算避免嵌套 f-string 中的反斜杠）
    _modules_tab = f'<button class="tab-btn" onclick="switchTab(\'modules\')">{_tr("modules")} ({len(v["map_data"]["modules"])})</button>' if has_modules else ''
    _regions_tab = f'<button class="tab-btn" onclick="switchTab(\'regions\')">{_tr("regions")}</button>' if has_regions else ''

    return f"""<!DOCTYPE html>
<html lang="{_tr('html_lang')}">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>STM32 Memory Analysis — {v['mcu_name']}</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
/* ========== 基础 + 浅色主题 ========== */
:root {{
    --bg: #f1f5f9; --surface: #fff; --text: #1e293b; --text-secondary: #64748b;
    --border: #e2e8f0; --hover: #f8fafc; --table-header: #f8fafc;
    --code-bg: #f1f5f9; --shadow: rgba(0,0,0,0.08);
}}
[data-theme="dark"] {{
    --bg: #0f172a; --surface: #1e293b; --text: #e2e8f0; --text-secondary: #94a3b8;
    --border: #334155; --hover: #1e293b; --table-header: #1e293b;
    --code-bg: #0f172a; --shadow: rgba(0,0,0,0.3);
}}
* {{ margin: 0; padding: 0; box-sizing: border-box; }}
body {{
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    background: var(--bg); color: var(--text); line-height: 1.6; transition: background 0.3s, color 0.3s;
}}
.container {{ max-width: 1200px; margin: 0 auto; padding: 24px 16px; }}
/* 顶部栏 */
.topbar {{
    display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 12px;
    margin-bottom: 24px;
}}
.topbar .brand {{ font-size: 0.85rem; color: var(--text-secondary); }}
.topbar .actions {{ display: flex; gap: 8px; flex-wrap: wrap; }}
.btn {{
    padding: 6px 14px; border-radius: 6px; border: 1px solid var(--border); background: var(--surface);
    color: var(--text); cursor: pointer; font-size: 0.8rem; transition: all 0.2s;
    display: flex; align-items: center; gap: 4px;
}}
.btn:hover {{ background: var(--hover); }}
.btn.active {{ background: #2563eb; color: #fff; border-color: #2563eb; }}
/* 头部 */
.header {{
    background: linear-gradient(135deg, #1e3a5f 0%, #2563eb 100%);
    color: #fff; padding: 32px 24px; border-radius: 12px; margin-bottom: 24px;
}}
.header h1 {{ font-size: 1.6rem; font-weight: 700; }}
.header .sub {{ font-size: 0.85rem; opacity: 0.8; margin-top: 4px; }}
.header .meta {{ font-size: 0.8rem; opacity: 0.65; margin-top: 8px; }}
/* 选项卡 */
.tabs {{ display: flex; gap: 0; margin-bottom: 24px; border-bottom: 2px solid var(--border); }}
.tab-btn {{
    padding: 10px 20px; border: none; background: none; color: var(--text-secondary);
    cursor: pointer; font-size: 0.9rem; font-weight: 500; border-bottom: 2px solid transparent;
    margin-bottom: -2px; transition: all 0.2s;
}}
.tab-btn:hover {{ color: var(--text); }}
.tab-btn.active {{ color: #2563eb; border-bottom-color: #2563eb; }}
.tab-panel {{ display: none; }}
.tab-panel.active {{ display: block; }}
/* 卡片 */
.cards {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-bottom: 24px; }}
.card {{
    background: var(--surface); border-radius: 10px; padding: 20px;
    box-shadow: 0 1px 3px var(--shadow); border: 1px solid var(--border);
}}
.card h3 {{ font-size: 0.75rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-secondary); margin-bottom: 8px; }}
.card .value {{ font-size: 1.5rem; font-weight: 700; }}
.card .sub-value {{ font-size: 0.8rem; color: var(--text-secondary); margin-top: 2px; }}
.card .bar-bg {{ height: 8px; background: var(--border); border-radius: 4px; margin-top: 12px; overflow: hidden; }}
.card .bar-fill {{ height: 100%; border-radius: 4px; transition: width 0.5s; }}
/* 面板 */
.panel {{
    background: var(--surface); border-radius: 10px; padding: 24px;
    box-shadow: 0 1px 3px var(--shadow); border: 1px solid var(--border);
    margin-bottom: 24px;
}}
.panel h2 {{ font-size: 1.1rem; font-weight: 600; margin-bottom: 16px; color: var(--text); }}
/* 图表 */
.chart-wrap {{ max-width: 480px; margin: 0 auto; }}
.chart-row {{ display: grid; grid-template-columns: 1fr 1fr; gap: 24px; }}
@media (max-width: 700px) {{ .chart-row {{ grid-template-columns: 1fr; }} }}
/* 表格 */
.table-wrap {{ overflow-x: auto; }}
table {{ width: 100%; border-collapse: collapse; font-size: 0.85rem; }}
th {{ background: var(--table-header); text-align: left; padding: 10px 12px; border-bottom: 2px solid var(--border);
      font-weight: 600; color: var(--text-secondary); white-space: nowrap; }}
td {{ padding: 8px 12px; border-bottom: 1px solid var(--border); white-space: nowrap; }}
td.name {{ max-width: 260px; overflow: hidden; text-overflow: ellipsis; }}
tr:hover td {{ background: var(--hover); }}
/* 图例 */
.legend {{ display: flex; flex-wrap: wrap; gap: 16px; margin-top: 12px; font-size: 0.8rem; }}
.legend-item {{ display: flex; align-items: center; gap: 6px; }}
.legend-dot {{ width: 12px; height: 12px; border-radius: 3px; }}
/* 告警 */
.alert {{
    padding: 12px 16px; border-radius: 8px; font-size: 0.85rem; margin-bottom: 16px;
    display: flex; align-items: center; gap: 8px;
}}
.alert-critical {{ background: #fde8e8; color: #991b1b; border: 1px solid #fca5a5; }}
.alert-warning {{ background: #fef3c7; color: #92400e; border: 1px solid #fcd34d; }}
.alert-icon {{ font-size: 1.2rem; }}
[data-theme="dark"] .alert-critical {{ background: #451a1a; color: #fca5a5; border-color: #7f1d1d; }}
[data-theme="dark"] .alert-warning {{ background: #452c0a; color: #fcd34d; border-color: #78350f; }}
/* 页脚 */
.footer {{ text-align: center; font-size: 0.75rem; color: var(--text-secondary); margin-top: 32px; padding: 16px; }}
/* 徽章 */
.badge {{
    display: inline-block; padding: 2px 8px; border-radius: 10px; font-size: 0.7rem; font-weight: 600;
}}
.badge-ok {{ background: #d1fae5; color: #065f46; }}
.badge-warning {{ background: #fef3c7; color: #92400e; }}
.badge-critical {{ background: #fde8e8; color: #991b1b; }}
[data-theme="dark"] .badge-ok {{ background: #064e3b; color: #6ee7b7; }}
[data-theme="dark"] .badge-warning {{ background: #78350f; color: #fcd34d; }}
[data-theme="dark"] .badge-critical {{ background: #7f1d1d; color: #fca5a5; }}
/* 打印 */
@media print {{
    body {{ background: #fff; color: #000; font-size: 11pt; }}
    .topbar, .tabs, .btn {{ display: none !important; }}
    .tab-panel {{ display: block !important; page-break-inside: avoid; }}
    .header {{ background: #fff !important; color: #000 !important; border: 1px solid #ccc; }}
    .panel {{ box-shadow: none; border: 1px solid #ccc; page-break-inside: avoid; }}
    .card {{ box-shadow: none; border: 1px solid #ccc; }}
    .container {{ max-width: 100%; padding: 0; }}
}}
</style>
</head>
<body>
<div class="container">

<!-- 顶部栏 -->
<div class="topbar">
    <div class="brand">STM32 Memory Analyzer v2.0</div>
    <div class="actions">
        <button class="btn" onclick="toggleTheme()" title="{_tr('toggle_dark_mode')}">&#9788; {_tr('dark_mode')}</button>
        <button class="btn" onclick="window.print()" title="{_tr('print_report')}">&#9113; {_tr('print')}</button>
        <button class="btn" onclick="exportJSON()" title="{_tr('export_json')}">&#8615; JSON</button>
        <button class="btn" onclick="exportCSV()" title="{_tr('export_csv')}">&#8615; CSV</button>
    </div>
</div>

<!-- 头部 -->
<div class="header">
    <h1>{_tr('report_title')}</h1>
    <div class="sub">{v['mcu_spec']['desc']}  ·  Flash: {v['flash'] // 1024} KB  ·  SRAM: {v['sram'] // 1024} KB{'  ·  CCM: ' + str(v['ccm'] // 1024) + ' KB' if v['ccm'] else ''}</div>
    <div class="meta">{_tr('generated_at')}: {v['gen_time']}  ·  {_tr('alert_thresholds')}: Caution {v['thresholds']['caution']}% / Warning {v['thresholds']['warning']}% / Critical {v['thresholds']['critical']}%</div>
</div>

<!-- 告警 -->
{_render_alerts(v['flash_level'], v['flash_pct'], v['ram_level'], v['ram_pct'])}

<!-- 概览卡片 -->
<div class="cards">
    <div class="card">
        <h3>{_tr('flash_usage')}</h3>
        <div class="value" style="color:{_level_color(v['flash_level'])}">{v['flash_pct']:.1f}%</div>
        <div class="sub-value">{_size_str_detailed(v['flash_used'])} / {v['flash'] // 1024} KB</div>
        <div class="bar-bg"><div class="bar-fill" style="width:{v['flash_pct']:.1f}%;background:{_level_color(v['flash_level'])}"></div></div>
    </div>
    <div class="card">
        <h3>{_tr('sram_usage')}</h3>
        <div class="value" style="color:{_level_color(v['ram_level'])}">{v['ram_pct']:.1f}%</div>
        <div class="sub-value">{_size_str_detailed(v['ram_used'])} / {v['sram'] // 1024} KB</div>
        <div class="bar-bg"><div class="bar-fill" style="width:{v['ram_pct']:.1f}%;background:{_level_color(v['ram_level'])}"></div></div>
    </div>
    <div class="card">
        <h3>Code</h3>
        <div class="value">{_size_str(v['s_code'])}</div>
        <div class="sub-value">{_tr('executable_code')}</div>
    </div>
    <div class="card">
        <h3>ZI-Data</h3>
        <div class="value">{_size_str(v['s_zi'])}</div>
        <div class="sub-value">{_tr('bss_stack_heap')}</div>
    </div>
    {f'''<div class="card">
        <h3>{_tr('stack')} (Stack)</h3>
        <div class="value">{_size_str(v['stack_size'])}</div>
        <div class="sub-value">{_tr('defined_in_startup')}</div>
    </div>''' if v['stack_size'] else ''}
    {f'''<div class="card">
        <h3>{_tr('heap')} (Heap)</h3>
        <div class="value">{_size_str(v['heap_size'])}</div>
        <div class="sub-value">{_tr('dynamic_memory')}</div>
    </div>''' if v['heap_size'] else ''}
</div>

<!-- 选项卡导航 -->
<div class="tabs">
    <button class="tab-btn active" onclick="switchTab('overview')">{_tr('overview')}</button>
    <button class="tab-btn" onclick="switchTab('details')">{_tr('details')}</button>
    {_modules_tab}
    {_regions_tab}
</div>

<!-- Tab: 概览 -->
<div class="tab-panel active" id="tab-overview">
    <div class="panel">
        <h2>{_tr('storage_distribution')}</h2>
        <div class="chart-row">
            <div class="chart-wrap">
                <canvas id="flashChart"></canvas>
                <div class="legend">
                    <div class="legend-item"><span class="legend-dot" style="background:#2563eb"></span> Code</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#60a5fa"></span> RO-Data</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#f59e0b"></span> RW-Data</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#cbd5e1"></span> Free</div>
                </div>
            </div>
            <div class="chart-wrap">
                <canvas id="ramChart"></canvas>
                <div class="legend">
                    <div class="legend-item"><span class="legend-dot" style="background:#f59e0b"></span> RW-Data</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#ef4444"></span> ZI-Data</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#cbd5e1"></span> Free</div>
                </div>
            </div>
        </div>
    </div>
    {_render_ram_breakdown_chart(v['map_data'], v['sram']) if has_map else ''}
</div>

<!-- Tab: 数据明细 -->
<div class="tab-panel" id="tab-details">
    <div class="panel">
        <h2>{_tr('memory_details')}</h2>
        <div class="table-wrap">
        <table>
            <tr><th>{_tr('type')}</th><th>{_tr('size')}</th><th>{_tr('percentage')}</th><th>{_tr('description')}</th></tr>
            <tr><td>Code</td><td>{v['s_code']:,} B</td><td><span class="badge badge-ok">{_pct(v['s_code'], v['flash']):.1f}% Flash</span></td><td>{_tr('executable_code_flash')}</td></tr>
            <tr><td>RO-Data</td><td>{v['s_ro']:,} B</td><td><span class="badge badge-ok">{_pct(v['s_ro'], v['flash']):.1f}% Flash</span></td><td>{_tr('readonly_data')}</td></tr>
            <tr><td>RW-Data</td><td>{v['s_rw']:,} B</td><td><span class="badge badge-ok">{_pct(v['s_rw'], v['flash']):.1f}% Flash</span> / <span class="badge badge-ok">{_pct(v['s_rw'], v['sram']):.1f}% SRAM</span></td><td>{_tr('rw_data_dual')}</td></tr>
            <tr><td>ZI-Data</td><td>{v['s_zi']:,} B</td><td><span class="badge badge-ok">{_pct(v['s_zi'], v['sram']):.1f}% SRAM</span></td><td>{_tr('zi_data_sram')}</td></tr>
            <tr style="font-weight:600;background:var(--table-header);">
                <td>{_tr('flash_total')}</td><td>{v['flash_used']:,} B</td><td><span class="badge badge-{v['flash_level']}">{v['flash_pct']:.1f}%</span></td><td>Code + RO-Data + RW-Data</td></tr>
            <tr style="font-weight:600;background:var(--table-header);">
                <td>{_tr('sram_total')}</td><td>{v['ram_used']:,} B</td><td><span class="badge badge-{v['ram_level']}">{v['ram_pct']:.1f}%</span></td><td>RW-Data + ZI-Data</td></tr>
            {f'''<tr><td>{_tr('stack')} (Stack)</td><td>{v['stack_size']:,} B</td><td><span class="badge badge-ok">{_pct(v['stack_size'], v['sram']):.1f}% SRAM</span></td><td>{_tr('stack_space')}</td></tr>''' if v['stack_size'] else ''}
            {f'''<tr><td>{_tr('heap')} (Heap)</td><td>{v['heap_size']:,} B</td><td><span class="badge badge-ok">{_pct(v['heap_size'], v['sram']):.1f}% SRAM</span></td><td>{_tr('heap_space')}</td></tr>''' if v['heap_size'] else ''}
        </table>
        </div>
    </div>
</div>

<!-- Tab: 模块 -->
<div class="tab-panel" id="tab-modules">
    {f'''<div class="panel">
        <h2>{_tr('module_flash_top12')}</h2>
        <div class="chart-wrap" style="max-width:100%;">
            <canvas id="moduleChart"></canvas>
        </div>
    </div>
    <div class="panel">
        <h2>{_tr('module_detail')}</h2>
        <div class="table-wrap">
        <table>
            <thead><tr><th>{_tr('module')}</th><th>Code</th><th>RO-Data</th><th>RW-Data</th><th>ZI-Data</th><th>Flash (Code+RO)</th><th>RAM (RW+ZI)</th></tr></thead>
            <tbody>{v['module_rows']}</tbody>
        </table>
        </div>
    </div>''' if has_modules else _empty_panel(_tr('no_map_module_data'))}
</div>

<!-- Tab: 区域 -->
<div class="tab-panel" id="tab-regions">
    {v['region_sections_html'] if has_regions else _empty_panel(_tr('no_region_data'))}
</div>

<div class="footer">
    Generated by STM32 Memory Analyzer v2.0  ·  {v['mcu_name']}  ·  {v['gen_time']}
</div>

</div>

<script>
// ========== 主题切换 ==========
function toggleTheme() {{
    const html = document.documentElement;
    const isDark = html.getAttribute('data-theme') === 'dark';
    html.setAttribute('data-theme', isDark ? '' : 'dark');
    localStorage.setItem('stm32-mem-theme', isDark ? 'light' : 'dark');
}}
(function() {{
    if (localStorage.getItem('stm32-mem-theme') === 'dark') {{
        document.documentElement.setAttribute('data-theme', 'dark');
    }}
}})();

// ========== 选项卡切换 ==========
function switchTab(name) {{
    document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
    document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
    document.getElementById('tab-' + name).classList.add('active');
    event.target.classList.add('active');
    // 重绘图表
    if (name === 'modules') {{ setTimeout(function() {{ if (window.moduleChartInstance) window.moduleChartInstance.resize(); }}, 100); }}
}}

// ========== 导出 ==========
function exportJSON() {{
    const data = {{
        mcu: "{v['mcu_name']}",
        flash: {{ total: {v['flash']}, used: {v['flash_used']}, pct: {v['flash_pct']} }},
        sram: {{ total: {v['sram']}, used: {v['ram_used']}, pct: {v['ram_pct']} }},
        breakdown: {{ code: {v['code']}, ro_data: {v['ro_data']}, rw_data: {v['rw_data']}, zi_data: {v['zi_data']} }},
        stack: {v['stack_size']}, heap: {v['heap_size']},
        generated: "{v['gen_time']}"
    }};
    downloadFile('memory_report.json', JSON.stringify(data, null, 2), 'application/json');
}}

function exportCSV() {{
    var csv = 'Name,Code,RO_Data,RW_Data,ZI_Data,Flash,RAM{_BSN}';
    var rows = document.querySelectorAll('#tab-modules tbody tr');
    rows.forEach(function(row) {{
        var cols = row.querySelectorAll('td');
        if (cols.length >= 7) {{
            csv += '"' + cols[0].textContent.trim() + '",' +
                   cols[1].textContent.trim().replace(/,/g,'') + ',' +
                   cols[2].textContent.trim().replace(/,/g,'') + ',' +
                   cols[3].textContent.trim().replace(/,/g,'') + ',' +
                   cols[4].textContent.trim().replace(/,/g,'') + ',' +
                   cols[5].textContent.trim().replace(/,/g,'') + ',' +
                   cols[6].textContent.trim().replace(/,/g,'') + '{_BSN}';
        }}
    }});
    if (!rows.length) csv = 'Name,Code,RO_Data,RW_Data,ZI_Data,Flash,RAM{_BSN}{_tr('no_module_data')}{_BSN}';
    downloadFile('memory_modules.csv', csv, 'text/csv');
}}

function downloadFile(filename, content, mime) {{
    var blob = new Blob([content], {{ type: mime + ';charset=utf-8;' }});
    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url; a.download = filename;
    document.body.appendChild(a); a.click();
    document.body.removeChild(a); URL.revokeObjectURL(url);
}}

// ========== Chart.js ==========
// Flash 饼图
new Chart(document.getElementById('flashChart'), {{
    type: 'doughnut',
    data: {{
        labels: ['Code', 'RO-Data', 'RW-Data', 'Free'],
        datasets: [{{ data: [{v['code']}, {v['ro_data']}, {v['rw_data']}, {max(v['flash'] - v['flash_used'], 0)}],
            backgroundColor: ['#2563eb', '#60a5fa', '#f59e0b', '#cbd5e1'],
            borderColor: '#fff', borderWidth: 2 }}]
    }},
    options: {{ responsive: true, plugins: {{ legend: {{ display: false }} }} }}
}});

// RAM 饼图
new Chart(document.getElementById('ramChart'), {{
    type: 'doughnut',
    data: {{
        labels: ['RW-Data', 'ZI-Data', 'Free'],
        datasets: [{{ data: [{v['rw_data']}, {v['zi_data']}, {max(v['sram'] - v['ram_used'], 0)}],
            backgroundColor: ['#f59e0b', '#ef4444', '#cbd5e1'],
            borderColor: '#fff', borderWidth: 2 }}]
    }},
    options: {{ responsive: true, plugins: {{ legend: {{ display: false }} }} }}
}});

// 模块柱状图
{_render_module_chart_js(v['module_chart_labels'], v['module_chart_data']) if has_modules else ""}

// RAM 细分图 (BSS vs STACK vs DATA)
{_render_ram_detail_chart_js(v['map_data'], v['sram'], v['ram_used']) if has_map else ""}
</script>
</body>
</html>"""


def _render_alerts(flash_level: str, flash_pct: float, ram_level: str, ram_pct: float) -> str:
    alerts = []
    if ram_level == "critical":
        alerts.append(('alert-critical', '&#9888;', _tr('sram_critical', pct=ram_pct)))
    elif ram_level == "warning":
        alerts.append(('alert-warning', '&#9888;', _tr('sram_warning', pct=ram_pct)))
    if flash_level == "critical":
        alerts.append(('alert-critical', '&#9888;', _tr('flash_critical', pct=flash_pct)))
    elif flash_level == "warning":
        alerts.append(('alert-warning', '&#9888;', _tr('flash_warning', pct=flash_pct)))
    if not alerts:
        return ""
    return "\n".join(
        f'<div class="alert {cls}"><span class="alert-icon">{icon}</span> {msg}</div>'
        for cls, icon, msg in alerts
    )


def _extract_ram_breakdown(map_data: Optional[dict]) -> dict:
    """从执行区域 section 汇总 RAM 细分 (bss/data/stack/heap)。"""
    bss_size = 0
    data_size = 0
    stack_size = map_data.get("stack_size", 0) if map_data else 0
    heap_size = map_data.get("heap_size", 0) if map_data else 0

    for region in (map_data.get("execution_regions", []) if map_data else []):
        if "RW_IRAM" in region["name"]:
            for sec in region.get("sections", []):
                if sec["name"] == ".bss":
                    bss_size += sec["size"]
                elif sec["name"] == ".data":
                    data_size += sec["size"]
                elif sec["name"] == "STACK":
                    stack_size = max(stack_size, sec["size"])
                elif sec["name"] == "HEAP":
                    heap_size = max(heap_size, sec["size"])

    return {"bss": bss_size, "data": data_size, "stack": stack_size, "heap": heap_size}


def _render_ram_breakdown_chart(map_data: Optional[dict], sram: int) -> str:
    """渲染 RAM 细分图 (BSS/DATA/STACK/HEAP)。"""
    if not map_data:
        return ""
    bd = _extract_ram_breakdown(map_data)
    bss_size, data_size = bd["bss"], bd["data"]
    stack_size, heap_size = bd["stack"], bd["heap"]

    if bss_size == 0 and data_size == 0 and stack_size == 0:
        return ""

    return f"""
    <div class="panel">
        <h2>{_tr('sram_breakdown')}</h2>
        <div class="chart-row">
            <div class="chart-wrap">
                <canvas id="ramDetailChart"></canvas>
                <div class="legend">
                    <div class="legend-item"><span class="legend-dot" style="background:#8b5cf6"></span> .bss</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#f59e0b"></span> .data</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#ef4444"></span> Stack</div>
                    <div class="legend-item"><span class="legend-dot" style="background:#cbd5e1"></span> Free</div>
                </div>
            </div>
            <div class="table-wrap">
                <table>
                    <tr><th>{_tr('region')}</th><th>{_tr('size')}</th><th>{_tr('proportion')}</th></tr>
                    <tr><td>{_tr('bss_uninit')}</td><td>{bss_size:,} B</td><td><span class="badge badge-ok">{_pct(bss_size, sram):.1f}%</span></td></tr>
                    <tr><td>{_tr('data_init')}</td><td>{data_size:,} B</td><td><span class="badge badge-ok">{_pct(data_size, sram):.1f}%</span></td></tr>
                    <tr><td>{_tr('stack_call')}</td><td>{stack_size:,} B</td><td><span class="badge badge-ok">{_pct(stack_size, sram):.1f}%</span></td></tr>
                    {f'<tr><td>{_tr("heap_dynamic")}</td><td>{heap_size:,} B</td><td><span class="badge badge-ok">{_pct(heap_size, sram):.1f}%</span></td></tr>' if heap_size else ''}
                </table>
            </div>
        </div>
    </div>"""


def _render_ram_detail_chart_js(map_data: Optional[dict], sram: int, ram_used: int) -> str:
    if not map_data:
        return ""
    bd = _extract_ram_breakdown(map_data)
    bss_size, data_size = bd["bss"], bd["data"]
    stack_size = bd["stack"]

    if bss_size == 0 and data_size == 0 and stack_size == 0:
        return ""

    return f"""
new Chart(document.getElementById('ramDetailChart'), {{
    type: 'doughnut',
    data: {{
        labels: ['.bss', '.data', 'Stack', 'Free'],
        datasets: [{{ data: [{bss_size}, {data_size}, {stack_size}, {max(sram - ram_used, 0)}],
            backgroundColor: ['#8b5cf6', '#f59e0b', '#ef4444', '#cbd5e1'],
            borderColor: '#fff', borderWidth: 2 }}]
    }},
    options: {{ responsive: true, plugins: {{ legend: {{ display: false }} }} }}
}});
"""


def _render_module_chart_js(labels: str, data: str) -> str:
    return f"""
window.moduleChartInstance = new Chart(document.getElementById('moduleChart'), {{
    type: 'bar',
    data: {{
        labels: {labels},
        datasets: [{{ label: 'Flash (Code+RO) Bytes', data: {data},
            backgroundColor: '#2563eb', borderRadius: 4 }}]
    }},
    options: {{
        indexAxis: 'y', responsive: true,
        plugins: {{ legend: {{ display: false }} }},
        scales: {{ x: {{ ticks: {{ callback: function(v) {{ return (v/1024).toFixed(1) + ' KB'; }} }} }} }}
    }}
}});
"""


# ============================================================
#  JSON/CSV 导出
# ============================================================

def export_json(build_data: dict, mcu_name: str, mcu_spec: dict, map_data: Optional[dict], output_path: str) -> None:
    """导出为 JSON 格式。"""
    flash = mcu_spec["flash"]
    sram = mcu_spec["sram"]
    flash_used = build_data["code"] + build_data["ro_data"] + build_data["rw_data"]
    ram_used = build_data["rw_data"] + build_data["zi_data"]

    export = {
        "meta": {
            "mcu": mcu_name,
            "description": mcu_spec["desc"],
            "flash_total": flash,
            "sram_total": sram,
            "ccm": mcu_spec.get("ccm", 0),
            "generated": datetime.now().isoformat(),
        },
        "summary": {
            "flash_used": flash_used,
            "flash_pct": _pct(flash_used, flash),
            "sram_used": ram_used,
            "sram_pct": _pct(ram_used, sram),
        },
        "breakdown": build_data,
    }

    if map_data and map_data.get("modules"):
        export["modules"] = map_data["modules"]
    if map_data and map_data.get("stack_size"):
        export["stack_size"] = map_data["stack_size"]
    if map_data and map_data.get("heap_size"):
        export["heap_size"] = map_data["heap_size"]
    if map_data and map_data.get("execution_regions"):
        export["regions"] = [
            {
                "name": r["name"],
                "size": r["size"],
                "max": r["max"],
                "exec_base": f"0x{r['exec_base']:08X}",
            }
            for r in map_data["execution_regions"]
        ]

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(export, f, indent=2, ensure_ascii=False)
    print(f"[OK] {_tr('ok_json')}: {output_path}")


def export_csv(map_data: dict, output_path: str) -> None:
    """导出模块数据为 CSV。"""
    if not map_data or not map_data.get("modules"):
        print(f"[WARN] {_tr('warn_no_csv')}")
        return

    with open(output_path, "w", newline="", encoding="utf-8-sig") as f:
        writer = csv.writer(f)
        writer.writerow(["Name", "Code", "RO_Data", "RW_Data", "ZI_Data", "Flash", "RAM"])
        for m in map_data["modules"]:
            writer.writerow([
                m["name"], m["code"], m["ro_data"], m["rw_data"], m["zi_data"],
                m["code"] + m["ro_data"], m["rw_data"] + m["zi_data"],
            ])
    print(f"[OK] {_tr('ok_csv')}: {output_path}")


# ============================================================
#  CLI 入口
# ============================================================

def main() -> None:
    _setup_console_encoding()

    parser = argparse.ArgumentParser(
        description="STM32 内存分析专业工具 — 解析 Keil MDK 构建日志，生成 HTML 可视化报告",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
    python memory_analyzer.py build_log.txt
    python memory_analyzer.py build_log.txt -m Listings/stm32f1_ov2640.map
    python memory_analyzer.py --project .                          # 自动发现项目文件
    python memory_analyzer.py build_log.txt --mcu STM32F407VG -o report.html
    python memory_analyzer.py build_log.txt --json summary.json    # 同时导出 JSON
    python memory_analyzer.py build_log.txt -m a.map --warning 80 --critical 95
        """,
    )
    parser.add_argument("build_log", nargs="?", default=None, help="Keil MDK 构建日志文件路径")
    parser.add_argument("--project", "-p", default=None, help="项目根目录（自动查找 build_log 和 .map）")
    parser.add_argument("--map", "-m", dest="map_file", default=None, help="Keil .map 文件路径")
    parser.add_argument("--mcu", "-c", default=None, help="MCU 型号（如 STM32F103C8）")
    parser.add_argument("--output", "-o", default=None, help="HTML 输出路径（默认: memory_report.html）")
    parser.add_argument("--json", "-j", dest="json_out", default=None, help="JSON 导出路径")
    parser.add_argument("--csv", dest="csv_out", default=None, help="CSV 导出路径")
    parser.add_argument("--warning", type=float, default=85, help="SRAM/Flash 告警阈值 %% (默认: 85)")
    parser.add_argument("--critical", type=float, default=95, help="SRAM/Flash 严重阈值 %% (默认: 95)")
    parser.add_argument("--lang", "-l", choices=["zh", "en"], default="zh", help="界面语言 / report language (zh/en, 默认 zh)")
    parser.add_argument("--list-mcus", action="store_true", help="列出支持的 MCU 型号")
    args = parser.parse_args()

    # 设置界面语言
    global LANG
    LANG = args.lang

    if args.list_mcus:
        print(f"{_tr('supported_mcus')} ({len(MCU_DATABASE)}):")
        for name, spec in sorted(MCU_DATABASE.items()):
            ccm_str = f"  CCM={spec['ccm'] // 1024}KB" if spec.get("ccm") else ""
            print(f"  {name:16s}  Flash={spec['flash'] // 1024:4d}KB  SRAM={spec['sram'] // 1024:4d}KB{ccm_str}  ({spec['desc']})")
        return

    # 阈值
    thresholds = {
        "caution": 70,
        "warning": args.warning,
        "critical": args.critical,
    }

    # 确定项目目录
    project_dir = "."
    if args.project:
        project_dir = args.project
    elif args.build_log:
        project_dir = os.path.dirname(os.path.abspath(args.build_log))

    project_dir = os.path.abspath(project_dir)

    # 确定 build_log
    build_log_path = args.build_log
    if not build_log_path and args.project:
        # 自动查找
        for candidate in ["build_log.txt", "build.log", "output.txt"]:
            p = os.path.join(project_dir, candidate)
            if os.path.isfile(p):
                build_log_path = p
                break
        if not build_log_path:
            print(f"[ERROR] {_tr('err_need_build_log')}", file=sys.stderr)
            sys.exit(1)

    if not build_log_path:
        print(f"[ERROR] {_tr('err_need_build_log_arg')}", file=sys.stderr)
        sys.exit(1)

    # 解析构建日志
    build_data = parse_build_log(build_log_path)
    if not build_data:
        print(f"[ERROR] {_tr('err_parse_build_log', path=build_log_path)}", file=sys.stderr)
        sys.exit(1)

    print(f"[INFO] {_tr('info_parsed')}: Code={build_data['code']}, RO-data={build_data['ro_data']}, "
          f"RW-data={build_data['rw_data']}, ZI-data={build_data['zi_data']}")

    # 确定 MCU
    mcu_name = args.mcu
    if not mcu_name:
        detected = _auto_detect_mcu(project_dir)
        if detected:
            mcu_name = detected
            print(f"[INFO] {_tr('info_detected_mcu')}: {mcu_name}")

    if not mcu_name:
        mcu_name = "STM32F103C8"
        print(f"[WARN] {_tr('warn_default_mcu')}: {mcu_name}")

    mcu_name_upper = mcu_name.upper()
    mcu_spec = MCU_DATABASE.get(mcu_name_upper)
    if not mcu_spec:
        print(f"[ERROR] {_tr('err_unknown_mcu')}: {mcu_name}。{_tr('err_unknown_mcu_hint')}", file=sys.stderr)
        sys.exit(1)

    # 自动查找 .map
    map_file = args.map_file
    if not map_file and args.project:
        map_file = _find_map_file(project_dir)
        if map_file:
            print(f"[INFO] {_tr('info_found_map')}: {map_file}")

    # 解析 .map
    map_data = None
    if map_file:
        map_data = parse_map_file(map_file)
        if map_data:
            n_mods = len(map_data.get("modules", []))
            n_regions = len(map_data.get("execution_regions", []))
            stk = map_data.get("stack_size", 0)
            print(f"[INFO] {_tr('info_parsed_map')}: {n_mods} {_tr('info_modules')}, {n_regions} {_tr('info_regions')}"
                  f"{', Stack=' + str(stk) + 'B' if stk else ''}")
        else:
            print(f"[WARN] {_tr('warn_parse_map')}")

    # 生成 HTML
    output_path = args.output or "memory_report.html"
    generate_html(build_data, mcu_name_upper, mcu_spec, map_data, output_path, thresholds)

    # JSON 导出
    if args.json_out:
        export_json(build_data, mcu_name_upper, mcu_spec, map_data, args.json_out)

    # CSV 导出
    if args.csv_out:
        if map_data:
            export_csv(map_data, args.csv_out)
        else:
            print(f"[WARN] {_tr('warn_no_map_csv')}")

    # 摘要
    flash_used = build_data["code"] + build_data["ro_data"] + build_data["rw_data"]
    ram_used = build_data["rw_data"] + build_data["zi_data"]
    print(f"\n{'='*50}")
    print(f"  MCU: {mcu_name_upper} ({mcu_spec['desc']})")
    print(f"  Flash: {flash_used:,} / {mcu_spec['flash']:,} B ({_pct(flash_used, mcu_spec['flash']):.1f}%)")
    print(f"  SRAM:  {ram_used:,} / {mcu_spec['sram']:,} B ({_pct(ram_used, mcu_spec['sram']):.1f}%)")
    print(f"{'='*50}")


if __name__ == "__main__":
    main()