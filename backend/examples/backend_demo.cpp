#include "toyc/backend/ir.hpp"
#include "toyc/backend/riscv32_codegen.hpp"

#include <iostream>
#include <utility>

using namespace toyc::backend;

int main() {
    Program program;
    program.globals.push_back(Global{"counter", 0, false});

    FunctionBuilder factorial("factorial");
    const auto n = factorial.addParameter("n");
    const auto condition = factorial.createTemporary();
    const auto n_minus_one = factorial.createTemporary();
    const auto recursive_value = factorial.createTemporary();
    const auto result = factorial.createTemporary();
    const auto base_case = factorial.createLabel("base");
    const auto recursive_case = factorial.createLabel("recursive");

    factorial.emit(Instruction::binary(condition, BinaryOp::LessEqual, n,
                                       Value::imm(1)));
    factorial.emit(Instruction::branch(condition, base_case, recursive_case));
    factorial.emit(Instruction::makeLabel(base_case));
    factorial.emit(Instruction::ret(Value::imm(1)));
    factorial.emit(Instruction::makeLabel(recursive_case));
    factorial.emit(Instruction::binary(n_minus_one, BinaryOp::Subtract, n,
                                       Value::imm(1)));
    factorial.emit(Instruction::call(recursive_value, "factorial",
                                     {n_minus_one}));
    factorial.emit(
        Instruction::binary(result, BinaryOp::Multiply, n, recursive_value));
    factorial.emit(Instruction::ret(result));
    program.functions.push_back(std::move(factorial).finish());

    FunctionBuilder main_function("main");
    const auto answer = main_function.createTemporary();
    main_function.emit(
        Instruction::call(answer, "factorial", {Value::imm(5)}));
    main_function.emit(
        Instruction::copy(Value::global("counter"), answer));
    main_function.emit(Instruction::ret(answer));
    program.functions.push_back(std::move(main_function).finish());

    std::cout << RiscV32CodeGenerator{}.generate(program);
}
