# Phase 2: 错误分类与修复决策引擎

**日期**: 2026-03-26
**目标**: 为编译、链接、调试三个阶段建立智能修复系统
**预计工作量**: 3-4 小时

---

## 第一部分：错误分类体系设计

### 1. 编译阶段错误分类

#### 第 1 层：错误大类
```
编译错误
├─ 语法错误 (syntax_error)
├─ 类型错误 (type_error)
├─ 声明错误 (declaration_error)
├─ 预处理错误 (preprocessor_error)
└─ 其他编译错误 (other_error)
```

#### 第 2 层：具体错误类型与识别规则

**1.1 语法错误 (syntax_error)**

```json
{
  "type": "syntax_error",
  "subtypes": [
    {
      "name": "missing_semicolon",
      "patterns": ["expected ';'", "expected declaration"],
      "confidence": "high",
      "fix_strategy": "add_semicolon",
      "root_causes": [
        "行末缺少分号",
        "大括号匹配错误"
      ],
      "ai_actions": [
        "定位错误行前后",
        "检查分号和大括号"
      ]
    },
    {
      "name": "mismatched_braces",
      "patterns": ["expected '}'", "expected declaration"],
      "confidence": "medium",
      "fix_strategy": "fix_braces",
      "root_causes": [
        "{}未配对",
        "()未配对"
      ]
    },
    {
      "name": "invalid_token",
      "patterns": ["expected", "unexpected token"],
      "confidence": "low",
      "fix_strategy": "manual_review"
    }
  ]
}
```

**1.2 类型错误 (type_error)**

```json
{
  "type": "type_error",
  "subtypes": [
    {
      "name": "type_mismatch",
      "patterns": [
        "incompatible pointer type",
        "assignment to .* from .* with loss of precision",
        "conflicting types for"
      ],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "cast_conversion",
          "priority": 1,
          "when": "明确需要类型转换"
        },
        {
          "strategy": "change_variable_type",
          "priority": 2,
          "when": "变量声明类型错误"
        },
        {
          "strategy": "change_function_signature",
          "priority": 3,
          "when": "函数参数类型不匹配"
        }
      ]
    },
    {
      "name": "implicit_declaration",
      "patterns": [
        "implicit declaration of function",
        "undeclared identifier"
      ],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "add_include",
          "priority": 1,
          "hint": "查找函数所在的标准库或本地头文件"
        },
        {
          "strategy": "add_forward_declaration",
          "priority": 2
        }
      ]
    }
  ]
}
```

**1.3 声明错误 (declaration_error)**

```json
{
  "type": "declaration_error",
  "subtypes": [
    {
      "name": "undefined_identifier",
      "patterns": ["undeclared identifier", "'XXX' undeclared"],
      "confidence": "high",
      "root_causes": [
        "变量未声明",
        "头文件未包含",
        "变量名拼写错误"
      ],
      "fix_strategies": [
        {
          "strategy": "add_include",
          "priority": 1
        },
        {
          "strategy": "declare_variable",
          "priority": 2
        },
        {
          "strategy": "check_spelling",
          "priority": 3
        }
      ]
    },
    {
      "name": "redeclared_identifier",
      "patterns": ["redeclared", "conflicting"],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "remove_duplicate_declaration",
          "priority": 1
        },
        {
          "strategy": "use_include_guard",
          "priority": 2,
          "when": "头文件被重复包含"
        }
      ]
    }
  ]
}
```

**1.4 预处理错误 (preprocessor_error)**

```json
{
  "type": "preprocessor_error",
  "subtypes": [
    {
      "name": "include_not_found",
      "patterns": ["No such file or directory", "include.*not found"],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "fix_include_path",
          "priority": 1
        },
        {
          "strategy": "check_file_exists",
          "priority": 2
        }
      ]
    },
    {
      "name": "macro_error",
      "patterns": ["#error"],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "define_required_macro",
          "priority": 1
        },
        {
          "strategy": "check_config",
          "priority": 2
        }
      ]
    }
  ]
}
```

---

### 2. 链接阶段错误分类

#### 第 1 层：错误大类
```
链接错误
├─ 符号错误 (symbol_error)
├─ 内存错误 (memory_error)
├─ 脚本错误 (script_error)
└─ 库错误 (library_error)
```

#### 第 2 层：具体错误类型

**2.1 符号错误 (symbol_error)**

