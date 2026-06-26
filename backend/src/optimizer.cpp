#include "toyc/backend/optimizer.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

using LiveValues = std::unordered_set<ValueKey, ValueKeyHash>;

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

void addLiveUse(const std::optional<Value>& value, LiveValues& live) {
    if (value && (value->kind == ValueKind::VirtualRegister ||
                  value->kind == ValueKind::Local)) {
        live.insert(ValueKey{value->kind, value->id});
    }
}

void addInstructionUses(const Instruction& instruction, LiveValues& live) {
    addLiveUse(instruction.left, live);
    addLiveUse(instruction.right, live);
    for (const auto& argument : instruction.arguments) {
        addLiveUse(argument, live);
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

    LiveValues live;
    std::vector<Instruction> kept_reversed;
    kept_reversed.reserve(function.instructions.size());

    for (auto it = function.instructions.rbegin();
         it != function.instructions.rend(); ++it) {
        const auto& instruction = *it;
        if (isPureTemporaryDefinition(instruction)) {
            const ValueKey destination{instruction.destination->kind,
                                       instruction.destination->id};
            if (!live.count(destination)) {
                continue;
            }
            live.erase(destination);
        }
        addInstructionUses(instruction, live);
        kept_reversed.push_back(instruction);
    }

    function.instructions.assign(kept_reversed.rbegin(), kept_reversed.rend());
}

} // namespace

Program optimize(Program program, OptimizationOptions options) {
    for (auto& function : program.functions) {
        optimizeFunction(function, options);
    }
    return program;
}

} // namespace toyc::backend
