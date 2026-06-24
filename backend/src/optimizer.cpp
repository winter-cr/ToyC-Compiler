#include "toyc/backend/optimizer.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace toyc::backend {
namespace {

struct ValueKey {
    ValueKind kind;
    std::uint32_t id;

    bool operator==(const ValueKey& other) const {
        return kind == other.kind && id == other.id;
    }
};

struct ValueKeyHash {
    std::size_t operator()(const ValueKey& key) const {
        return (static_cast<std::size_t>(key.kind) << 32U) ^ key.id;
    }
};

using UseCounts = std::unordered_map<ValueKey, std::size_t, ValueKeyHash>;

std::optional<std::int32_t> foldUnary(UnaryOp op, std::int32_t value) {
    if (op == UnaryOp::Negate) {
        return static_cast<std::int32_t>(0U -
            static_cast<std::uint32_t>(value));
    }
    return value == 0 ? 1 : 0;
}

std::optional<std::int32_t> foldBinary(BinaryOp op, std::int32_t left,
                                       std::int32_t right) {
    const auto lhs = static_cast<std::uint32_t>(left);
    const auto rhs = static_cast<std::uint32_t>(right);
    switch (op) {
    case BinaryOp::Add:
        return static_cast<std::int32_t>(lhs + rhs);
    case BinaryOp::Subtract:
        return static_cast<std::int32_t>(lhs - rhs);
    case BinaryOp::Multiply:
        return static_cast<std::int32_t>(lhs * rhs);
    case BinaryOp::Divide:
        if (right == 0) {
            return std::nullopt;
        }
        if (left == INT32_MIN && right == -1) {
            return INT32_MIN;
        }
        return left / right;
    case BinaryOp::Remainder:
        if (right == 0) {
            return std::nullopt;
        }
        if (left == INT32_MIN && right == -1) {
            return 0;
        }
        return left % right;
    case BinaryOp::Equal:
        return left == right;
    case BinaryOp::NotEqual:
        return left != right;
    case BinaryOp::Less:
        return left < right;
    case BinaryOp::LessEqual:
        return left <= right;
    case BinaryOp::Greater:
        return left > right;
    case BinaryOp::GreaterEqual:
        return left >= right;
    }
    return std::nullopt;
}

void addUse(const std::optional<Value>& value, UseCounts& uses) {
    if (value && (value->kind == ValueKind::VirtualRegister ||
                  value->kind == ValueKind::Local)) {
        ++uses[ValueKey{value->kind, value->id}];
    }
}

bool isPureTemporaryDefinition(const Instruction& instruction) {
    if (!instruction.destination ||
        instruction.destination->kind != ValueKind::VirtualRegister) {
        return false;
    }
    return instruction.kind == InstructionKind::Copy ||
           instruction.kind == InstructionKind::Unary ||
           instruction.kind == InstructionKind::Binary;
}

void optimizeFunction(Function& function, const OptimizationOptions& options) {
    if (options.fold_constants || options.simplify_branches) {
        for (auto& instruction : function.instructions) {
            if (options.fold_constants &&
                instruction.kind == InstructionKind::Unary &&
                instruction.left &&
                instruction.left->kind == ValueKind::Immediate) {
                const auto value =
                    foldUnary(*instruction.unary_op, instruction.left->immediate);
                instruction =
                    Instruction::copy(*instruction.destination, Value::imm(*value));
            } else if (options.fold_constants &&
                       instruction.kind == InstructionKind::Binary &&
                       instruction.left && instruction.right &&
                       instruction.left->kind == ValueKind::Immediate &&
                       instruction.right->kind == ValueKind::Immediate) {
                const auto value =
                    foldBinary(*instruction.binary_op,
                               instruction.left->immediate,
                               instruction.right->immediate);
                if (value) {
                    instruction = Instruction::copy(
                        *instruction.destination, Value::imm(*value));
                }
            } else if (options.simplify_branches &&
                       instruction.kind == InstructionKind::Branch &&
                       instruction.left &&
                       instruction.left->kind == ValueKind::Immediate) {
                instruction = Instruction::jump(
                    instruction.left->immediate != 0
                        ? instruction.label
                        : instruction.false_label);
            }
        }
    }

    if (!options.eliminate_dead_temporaries) {
        return;
    }

    bool changed = true;
    while (changed) {
        changed = false;
        UseCounts uses;
        for (const auto& instruction : function.instructions) {
            addUse(instruction.left, uses);
            addUse(instruction.right, uses);
            for (const auto& argument : instruction.arguments) {
                addUse(argument, uses);
            }
        }

        std::vector<Instruction> kept;
        kept.reserve(function.instructions.size());
        for (const auto& instruction : function.instructions) {
            if (isPureTemporaryDefinition(instruction) &&
                uses[ValueKey{instruction.destination->kind,
                              instruction.destination->id}] == 0) {
                changed = true;
                continue;
            }
            kept.push_back(instruction);
        }
        function.instructions = std::move(kept);
    }
}

} // namespace

Program optimize(Program program, OptimizationOptions options) {
    for (auto& function : program.functions) {
        optimizeFunction(function, options);
    }
    return program;
}

} // namespace toyc::backend
