#include "toyc/backend/ir.hpp"

#include <stdexcept>
#include <unordered_set>

namespace toyc::backend {

Value Value::imm(std::int32_t value) {
    Value result;
    result.kind = ValueKind::Immediate;
    result.immediate = value;
    return result;
}

Value Value::vreg(std::uint32_t id) {
    Value result;
    result.kind = ValueKind::VirtualRegister;
    result.id = id;
    return result;
}

Value Value::local(std::uint32_t id, std::string debug_name) {
    Value result;
    result.kind = ValueKind::Local;
    result.id = id;
    result.name = std::move(debug_name);
    return result;
}

Value Value::global(std::string name) {
    Value result;
    result.kind = ValueKind::Global;
    result.name = std::move(name);
    return result;
}

std::string Value::toString() const {
    switch (kind) {
    case ValueKind::Immediate:
        return std::to_string(immediate);
    case ValueKind::VirtualRegister:
        return "%t" + std::to_string(id);
    case ValueKind::Local:
        return name.empty() ? "%l" + std::to_string(id)
                            : "%" + name + "." + std::to_string(id);
    case ValueKind::Global:
        return "@" + name;
    }
    return "<invalid>";
}

Instruction Instruction::makeLabel(std::string label_name) {
    Instruction result;
    result.kind = InstructionKind::Label;
    result.label = std::move(label_name);
    return result;
}

Instruction Instruction::copy(Value destination_value, Value source) {
    Instruction result;
    result.kind = InstructionKind::Copy;
    result.destination = std::move(destination_value);
    result.left = std::move(source);
    return result;
}

Instruction Instruction::unary(Value destination_value, UnaryOp op, Value operand) {
    Instruction result;
    result.kind = InstructionKind::Unary;
    result.destination = std::move(destination_value);
    result.left = std::move(operand);
    result.unary_op = op;
    return result;
}

Instruction Instruction::binary(Value destination_value, BinaryOp op, Value lhs, Value rhs) {
    Instruction result;
    result.kind = InstructionKind::Binary;
    result.destination = std::move(destination_value);
    result.left = std::move(lhs);
    result.right = std::move(rhs);
    result.binary_op = op;
    return result;
}

Instruction Instruction::jump(std::string target) {
    Instruction result;
    result.kind = InstructionKind::Jump;
    result.label = std::move(target);
    return result;
}

Instruction Instruction::branch(Value condition, std::string true_target,
                                std::string false_target) {
    Instruction result;
    result.kind = InstructionKind::Branch;
    result.left = std::move(condition);
    result.label = std::move(true_target);
    result.false_label = std::move(false_target);
    return result;
}

Instruction Instruction::call(std::optional<Value> destination_value,
                              std::string function_name,
                              std::vector<Value> call_arguments) {
    Instruction result;
    result.kind = InstructionKind::Call;
    result.destination = std::move(destination_value);
    result.callee = std::move(function_name);
    result.arguments = std::move(call_arguments);
    return result;
}

Instruction Instruction::ret(std::optional<Value> value) {
    Instruction result;
    result.kind = InstructionKind::Return;
    result.left = std::move(value);
    return result;
}

FunctionBuilder::FunctionBuilder(std::string name, bool returns_value) {
    function_.name = std::move(name);
    function_.returns_value = returns_value;
}

Value FunctionBuilder::addParameter(std::string name) {
    const auto id = next_local_++;
    function_.parameters.push_back(Parameter{id, name});
    return Value::local(id, std::move(name));
}

Value FunctionBuilder::createLocal(std::string debug_name) {
    return Value::local(next_local_++, std::move(debug_name));
}

Value FunctionBuilder::createTemporary() {
    return Value::vreg(next_temporary_++);
}

std::string FunctionBuilder::createLabel(std::string prefix) {
    return "." + function_.name + "_" + std::move(prefix) + "_" +
           std::to_string(next_label_++);
}

void FunctionBuilder::emit(Instruction instruction) {
    function_.instructions.push_back(std::move(instruction));
}

Function FunctionBuilder::finish() && {
    return std::move(function_);
}

std::vector<std::string> validate(const Program& program) {
    std::vector<std::string> errors;
    std::unordered_set<std::string> symbols;

    for (const auto& global : program.globals) {
        if (global.name.empty()) {
            errors.emplace_back("global variable has an empty name");
        } else if (!symbols.insert(global.name).second) {
            errors.push_back("duplicate global symbol: " + global.name);
        }
    }

    for (const auto& function : program.functions) {
        if (function.name.empty()) {
            errors.emplace_back("function has an empty name");
            continue;
        }
        if (!symbols.insert(function.name).second) {
            errors.push_back("duplicate global symbol: " + function.name);
        }

        std::unordered_set<std::string> labels;
        for (const auto& instruction : function.instructions) {
            if (instruction.kind == InstructionKind::Label &&
                !labels.insert(instruction.label).second) {
                errors.push_back("duplicate label in " + function.name + ": " +
                                 instruction.label);
            }
        }

        const auto check_target = [&](const std::string& target) {
            if (!labels.count(target)) {
                errors.push_back("undefined label in " + function.name + ": " + target);
            }
        };

        for (const auto& instruction : function.instructions) {
            if (instruction.kind == InstructionKind::Jump) {
                check_target(instruction.label);
            } else if (instruction.kind == InstructionKind::Branch) {
                check_target(instruction.label);
                check_target(instruction.false_label);
            } else if (instruction.kind == InstructionKind::Call &&
                       instruction.callee.empty()) {
                errors.push_back("call with empty callee in " + function.name);
            }
        }
    }

    return errors;
}

} // namespace toyc::backend
