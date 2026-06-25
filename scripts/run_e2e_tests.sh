#!/usr/bin/env bash
# 端到端测试脚本：ToyC 源码 → RISC-V32 汇编 → 汇编链接运行 → 对比退出码。
#
# 用法：
#   bash scripts/run_e2e_tests.sh           # 普通模式
#   bash scripts/run_e2e_tests.sh --opt     # 优化模式（编译器传 -opt）
#   bash scripts/run_e2e_tests.sh --verbose # 详细输出
#
# 前置条件：
#   - ToyC 编译器已构建（build/toyc）
#   - RISC-V 工具链已安装（riscv64-linux-gnu-gcc, qemu-riscv32）
#   - 或使用评测环境提供的工具链
#
# 测试集：
#   - tests/basic/      基础功能测试
#   - tests/control_flow/ 控制流测试
#   - tests/performance/  性能基准测试

set -euo pipefail

# ---- 配置 ----
BUILD_DIR="${BUILD_DIR:-build}"
COMPILER="${COMPILER:-$BUILD_DIR/toyc}"
EXPECTED_FILE="tests/basic/expected.tsv"
OUT_DIR="${OUT_DIR:-$BUILD_DIR/e2e_out}"
RISCV_GCC="${RISCV_GCC:-riscv64-linux-gnu-gcc}"
QEMU="${QEMU:-qemu-riscv32}"
OPT_FLAG=""
VERBOSE=false

# ---- 参数解析 ----
while [[ $# -gt 0 ]]; do
    case "$1" in
        --opt)
            OPT_FLAG="-opt"
            ;;
        --verbose)
            VERBOSE=true
            ;;
        --compiler)
            COMPILER="$2"
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 1
            ;;
    esac
    shift
done

# ---- 初始化 ----
mkdir -p "$OUT_DIR"

# 最小 _start 入口：调用 main，然后用 exit 系统调用退出
cat > "$OUT_DIR/_start.s" <<'ASM'
  .section .text.start
  .globl _start
_start:
  call main
  li a7, 93
  ecall
ASM

PASS=0
FAIL=0

# ---- 辅助函数 ----
run_test() {
    local tc_file="$1"
    local expect="$2"
    local base
    base=$(basename "$tc_file" .tc)
    local asm_file="$OUT_DIR/${base}.s"
    local exe_file="$OUT_DIR/${base}.elf"

    $VERBOSE && echo "  compiling: $tc_file → $asm_file"

    # 1. ToyC → RISC-V32 汇编
    if ! "$COMPILER" $OPT_FLAG < "$tc_file" > "$asm_file" 2>"$OUT_DIR/${base}.stderr"; then
        echo "  [FAIL] $tc_file — compiler returned non-zero"
        FAIL=$((FAIL + 1))
        $VERBOSE && cat "$OUT_DIR/${base}.stderr" >&2
        return
    fi

    # 2. 汇编 + 链接（nostdlib+static 避免 64/32 CRT 不兼容，-static 避免 PIE 动态链接）
    if ! "$RISCV_GCC" -march=rv32im -mabi=ilp32 \
         -nostdlib -nostartfiles -static \
         -Wl,-e,_start -Wl,-Ttext=0x10000 -Wl,--build-id=none \
         -o "$exe_file" "$OUT_DIR/_start.s" "$asm_file" 2>"$OUT_DIR/${base}.linkerr"; then
        echo "  [FAIL] $tc_file — assembler/linker failed"
        FAIL=$((FAIL + 1))
        $VERBOSE && cat "$OUT_DIR/${base}.linkerr" >&2
        return
    fi

    # 3. 运行
    local actual=0
    actual=$("$QEMU" "$exe_file" 2>/dev/null; echo $?)

    # 4. 对比退出码
    if [ "$actual" -eq "$expect" ]; then
        echo "  [PASS] $tc_file → $actual"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $tc_file — expected $expect, got $actual"
        FAIL=$((FAIL + 1))
    fi
}

# ---- 主流程 ----
echo "=== ToyC End-to-End Tests ==="
if [ -n "$OPT_FLAG" ]; then
    echo "Mode: optimized (-opt)"
else
    echo "Mode: normal"
fi
echo ""

# 检查编译器
if [ ! -x "$COMPILER" ] && [ ! -f "$COMPILER" ]; then
    echo "error: compiler not found at $COMPILER" >&2
    echo "Build first: cmake --build $BUILD_DIR" >&2
    exit 1
fi

# 从 expected.tsv 读取测试清单
echo "--- Basic Tests ---"
if [ -f "$EXPECTED_FILE" ]; then
    while IFS=$'\t' read -r tc_file expect || [ -n "$tc_file" ]; do
        # 跳过注释和空行
        case "$tc_file" in
            ""|\#*) continue ;;
        esac
        if [ -f "$tc_file" ]; then
            run_test "$tc_file" "$expect"
        else
            echo "  [SKIP] $tc_file — file not found"
        fi
    done < "$EXPECTED_FILE"
else
    echo "  warning: expected file $EXPECTED_FILE not found"
fi

# 额外扫描 tests/control_flow/ 和 tests/performance/ 中不在 TSV 的 .tc 文件
for dir in tests/control_flow tests/performance; do
    if [ -d "$dir" ]; then
        echo "--- $(basename "$dir" | sed 's/_/ /g' | awk '{for(i=1;i<=NF;i++) $i=toupper(substr($i,1,1)) substr($i,2)}1') Tests ---"
        for tc in "$dir"/*.tc; do
            [ -f "$tc" ] || continue
            # 检查是否已在 TSV 中
            if grep -q "^$tc" "$EXPECTED_FILE" 2>/dev/null; then
                continue
            fi
            echo "  [SKIP] $tc — no expected value in TSV (add to $EXPECTED_FILE)"
        done
    fi
done

# ---- 汇总 ----
echo ""
echo "=== Results ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
if [ $FAIL -eq 0 ] && [ $PASS -gt 0 ]; then
    echo "All tests passed."
else
    echo "Total:  $((PASS + FAIL))"
fi

[ $FAIL -eq 0 ] || exit 1