```json
{
  "type": "symbol_error",
  "subtypes": [
    {
      "name": "undefined_symbol",
      "patterns": ["undefined reference to"],
      "confidence": "high",
      "extraction": "提取引号内的符号名",
      "root_causes": [
        "函数未实现（在编译阶段应该处理）",
        "源文件未被编译（Makefile/CMakeLists 问题）",
        "库文件未链接（缺少 -l*** 或 -L**）",
        "弱符号需要覆盖（__weak 修饰）"
      ],
      "fix_strategies": [
        {
          "strategy": "implement_function",
          "priority": 1,
          "when": "函数声明存在但无实现",
          "actions": [
            "检查源码是否有该符号",
            "如无则回编译阶段补充"
          ]
        },
        {
          "strategy": "add_source_file",
          "priority": 2,
          "when": "源文件未被编译",
          "actions": [
            "检查 .map 文件",
            "确认对象文件是否存在"
          ]
        },
        {
          "strategy": "add_library",
          "priority": 3,
          "when": "库函数缺失",
          "actions": [
            "识别符号来自哪个库",
            "在链接命令中添加 -l***"
          ]
        }
      ],
      "detection_hints": [
        "查看是否在 .map 文件中找到该符号",
        "查看对象文件列表是否完整"
      ]
    },
    {
      "name": "duplicate_symbol",
      "patterns": ["multiple definition of"],
      "confidence": "high",
      "extraction": "提取符号名",
      "root_causes": [
        "头文件中有函数实现（不是声明）",
        "多个源文件定义同一符号",
        "头文件缺少 include guard"
      ],
      "fix_strategies": [
        {
          "strategy": "move_to_source_file",
          "priority": 1,
          "when": "头文件包含实现"
        },
        {
          "strategy": "add_include_guard",
          "priority": 2,
          "when": "头文件被重复包含"
        },
        {
          "strategy": "remove_duplicate_definition",
          "priority": 3
        }
      ]
    }
  ]
}
```

**2.2 内存错误 (memory_error)**

```json
{
  "type": "memory_error",
  "subtypes": [
    {
      "name": "flash_overflow",
      "patterns": ["region.*FLASH.*overflowed"],
      "confidence": "high",
      "extraction": "提取溢出大小",
      "root_causes": [
        "代码过大",
        "Flash 大小配置错误",
        "未启用优化"
      ],
      "fix_strategies": [
        {
          "strategy": "enable_optimization",
          "priority": 1,
          "actions": [
            "修改 CFLAGS: -Os（或 -O2）",
            "启用 -fdata-sections -ffunction-sections",
            "启用链接选项 --gc-sections"
          ]
        },
        {
          "strategy": "verify_flash_size",
          "priority": 2,
          "actions": [
            "检查 config/project.env 中的 FLASH_SIZE",
            "对照芯片手册"
          ]
        },
        {
          "strategy": "reduce_code_size",
          "priority": 3,
          "actions": [
            "移除不使用的函数",
            "考虑使用库函数替代自定义实现"
          ]
        }
      ]
    },
    {
      "name": "ram_overflow",
      "patterns": ["region.*RAM.*overflowed"],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "move_to_flash",
          "priority": 1,
          "actions": [
            "将可初始化数据改为 const",
            "使用 __attribute__((section)) 移至其他区域"
          ]
        },
        {
          "strategy": "reduce_global_data",
          "priority": 2,
          "actions": [
            "减少全局数组大小",
            "考虑动态分配"
          ]
        }
      ]
    }
  ]
}
```

**2.3 脚本错误 (script_error)**

```json
{
  "type": "script_error",
  "subtypes": [
    {
      "name": "invalid_linker_script",
      "patterns": ["error.*linker script"],
      "confidence": "high",
      "fix_strategies": [
        {
          "strategy": "fix_script_syntax",
          "priority": 1
        },
        {
          "strategy": "verify_memory_regions",
          "priority": 2,
          "actions": [
            "检查 MEMORY {} 块",
            "对照芯片手册"
          ]
        }
      ]
    }
  ]
}
```

---

### 3. 调试阶段验证分类

#### 验证项目树

```json
{
  "stage": "debug",
  "validation_items": [
    {
      "category": "initialization",
      "checks": [
        {
          "name": "module_init_complete",
          "indicators": [
            "serial_output 包含 'OK' 或 'INIT'",
            "status_register 的 READY 位被设置"
          ],
          "expected_state": "module initialized"
        },
        {
          "name": "clock_enabled",
          "indicators": [
            "clock_register 有非零值",
            "频率符合配置"
          ]
        }
      ]
    },
    {
      "category": "communication",
      "checks": [
        {
          "name": "message_send",
          "indicators": [
            "send_count > 0",
            "no error flags"
          ]
        },
        {
          "name": "message_receive",
          "indicators": [
            "receive_count > 0",
            "data integrity"
          ]
        }
      ]
    },
    {
      "category": "performance",
      "checks": [
        {
          "name": "timing_correct",
          "indicators": [
            "time_interval matches expected",
            "no timeout errors"
          ]
        }
      ]
    }
  ]
}
```

