#!/usr/bin/env python3
"""
IR 完整性验证脚本
根据 docs/ir-specification.md §5.3 检查清单进行自动和半自动验证

使用方法：
  python3 scripts/validate-ir.py ir/spi_ir_summary.json
  python3 scripts/validate-ir.py --all-ir        # 验证所有 IR 文件
"""

import json
import sys
import os
from pathlib import Path
from typing import Dict, List, Tuple, Any

class IRValidator:
    def __init__(self, ir_path: str):
        self.ir_path = ir_path
        self.ir = None
        self.errors = []
        self.warnings = []
        self.info = []

        self._load_ir()

    def _load_ir(self):
        """加载 IR JSON 文件"""
        try:
            with open(self.ir_path, 'r', encoding='utf-8') as f:
                self.ir = json.load(f)
        except Exception as e:
            self.errors.append(f"[A.1] JSON 语法错误: {e}")
            sys.exit(1)

    def validate_all(self) -> bool:
        """执行所有检查"""
        print(f"🔍 验证 IR: {self.ir_path}")
        print("=" * 70)

        # 基础检查（§7 验证清单）
        self._check_json_syntax()
        self._check_register_offsets()
        self._check_bitfields()
        self._check_sources()
        self._check_access_types()
        self._check_rc_seq_references()
        self._check_clock_instances_match()
        self._check_interrupts()
        self._check_pending_reviews()
        self._check_confidence()

        # 完整性检查（§5.3 检查清单）
        self._check_configuration_matrix()
        self._check_configuration_strategies()
        self._check_cross_field_constraints()
        self._check_init_sequence_prerequisites()
        self._check_errata_availability()

        # 输出结果
        self._print_results()
        return len(self.errors) == 0

    # ===== §7 基础验证清单 =====

    def _check_json_syntax(self):
        """检查 #1: JSON 语法合法"""
        self.info.append("[✓ 1] JSON 语法合法")

    def _check_register_offsets(self):
        """检查 #2: 寄存器偏移无重叠"""
        registers = self.ir['peripheral'].get('registers', [])
        offsets = []
        for reg in registers:
            offset = int(reg['offset'], 0)
            width = reg['width'] // 8
            offsets.append((offset, offset + width, reg['name']))

        offsets.sort()
        for i in range(len(offsets) - 1):
            end1, start2_actual, name1 = offsets[i]
            start2, end2, name2 = offsets[i+1]
            if end1 > start2:
                self.errors.append(
                    f"[✗ 2] 寄存器偏移重叠: {name1} (0x{offsets[i][0]:03x}) "
                    f"与 {name2} (0x{start2:03x})"
                )

        if not any(e.startswith("[✗ 2]") for e in self.errors):
            self.info.append(f"[✓ 2] 寄存器偏移无重叠 ({len(registers)} 个寄存器)")

    def _check_bitfields(self):
        """检查 #3: 位域无重叠"""
        registers = self.ir['peripheral'].get('registers', [])
        for reg in registers:
            bitfields = reg.get('bitfields', [])
            bits = []
            for bf in bitfields:
                bit_offset = bf['bit_offset']
                bit_width = bf['bit_width']
                bits.append((bit_offset, bit_offset + bit_width, bf['name']))

            bits.sort()
            for i in range(len(bits) - 1):
                end1, start2_actual, name1 = bits[i]
                start2, end2, name2 = bits[i+1]
                if end1 > start2:
                    self.errors.append(
                        f"[✗ 3] 寄存器 {reg['name']} 中位域重叠: "
                        f"{name1} (bit {bits[i][0]}) 与 {name2} (bit {start2})"
                    )

            # 检查不超出寄存器宽度
            for bit_offset, bit_end, bf_name in bits:
                if bit_end > reg['width']:
                    self.errors.append(
                        f"[✗ 4] 位域 {reg['name']}.{bf_name} 超出寄存器宽度 "
                        f"({bit_end} > {reg['width']})"
                    )

        if not any(e.startswith("[✗ 3]") or e.startswith("[✗ 4]") for e in self.errors):
            self.info.append(f"[✓ 3,4] 位域无重叠且不超出宽度")

    def _check_sources(self):
        """检查 #5: 所有字段都有 source"""
        registers = self.ir['peripheral'].get('registers', [])
        missing_source = []

        for reg in registers:
            if not reg.get('source'):
                missing_source.append(f"register {reg['name']}")

            for bf in reg.get('bitfields', []):
                if not bf.get('source'):
                    missing_source.append(f"{reg['name']}.{bf['name']}")

        if missing_source:
            self.warnings.append(f"[⚠ 5] 缺失 source: {', '.join(missing_source[:5])}")
        else:
            self.info.append(f"[✓ 5] 所有字段都有 source 引用")

    def _check_access_types(self):
        """检查 #6: 特殊 access type"""
        special_types = ['W1C', 'W0C', 'RC', 'RC_SEQ', 'WO_TRIGGER']
        registers = self.ir['peripheral'].get('registers', [])
        found_special = {}

        for reg in registers:
            for bf in reg.get('bitfields', []):
                access = bf.get('access')
                if access in special_types:
                    found_special[access] = found_special.get(access, 0) + 1

        if found_special:
            self.info.append(
                f"[✓ 6] 特殊 access type 已标注: "
                f"{', '.join(f'{k}({v})' for k,v in found_special.items())}"
            )
        else:
            self.warnings.append(f"[⚠ 6] 未发现 W1C/W0C/RC_SEQ 等特殊 access type")

    def _check_rc_seq_references(self):
        """检查 #7: RC_SEQ 的 clear_sequence 引用"""
        registers = self.ir['peripheral'].get('registers', [])
        atomic_seqs = {seq['id']: seq for seq in self.ir['peripheral'].get('atomic_sequences', [])}

        missing_refs = []
        for reg in registers:
            for bf in reg.get('bitfields', []):
                if bf.get('access') == 'RC_SEQ':
                    clear_seq = bf.get('clear_sequence')
                    if not clear_seq:
                        missing_refs.append(f"{reg['name']}.{bf['name']} 缺失 clear_sequence")
                    elif clear_seq not in atomic_seqs:
                        missing_refs.append(f"{reg['name']}.{bf['name']} -> {clear_seq} 不存在")

        if missing_refs:
            for ref in missing_refs:
                self.errors.append(f"[✗ 7] {ref}")
        else:
            self.info.append(f"[✓ 7] 所有 RC_SEQ 位域的 clear_sequence 引用有效")

    def _check_clock_instances_match(self):
        """检查 #8: clock[] 与 instances[] 数量匹配"""
        instances = self.ir['peripheral'].get('instances', [])
        clocks = self.ir['peripheral'].get('clock', [])

        instance_names = {inst['name'] for inst in instances}
        clock_instances = {clk['instance'] for clk in clocks}

        if instance_names != clock_instances:
            missing = instance_names - clock_instances
            extra = clock_instances - instance_names
            if missing:
                self.errors.append(f"[✗ 8] 缺失时钟配置: {missing}")
            if extra:
                self.warnings.append(f"[⚠ 8] 多余的时钟配置: {extra}")
        else:
            self.info.append(f"[✓ 8] clock[] 与 instances[] 数量匹配 ({len(instances)} 个)")

    def _check_interrupts(self):
        """检查 #9: interrupts[] 非空"""
        interrupts = self.ir['peripheral'].get('interrupts', [])

        if not interrupts:
            self.warnings.append(f"[⚠ 9] interrupts[] 为空（如无中断需显式说明）")
        else:
            for irq in interrupts:
                if 'irq_number' not in irq:
                    self.errors.append(f"[✗ 9] 中断 {irq.get('name', '?')} 缺失 irq_number")

            if not any(e.startswith("[✗ 9]") for e in self.errors):
                self.info.append(f"[✓ 9] 中断配置完整 ({len(interrupts)} 个中断)")

    def _check_pending_reviews(self):
        """检查 #10: pending_reviews"""
        pending = self.ir['peripheral'].get('pending_reviews', [])
        unresolved = [p for p in pending if not p.get('resolved', False)]

        if unresolved:
            self.warnings.append(
                f"[⚠ 10] 存在 {len(unresolved)} 个未解决的待审核项。"
                f"code-gen 应等待人工审核完成。"
            )
            for item in unresolved[:3]:
                self.warnings.append(
                    f"       - {item.get('review_id', '?')}: {item.get('question', '')[:60]}..."
                )
        else:
            self.info.append(f"[✓ 10] pending_reviews 已全部解决")

    def _check_confidence(self):
        """检查 #11: 平均置信度"""
        metadata = self.ir['peripheral'].get('generation_metadata', {})
        avg_conf = metadata.get('average_confidence', 1.0)

        if avg_conf < 0.85:
            self.errors.append(
                f"[✗ 11] 平均置信度过低: {avg_conf:.2f} < 0.85。"
                f"整体 IR 需人工审核。"
            )
        else:
            self.info.append(f"[✓ 11] 平均置信度: {avg_conf:.2f} (合格)")

    # ===== §5.3 完整性检查清单 =====

    def _check_configuration_matrix(self):
        """检查 A. 模式完整性"""
        modes = self.ir['peripheral'].get('functional_model', {}).get('operation_modes', [])

        if not modes:
            self.warnings.append("[⚠ A] operation_modes[] 为空或缺失")
        else:
            self.info.append(
                f"[✓ A] operation_modes 配置矩阵包含 {len(modes)} 种模式"
            )

            # 检查 master/slave 对称性（简单启发式）
            masters = [m for m in modes if 'master' in m['name'].lower()]
            slaves = [m for m in modes if 'slave' in m['name'].lower()]

            if masters and not slaves:
                self.warnings.append(
                    "[⚠ A.2] 仅包含 master 模式，缺少 slave 对称版本？"
                )
            elif len(masters) != len(slaves):
                self.warnings.append(
                    f"[⚠ A.2] master/slave 模式数量不对称 ({len(masters)} vs {len(slaves)})"
                )

    def _check_configuration_strategies(self):
        """检查 B. 策略完整性"""
        strategies = self.ir['peripheral'].get('configuration_strategies', [])

        if not strategies:
            self.warnings.append(
                "[⚠ B] configuration_strategies[] 缺失。"
                "应明确列出 single-master, multimaster 等重要配置策略。"
            )
        else:
            self.info.append(
                f"[✓ B] configuration_strategies 包含 {len(strategies)} 种策略"
            )

            for strat in strategies:
                if 'prerequisites' not in strat or 'constraints' not in strat:
                    self.warnings.append(
                        f"[⚠ B] 策略 {strat.get('id')} 缺失 prerequisites 或 constraints"
                    )

    def _check_cross_field_constraints(self):
        """检查 C. 约束完整性"""
        constraints = self.ir['peripheral'].get('cross_field_constraints', [])

        if not constraints:
            self.warnings.append(
                "[⚠ C] cross_field_constraints[] 缺失。"
                "应记录字段间的依赖关系（如 DFF ↔ CRC, LOCK 字段, AFIO 顺序）。"
            )
        else:
            self.info.append(
                f"[✓ C] cross_field_constraints 包含 {len(constraints)} 个约束"
            )

            for constr in constraints:
                if 'violation_effect' not in constr:
                    self.warnings.append(
                        f"[⚠ C] 约束 {constr.get('id')} 缺失 violation_effect"
                    )

    def _check_init_sequence_prerequisites(self):
        """检查 D. 初始化顺序依赖"""
        init_seq = self.ir['peripheral'].get('functional_model', {}).get('init_sequence', [])

        has_requires = any('requires' in step for step in init_seq)

        if not has_requires:
            self.warnings.append(
                "[⚠ D] init_sequence 中未标注 requires 前置条件。"
                "应明确标注步骤间的依赖关系（如 AFIO → GPIO）。"
            )
        else:
            self.info.append(
                "[✓ D] init_sequence 中有前置条件标注"
            )

    def _check_errata_availability(self):
        """检查 E. Errata 文档"""
        errata = self.ir['peripheral'].get('errata', [])
        pending_errata = [e for e in errata if e.get('id') == 'PENDING_ERRATA_CHECK']

        if pending_errata:
            self.warnings.append(
                "[⚠ E] Errata 文件缺失或未处理。"
                "应从 docs/ 中查找对应的 STM32 errata sheet（如 ES0306）。"
            )
        else:
            self.info.append(
                f"[✓ E] Errata 已处理或确认无关 ({len(errata)} 项)"
            )

    def _print_results(self):
        """输出验证结果"""
        print("\n📋 验证结果:")
        print("=" * 70)

        if self.info:
            print("\n✅ 通过项:")
            for msg in self.info:
                print(f"  {msg}")

        if self.warnings:
            print("\n⚠️  警告项:")
            for msg in self.warnings:
                print(f"  {msg}")

        if self.errors:
            print("\n❌ 错误项:")
            for msg in self.errors:
                print(f"  {msg}")

        print("\n" + "=" * 70)
        total = len(self.info) + len(self.warnings) + len(self.errors)
        status = "✅ 通过" if not self.errors else "❌ 失败"
        print(f"总计: {len(self.info)}/{total} 通过 | {len(self.warnings)} 警告 | {len(self.errors)} 错误")
        print(f"状态: {status}")

        return len(self.errors) == 0

def main():
    if len(sys.argv) < 2:
        print("使用方法: python3 scripts/validate-ir.py <ir_file>")
        print("          python3 scripts/validate-ir.py --all-ir")
        sys.exit(1)

    if sys.argv[1] == '--all-ir':
        ir_dir = Path('ir')
        ir_files = sorted(ir_dir.glob('*_ir_summary.json'))
        if not ir_files:
            print("❌ 未找到 IR 文件")
            sys.exit(1)

        all_passed = True
        for ir_file in ir_files:
            validator = IRValidator(str(ir_file))
            if not validator.validate_all():
                all_passed = False
            print()

        sys.exit(0 if all_passed else 1)
    else:
        validator = IRValidator(sys.argv[1])
        passed = validator.validate_all()
        sys.exit(0 if passed else 1)

if __name__ == '__main__':
    main()
