#include "toyc/backend/ir.hpp"
#include "toyc/backend/optimizer.hpp"
#include "toyc/backend/riscv32_codegen.hpp"
#include "toyc/backend/text_ir.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
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
    require(assembly.find("  add ") != std::string::npos ||
                assembly.find("  addi ") != std::string::npos,
            "selects RV32 integer addition");
    require(assembly.find("sw ra, -4(t5)") != std::string::npos,
            "saves return address");
    require(assembly.find("mv sp, s0") != std::string::npos,
            "restores stack pointer");
    require(assembly.find("sw s1, -12(t5)") != std::string::npos,
            "preserves an allocated callee-saved register");
    require(assembly.find("mv a0, s1") != std::string::npos,
            "returns the allocated temporary register");
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
    require(assembly.find("a7") != std::string::npos,
            "uses a0-a7 for first eight arguments");    require(assembly.find("sw t0, 0(sp)") != std::string::npos,
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

void testStructuredControlFlowAndShortCircuit() {
    Program program;
    FunctionBuilder function("main");
    const auto local = function.createLocal("counter");
    function.emit(Instruction::copy(local, Value::imm(0)));

    const auto logical = function.emitLogicalAnd(
        Value::imm(1), [](FunctionBuilder& builder) {
            const auto result = builder.createTemporary();
            builder.emit(Instruction::call(result, "side_effect", {}));
            return result;
        });

    function.emitIf(
        logical,
        [local](FunctionBuilder& builder) {
            builder.emit(Instruction::copy(local, Value::imm(1)));
        },
        [local](FunctionBuilder& builder) {
            builder.emit(Instruction::copy(local, Value::imm(2)));
        });

    function.emitWhile(
        [local](FunctionBuilder&) { return local; },
        [](FunctionBuilder& builder) {
            builder.emitContinue();
            builder.emitBreak();
        });
    function.emit(Instruction::ret(local));
    program.functions.push_back(std::move(function).finish());

    const auto errors = validate(program);
    require(errors.empty(), "structured lowering creates valid IR");
    const auto assembly = RiscV32CodeGenerator{}.generate(program);
    const auto rhs = assembly.find("and_rhs");
    const auto call = assembly.find("call side_effect");
    require(rhs != std::string::npos && call > rhs,
            "short-circuit right operand is emitted on its own path");
    require(assembly.find("while_condition") != std::string::npos,
            "emits while loop labels");
}

void testOptimizer() {
    Program program;
    FunctionBuilder function("main");
    const auto folded = function.createTemporary();
    const auto dead = function.createTemporary();
    function.emit(Instruction::binary(folded, BinaryOp::Multiply,
                                      Value::imm(6), Value::imm(7)));
    function.emit(Instruction::binary(dead, BinaryOp::Add, Value::imm(1),
                                      Value::imm(2)));
    function.emit(Instruction::ret(folded));
    program.functions.push_back(std::move(function).finish());

    const auto optimized = optimize(std::move(program));
    const auto& instructions = optimized.functions.front().instructions;
    require(instructions.size() <= 2, "removes unused temporaries");
    const auto& result = instructions.back();
    require(result.kind == InstructionKind::Return,
            "keeps the observable return");
    require(result.left && result.left->kind == ValueKind::Immediate &&
                result.left->immediate == 42,
            "folds and propagates the returned constant");}

void testTextIrParser() {
    std::istringstream input{
        "global @answer 0\n"
        "function main\n"
        "binary %t0 add 40 2\n"
        "copy @answer %t0\n"
        "return %t0\n"
        "end\n"};
    const auto parsed = parseTextIr(input);
    require(parsed.ok(), "parses textual IR");
    require(parsed.program.globals.size() == 1,
            "parses a global declaration");
    require(parsed.program.functions.size() == 1,
            "parses a function");
    require(parsed.program.functions.front().instructions.size() == 3,
            "parses function instructions");
}

void testTemporarySpilling() {
    Program program;
    FunctionBuilder function("main");
    std::vector<Value> temporaries;
    for (int index = 0; index < 12; ++index) {
        const auto temporary = function.createTemporary();
        temporaries.push_back(temporary);
        function.emit(Instruction::copy(temporary, Value::imm(index)));
    }
    function.emit(Instruction::ret(temporaries.back()));
    program.functions.push_back(std::move(function).finish());

    const auto assembly = RiscV32CodeGenerator{}.generate(program);
    require(assembly.find("sw s11") != std::string::npos,
            "uses all available temporary registers");
    require(assembly.find("sw t0, -56(s0)") != std::string::npos,
            "spills excess temporaries to the stack");
}

} // namespace

int main() {
    testArithmeticAndFrame();
    testControlFlowAndGlobal();
    testCallingConventionWithStackArguments();
    testValidation();
    testStructuredControlFlowAndShortCircuit();
    testOptimizer();
    testTextIrParser();
    testTemporarySpilling();
    std::cout << "backend tests passed\n";
}
