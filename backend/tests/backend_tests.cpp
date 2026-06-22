#include "toyc/backend/ir.hpp"
#include "toyc/backend/riscv32_codegen.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

using namespace toyc::backend;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testArithmeticAndFrame() {
    Program program;
    FunctionBuilder function("main");
    const auto temporary = function.createTemporary();
    function.emit(Instruction::binary(temporary, BinaryOp::Add, Value::imm(1),
                                      Value::imm(2)));
    function.emit(Instruction::ret(temporary));
    program.functions.push_back(std::move(function).finish());

    const auto assembly = RiscV32CodeGenerator{}.generate(program);
    require(assembly.find("main:") != std::string::npos, "emits function label");
    require(assembly.find("add t2, t0, t1") != std::string::npos,
            "selects RV32 add");
    require(assembly.find("sw ra, -4(t5)") != std::string::npos,
            "saves return address");
    require(assembly.find("mv sp, s0") != std::string::npos,
            "restores stack pointer");
}

void testControlFlowAndGlobal() {
    Program program;
    program.globals.push_back(Global{"g", 7, false});
    FunctionBuilder function("main");
    const auto yes = function.createLabel("yes");
    const auto no = function.createLabel("no");
    function.emit(Instruction::branch(Value::global("g"), yes, no));
    function.emit(Instruction::makeLabel(yes));
    function.emit(Instruction::ret(Value::imm(1)));
    function.emit(Instruction::makeLabel(no));
    function.emit(Instruction::ret(Value::imm(0)));
    program.functions.push_back(std::move(function).finish());

    const auto assembly = RiscV32CodeGenerator{}.generate(program);
    require(assembly.find(".word 7") != std::string::npos, "emits global data");
    require(assembly.find("la t6, g") != std::string::npos, "loads global address");
    require(assembly.find("bnez t0") != std::string::npos, "emits conditional branch");
}

void testCallingConventionWithStackArguments() {
    Program program;
    FunctionBuilder callee("sum9");
    std::vector<Value> parameters;
    for (int index = 0; index < 9; ++index) {
        parameters.push_back(callee.addParameter("p" + std::to_string(index)));
    }
    callee.emit(Instruction::ret(parameters.back()));
    program.functions.push_back(std::move(callee).finish());

    FunctionBuilder caller("main");
    const auto result = caller.createTemporary();
    std::vector<Value> arguments;
    for (int value = 1; value <= 9; ++value) {
        arguments.push_back(Value::imm(value));
    }
    caller.emit(Instruction::call(result, "sum9", arguments));
    caller.emit(Instruction::ret(result));
    program.functions.push_back(std::move(caller).finish());

    const auto assembly = RiscV32CodeGenerator{}.generate(program);
    require(assembly.find("mv a7, t0") != std::string::npos,
            "uses a0-a7 for first eight arguments");
    require(assembly.find("sw t0, 0(sp)") != std::string::npos,
            "passes ninth argument on caller stack");
    require(assembly.find("lw t0, 0(s0)") != std::string::npos,
            "callee reads ninth argument from incoming stack");
}

void testValidation() {
    Program program;
    Function function;
    function.name = "main";
    function.instructions.push_back(Instruction::jump(".missing"));
    program.functions.push_back(std::move(function));
    require(!validate(program).empty(), "rejects undefined branch target");
}

} // namespace

int main() {
    testArithmeticAndFrame();
    testControlFlowAndGlobal();
    testCallingConventionWithStackArguments();
    testValidation();
    std::cout << "backend tests passed\n";
}
