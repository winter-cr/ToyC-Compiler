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

Value FunctionBuilder::emitLogicalAnd(
    Value left, const ConditionEmitter& emit_right) {
    const auto result = createTemporary();
    const auto evaluate_right = createLabel("and_rhs");
    const auto true_label = createLabel("and_true");
    const auto false_label = createLabel("and_false");
    const auto end_label = createLabel("and_end");

    emit(Instruction::branch(std::move(left), evaluate_right, false_label));
    emit(Instruction::makeLabel(evaluate_right));
    emit(Instruction::branch(emit_right(*this), true_label, false_label));
    emit(Instruction::makeLabel(true_label));
    emit(Instruction::copy(result, Value::imm(1)));
    emit(Instruction::jump(end_label));
    emit(Instruction::makeLabel(false_label));
    emit(Instruction::copy(result, Value::imm(0)));
    emit(Instruction::makeLabel(end_label));
    return result;
}

Value FunctionBuilder::emitLogicalOr(
    Value left, const ConditionEmitter& emit_right) {
    const auto result = createTemporary();
    const auto evaluate_right = createLabel("or_rhs");
    const auto true_label = createLabel("or_true");
    const auto false_label = createLabel("or_false");
    const auto end_label = createLabel("or_end");

    emit(Instruction::branch(std::move(left), true_label, evaluate_right));
    emit(Instruction::makeLabel(evaluate_right));
    emit(Instruction::branch(emit_right(*this), true_label, false_label));
    emit(Instruction::makeLabel(true_label));
    emit(Instruction::copy(result, Value::imm(1)));
    emit(Instruction::jump(end_label));
    emit(Instruction::makeLabel(false_label));
    emit(Instruction::copy(result, Value::imm(0)));
    emit(Instruction::makeLabel(end_label));
    return result;
}

void FunctionBuilder::emitIf(Value condition,
                             const StatementEmitter& emit_then,
                             const StatementEmitter& emit_else) {
    const auto then_label = createLabel("if_then");
    const auto else_label = createLabel("if_else");
    const auto end_label = createLabel("if_end");

    emit(Instruction::branch(std::move(condition), then_label,
                             emit_else ? else_label : end_label));
    emit(Instruction::makeLabel(then_label));
    emit_then(*this);
    emit(Instruction::jump(end_label));
    if (emit_else) {
        emit(Instruction::makeLabel(else_label));
        emit_else(*this);
        emit(Instruction::jump(end_label));
    }
    emit(Instruction::makeLabel(end_label));
}

void FunctionBuilder::emitWhile(const ConditionEmitter& emit_condition,
                                const StatementEmitter& emit_body) {
    const auto condition_label = createLabel("while_condition");
    const auto body_label = createLabel("while_body");
    const auto end_label = createLabel("while_end");

    emit(Instruction::jump(condition_label));
    emit(Instruction::makeLabel(condition_label));
    emit(Instruction::branch(emit_condition(*this), body_label, end_label));
    emit(Instruction::makeLabel(body_label));
    loop_targets_.push_back(LoopTargets{condition_label, end_label});
    emit_body(*this);
    loop_targets_.pop_back();
    emit(Instruction::jump(condition_label));
    emit(Instruction::makeLabel(end_label));
}

void FunctionBuilder::emitBreak() {
    if (loop_targets_.empty()) {
        throw std::logic_error("break emitted outside of a loop");
    }
    emit(Instruction::jump(loop_targets_.back().break_target));
}

void FunctionBuilder::emitContinue() {
    if (loop_targets_.empty()) {
        throw std::logic_error("continue emitted outside of a loop");
    }
    emit(Instruction::jump(loop_targets_.back().continue_target));
}

Function FunctionBuilder::finish() && {
    if (!loop_targets_.empty()) {
        throw std::logic_error("cannot finish a function while lowering a loop");
    }
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
            const auto require_destination =
                instruction.kind == InstructionKind::Copy ||
                instruction.kind == InstructionKind::Unary ||
                instruction.kind == InstructionKind::Binary;
            if (require_destination && !instruction.destination) {
                errors.push_back("missing destination in " + function.name);
            }
            if ((instruction.kind == InstructionKind::Copy ||
                 instruction.kind == InstructionKind::Unary ||
                 instruction.kind == InstructionKind::Branch) &&
                !instruction.left) {
                errors.push_back("missing operand in " + function.name);
            }
            if (instruction.kind == InstructionKind::Binary &&
                (!instruction.left || !instruction.right ||
                 !instruction.binary_op)) {
                errors.push_back("incomplete binary instruction in " +
                                 function.name);
            }
            if (instruction.kind == InstructionKind::Unary &&
                !instruction.unary_op) {
                errors.push_back("incomplete unary instruction in " +
                                 function.name);
            }
            if (instruction.destination &&
                instruction.destination->kind == ValueKind::Immediate) {
                errors.push_back("immediate destination in " + function.name);
            }

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