---

## 第二部分：修复决策引擎设计

### 决策树结构

```python
decision_tree = {
    "compile": {
        "syntax_error": {
            "missing_semicolon": {
                "action": "locate_error_line",
                "next": "check_previous_line",
                "fallback": "ask_ai"
            }
        },
        "type_error": {
            "type_mismatch": {
                "action": "analyze_mismatch",
                "options": [
                    "add_cast",
                    "change_type",
                    "change_function_signature"
                ]
            }
        }
    },
    "link": {
        "symbol_error": {
            "undefined_symbol": {
                "action": "check_map_file",
                "decision": {
                    "symbol_in_map": "add_to_link_command",
                    "symbol_not_in_map": "back_to_compile"
                }
            }
        },
        "memory_error": {
            "flash_overflow": {
                "action": "enable_optimization",
                "if_not_enough": "need_manual_review"
            }
        }
    },
    "debug": {
        "init_failed": {
            "action": "check_register_init",
            "fix": "back_to_compile_with_logging"
        },
        "communication_error": {
            "action": "verify_timing",
            "fix": "adjust_baud_or_timing"
        }
    }
}
```

---

## 第三部分：实现计划

### 3.1 创建错误分类库 `error-classifier.sh`

```bash
#!/bin/bash
# 核心函数：
classify_compile_error(error_msg) → error_type, subtype, confidence
classify_link_error(error_msg) → error_type, subtype, symbol
classify_debug_issue(snapshot_data) → issue_type, severity

# 返回格式：JSON
{
  "type": "syntax_error",
  "subtype": "missing_semicolon",
  "confidence": "high",
  "file": "src/main.c",
  "line": 42,
  "suggested_fixes": [
    {
      "priority": 1,
      "action": "add_semicolon",
      "description": "行末添加分号"
    }
  ]
}
```

### 3.2 创建决策引擎 `repair-decision-engine.sh`

```bash
#!/bin/bash
# 核心函数：
decide_repair_action(error_classification) → repair_action, confidence

# 输入：error_classification (从 error-classifier 输出)
# 输出：repair_action JSON
{
  "action": "implement_function",
  "priority": 1,
  "steps": [
    "检查源码中是否有该符号定义",
    "如无则在相应 .c 文件中添加实现"
  ],
  "ai_instructions": "请在 src/driver.c 中实现 can_init() 函数"
}
```

### 3.3 集成到各 Agent

- **compiler-agent.sh**: 调用 error-classifier 和 repair-decision-engine
- **linker-agent.sh**: 调用 error-classifier（链接版本）和决策引擎
- **debugger-agent.sh**: 调用 debug 分类器和决策引擎

---

## 预期效果

### 修复准确率提升

```
当前（Phase 1）:
- AI 基于错误消息随机尝试修复
- 成功率: ~40-50%
- 平均需要: 3-5 轮才能正确修复同一错误

改进后（Phase 2）:
- AI 基于分类和决策树有针对性修复
- 成功率: ~75-85%
- 平均需要: 1-2 轮修复
```

### 用户体验改进

```
Before:
[COMPILE ERROR]
Type: undeclared identifier
Message: 'can_init' undeclared
→ AI: [随机尝试] 假设变量拼写错误，改为 can_init...
→ 编译仍失败，继续尝试...

After:
[COMPILE ERROR]
Type: undeclared_identifier
Classification: {
  "type": "declaration_error",
  "subtype": "undefined_identifier",
  "likely_causes": ["头文件未包含", "变量未声明"],
  "suggested_fixes": [
    "priority 1: add #include for can_init",
    "priority 2: declare can_init function"
  ]
}
→ AI: [有根据地判断] 这是函数未声明错误，可能需要头文件或前向声明
→ 修复成功率大幅提升
```

---

## 下一步行动

1. **实现 error-classifier.sh** (1.5 小时)
   - 编译错误分类函数
   - 链接错误分类函数
   - 调试问题分类函数

2. **实现 repair-decision-engine.sh** (1.5 小时)
   - 决策树逻辑
   - 行动生成器
   - AI 指令格式化

3. **集成到各 Agent** (1 小时)
   - 修改 compiler-agent.sh
   - 修改 linker-agent.sh
   - 修改 debugger-agent.sh

4. **测试和验证** (1 小时)
   - 单元测试
   - 集成测试
   - 准确率验证

---

**准备好实现这个系统了吗？**
