#!/usr/bin/env bash
# Unix 冒烟测试入口：CMake 配置 → 编译 → ctest → 用编译好的 toyc 编译单个 .tc 源文件，
# 验证编译器端到端管线是否正常工作。

set -euo pipefail

build_dir="${1:-build}"

cmake -S . -B "$build_dir"
cmake --build "$build_dir"
ctest --test-dir "$build_dir" --output-on-failure

compiler=""
for candidate in "$build_dir/toyc" "$build_dir/ToyC-Compiler"; do
    if [[ -x "$candidate" ]]; then
        compiler="$candidate"
        break
    fi
done

if [[ -z "$compiler" ]]; then
    echo "Compiler executable not found in $build_dir" >&2
    find "$build_dir" -maxdepth 2 -type f -perm -111 >&2 || true
    exit 1
fi

mkdir -p "$build_dir/smoke"
"$compiler" < tests/basic/return_7.tc > "$build_dir/smoke/return_7.s"
test -s "$build_dir/smoke/return_7.s"

echo "Smoke tests passed with compiler: $compiler"
