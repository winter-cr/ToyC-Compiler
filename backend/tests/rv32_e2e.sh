#!/usr/bin/env sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: $0 BACKEND_CLI RISCV_GCC QEMU_RISCV32 INPUT_IR" >&2
    exit 2
fi

backend_cli=$1
riscv_gcc=$2
qemu_riscv32=$3
input_ir=$4
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

"$backend_cli" -opt --no-comments < "$input_ir" > "$work_dir/program.s"

cat > "$work_dir/start.s" <<'EOF'
  .section .text.start
  .globl _start
_start:
  call main
  li a7, 93
  ecall
EOF

"$riscv_gcc" \
    -march=rv32im -mabi=ilp32 \
    -nostdlib -nostartfiles \
    -Wl,-e,_start -Wl,-Ttext=0x10000 \
    "$work_dir/start.s" "$work_dir/program.s" \
    -o "$work_dir/program"

set +e
"$qemu_riscv32" "$work_dir/program"
status=$?
set -e

if [ "$status" -ne 120 ]; then
    echo "expected RV32 program exit code 120, got $status" >&2
    exit 1
fi

echo "RV32 end-to-end test passed"
