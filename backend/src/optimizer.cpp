#include "toyc/backend/optimizer.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
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

struct ValueHash {
    std::size_t operator()(const Value& value) const {
        std::size_t seed = static_cast<std::size_t>(value.kind);
        seed = seed * 1315423911U + static_cast<std::size_t>(value.id);
        seed = seed * 1315423911U + static_cast<std::uint32_t>(value.immediate);
        for (char c : value.name) {
            seed = seed * 131U + static_cast<unsigned char>(c);
        }
        return seed;
    }
};

bool operator==(const Value& left, const Value& right) {
    return left.kind == right.kind && left.immediate == right.immediate &&
           left.id == right.id && left.name == right.name;
}

struct ExprKey {
    BinaryOp op;
    Value left;
    Value right;

    bool operator==(const ExprKey& other) const {
        return op == other.op && left == other.left && right == other.right;
    }
};

struct ExprKeyHash {
    std::size_t operator()(const ExprKey& key) const {
        ValueHash value_hash;
        auto seed = static_cast<std::size_t>(key.op);
        seed = seed * 1315423911U + value_hash(key.left);
        seed = seed * 1315423911U + value_hash(key.right);
        return seed;
    }
};

using LiveValues = std::unordered_set<ValueKey, ValueKeyHash>;
using ValueMap = std::unordered_map<ValueKey, Value, ValueKeyHash>;
using ExprMap = std::unordered_map<ExprKey, Value, ExprKeyHash>;

std::optional<ValueKey> storageKey(const Value& value) {
    if (value.kind == ValueKind::VirtualRegister ||
        value.kind == ValueKind::Local) {
        return ValueKey{value.kind, value.id};
    }
    return std::nullopt;
}

bool valueLess(const Value& left, const Value& right) {
    if (left.kind != right.kind) return left.kind < right.kind;
    if (left.immediate != right.immediate) return left.immediate < right.immediate;
    if (left.id != right.id) return left.id < right.id;
    return left.name < right.name;
}

bool isCommutative(BinaryOp op) {
    return op == BinaryOp::Add || op == BinaryOp::Multiply ||
           op == BinaryOp::Equal || op == BinaryOp::NotEqual;
}

bool sameValue(const Value& left, const Value& right) {
    return left == right;
}

Value resolveValue(Value value, const ValueMap& known_values) {
    for (int depth = 0; depth < 8; ++depth) {
        auto key = storageKey(value);
        if (!key) break;
        auto it = known_values.find(*key);
        if (it == known_values.end() || it->second == value) break;
        value = it->second;
    }
    return value;
}

void replaceOperand(std::optional<Value>& operand,
                    const ValueMap& known_values,
                    const std::unordered_map<std::string, std::int32_t>& global_constants) {
    if (!operand) return;
    if (operand->kind == ValueKind::Global) {
        auto it = global_constants.find(operand->name);
        if (it != global_constants.end()) {
            operand = Value::imm(it->second);
            return;
        }
    }
    operand = resolveValue(*operand, known_values);
}

std::optional<std::int32_t> foldUnary(UnaryOp op, std::int32_t value) {
    if (op == UnaryOp::Negate) {
        return static_cast<std::int32_t>(0U - static_cast<std::uint32_t>(value));
    }
    return value == 0 ? 1 : 0;
}

std::optional<std::int32_t> foldBinary(BinaryOp op, std::int32_t left,
                                       std::int32_t right) {
    const auto lhs = static_cast<std::uint32_t>(left);
    const auto rhs = static_cast<std::uint32_t>(right);
    switch (op) {
    case BinaryOp::Add: return static_cast<std::int32_t>(lhs + rhs);
    case BinaryOp::Subtract: return static_cast<std::int32_t>(lhs - rhs);
    case BinaryOp::Multiply: return static_cast<std::int32_t>(lhs * rhs);
    case BinaryOp::Divide:
        if (right == 0) return std::nullopt;
        if (left == INT32_MIN && right == -1) return INT32_MIN;
        return left / right;
    case BinaryOp::Remainder:
        if (right == 0) return std::nullopt;
        if (left == INT32_MIN && right == -1) return 0;
        return left % right;
    case BinaryOp::Equal: return left == right;
    case BinaryOp::NotEqual: return left != right;
    case BinaryOp::Less: return left < right;
    case BinaryOp::LessEqual: return left <= right;
    case BinaryOp::Greater: return left > right;
    case BinaryOp::GreaterEqual: return left >= right;
    }
    return std::nullopt;
}

std::optional<Value> simplifyAlgebra(BinaryOp op, const Value& left,
                                     const Value& right) {
    const bool left_imm = left.kind == ValueKind::Immediate;
    const bool right_imm = right.kind == ValueKind::Immediate;
    if (left_imm && right_imm) {
        if (auto folded = foldBinary(op, left.immediate, right.immediate)) {
            return Value::imm(*folded);
        }
        return std::nullopt;
    }

    switch (op) {
    case BinaryOp::Add:
        if (right_imm && right.immediate == 0) return left;
        if (left_imm && left.immediate == 0) return right;
        break;
    case BinaryOp::Subtract:
        if (right_imm && right.immediate == 0) return left;
        if (sameValue(left, right)) return Value::imm(0);
        break;
    case BinaryOp::Multiply:
        if ((right_imm && right.immediate == 0) ||
            (left_imm && left.immediate == 0)) return Value::imm(0);
        if (right_imm && right.immediate == 1) return left;
        if (left_imm && left.immediate == 1) return right;
        break;
    case BinaryOp::Divide:
        if (right_imm && right.immediate == 1) return left;
        if (left_imm && left.immediate == 0 && !(right_imm && right.immediate == 0)) {
            return Value::imm(0);
        }
        break;
    case BinaryOp::Remainder:
        if (right_imm && right.immediate == 1) return Value::imm(0);
        if (left_imm && left.immediate == 0 && !(right_imm && right.immediate == 0)) {
            return Value::imm(0);
        }
        break;
    case BinaryOp::Equal:
        if (sameValue(left, right)) return Value::imm(1);
        break;
    case BinaryOp::NotEqual:
        if (sameValue(left, right)) return Value::imm(0);
        break;
    case BinaryOp::Less:
    case BinaryOp::Greater:
        if (sameValue(left, right)) return Value::imm(0);
        break;
    case BinaryOp::LessEqual:
    case BinaryOp::GreaterEqual:
        if (sameValue(left, right)) return Value::imm(1);
        break;
    }
    return std::nullopt;
}

void killKnownValue(const Value& destination, ValueMap& known_values,
                    ExprMap& expressions) {
    auto key = storageKey(destination);
    if (!key) return;
    known_values.erase(*key);

    for (auto it = expressions.begin(); it != expressions.end();) {
        const auto uses_destination =
            (storageKey(it->first.left) && *storageKey(it->first.left) == *key) ||
            (storageKey(it->first.right) && *storageKey(it->first.right) == *key) ||
            (storageKey(it->second) && *storageKey(it->second) == *key);
        if (uses_destination) {
            it = expressions.erase(it);
        } else {
            ++it;
        }
    }
}

void clearBlockState(ValueMap& known_values, ExprMap& expressions) {
    known_values.clear();
    expressions.clear();
}

void forwardSimplify(Function& function,
                     const std::unordered_map<std::string, std::int32_t>& global_constants,
                     const OptimizationOptions& options) {
    ValueMap known_values;
    ExprMap expressions;

    for (auto& instruction : function.instructions) {
        if (instruction.kind == InstructionKind::Label) {
            clearBlockState(known_values, expressions);
            continue;
        }

        replaceOperand(instruction.left, known_values, global_constants);
        replaceOperand(instruction.right, known_values, global_constants);
        for (auto& argument : instruction.arguments) {
            if (argument.kind == ValueKind::Global) {
                auto it = global_constants.find(argument.name);
                if (it != global_constants.end()) argument = Value::imm(it->second);
            } else {
                argument = resolveValue(argument, known_values);
            }
        }

        if (instruction.destination) {
            killKnownValue(*instruction.destination, known_values, expressions);
        }

        if (options.fold_constants && instruction.kind == InstructionKind::Unary &&
            instruction.left && instruction.left->kind == ValueKind::Immediate) {
            const auto value = foldUnary(*instruction.unary_op, instruction.left->immediate);
            instruction = Instruction::copy(*instruction.destination, Value::imm(*value));
        } else if (options.fold_constants && instruction.kind == InstructionKind::Binary &&
                   instruction.left && instruction.right) {
            if (auto simplified = simplifyAlgebra(*instruction.binary_op,
                                                  *instruction.left,
                                                  *instruction.right)) {
                instruction = Instruction::copy(*instruction.destination, *simplified);
            } else {
                Value left = *instruction.left;
                Value right = *instruction.right;
                if (isCommutative(*instruction.binary_op) && valueLess(right, left)) {
                    std::swap(left, right);
                }
                const ExprKey key{*instruction.binary_op, left, right};
                auto existing = expressions.find(key);
                if (existing != expressions.end()) {
                    instruction = Instruction::copy(*instruction.destination, existing->second);
                } else {
                    expressions.emplace(key, *instruction.destination);
                    instruction.left = left;
                    instruction.right = right;
                }
            }
        }

        if (instruction.destination) {
            killKnownValue(*instruction.destination, known_values, expressions);
        }

        if (instruction.kind == InstructionKind::Copy && instruction.destination &&
            instruction.left) {
            if (auto key = storageKey(*instruction.destination)) {
                known_values[*key] = *instruction.left;
            }
        } else if (instruction.kind == InstructionKind::Call) {
            expressions.clear();
        }

        if (options.simplify_branches && instruction.kind == InstructionKind::Branch &&
            instruction.left && instruction.left->kind == ValueKind::Immediate) {
            instruction = Instruction::jump(instruction.left->immediate != 0
                                            ? instruction.label
                                            : instruction.false_label);
        }

        if (instruction.kind == InstructionKind::Jump ||
            instruction.kind == InstructionKind::Branch ||
            instruction.kind == InstructionKind::Return) {
            clearBlockState(known_values, expressions);
        }
    }
}

void addLiveUse(const std::optional<Value>& value, LiveValues& live) {
    if (value) {
        if (auto key = storageKey(*value)) live.insert(*key);
    }
}

void addInstructionUses(const Instruction& instruction, LiveValues& live) {
    addLiveUse(instruction.left, live);
    addLiveUse(instruction.right, live);
    for (const auto& argument : instruction.arguments) {
        if (auto key = storageKey(argument)) live.insert(*key);
    }
}

bool isRemovableDefinition(const Instruction& instruction) {
    if (!instruction.destination ||
        instruction.destination->kind != ValueKind::VirtualRegister) {
        return false;
    }
    return instruction.kind == InstructionKind::Copy ||
           instruction.kind == InstructionKind::Unary ||
           instruction.kind == InstructionKind::Binary;
}

void eliminateDeadDefinitions(Function& function) {
    LiveValues live;
    std::vector<Instruction> kept_reversed;
    kept_reversed.reserve(function.instructions.size());

    for (auto it = function.instructions.rbegin(); it != function.instructions.rend(); ++it) {
        const auto& instruction = *it;
        if (isRemovableDefinition(instruction)) {
            const ValueKey destination{instruction.destination->kind,
                                       instruction.destination->id};
            if (!live.count(destination)) {
                continue;
            }
            live.erase(destination);
        } else if (instruction.destination) {
            if (auto key = storageKey(*instruction.destination)) {
                live.erase(*key);
            }
        }
        addInstructionUses(instruction, live);
        kept_reversed.push_back(instruction);
    }

    function.instructions.assign(kept_reversed.rbegin(), kept_reversed.rend());
}

void removeUnreachableInstructions(Function& function) {
    std::vector<Instruction> kept;
    kept.reserve(function.instructions.size());
    bool reachable = true;

    for (auto& instruction : function.instructions) {
        if (!reachable && instruction.kind != InstructionKind::Label) {
            continue;
        }
        if (instruction.kind == InstructionKind::Label) {
            reachable = true;
        }
        kept.push_back(std::move(instruction));
        if (kept.back().kind == InstructionKind::Jump ||
            kept.back().kind == InstructionKind::Return) {
            reachable = false;
        }
    }

    function.instructions = std::move(kept);
}

bool sameStorageValue(const Value& value, const Value& other) {
    auto left = storageKey(value);
    auto right = storageKey(other);
    return left && right && *left == *right;
}

void replaceValueUse(std::optional<Value>& value, const Value& from, const Value& to) {
    if (value && sameStorageValue(*value, from)) {
        value = to;
    }
}

void coalesceTemporaryCopies(Function& function) {
    std::vector<Instruction> result;
    result.reserve(function.instructions.size());

    for (std::size_t i = 0; i < function.instructions.size(); ++i) {
        auto instruction = function.instructions[i];
        if (i + 1 < function.instructions.size() && instruction.destination &&
            instruction.destination->kind == ValueKind::VirtualRegister &&
            (instruction.kind == InstructionKind::Copy ||
             instruction.kind == InstructionKind::Unary ||
             instruction.kind == InstructionKind::Binary)) {
            const auto& next = function.instructions[i + 1];
            if (next.kind == InstructionKind::Copy && next.destination && next.left &&
                next.destination->kind == ValueKind::Local &&
                sameStorageValue(*next.left, *instruction.destination)) {
                const Value temporary = *instruction.destination;
                const Value replacement = *next.destination;
                instruction.destination = replacement;
                result.push_back(std::move(instruction));
                ++i;

                for (std::size_t j = i + 1; j < function.instructions.size(); ++j) {
                    auto& later = function.instructions[j];
                    replaceValueUse(later.left, temporary, replacement);
                    replaceValueUse(later.right, temporary, replacement);
                    for (auto& argument : later.arguments) {
                        if (sameStorageValue(argument, temporary)) {
                            argument = replacement;
                        }
                    }
                    if (later.kind == InstructionKind::Label ||
                        later.kind == InstructionKind::Jump ||
                        later.kind == InstructionKind::Branch ||
                        later.kind == InstructionKind::Return ||
                        (later.destination && sameStorageValue(*later.destination, replacement))) {
                        break;
                    }
                }
                continue;
            }
        }
        result.push_back(std::move(instruction));
    }

    function.instructions = std::move(result);
}
void optimizeFunction(Function& function,
                      const std::unordered_map<std::string, std::int32_t>& global_constants,
                      const OptimizationOptions& options) {
    for (int pass = 0; pass < 3; ++pass) {
        const auto before = function.instructions.size();
        forwardSimplify(function, global_constants, options);
        coalesceTemporaryCopies(function);
        removeUnreachableInstructions(function);
        if (options.eliminate_dead_temporaries) {
            eliminateDeadDefinitions(function);
        }
        if (function.instructions.size() == before) {
            break;
        }
    }
}

} // namespace

Program optimize(Program program, OptimizationOptions options) {
    std::unordered_map<std::string, std::int32_t> global_constants;
    for (const auto& global : program.globals) {
        if (global.is_constant) {
            global_constants.emplace(global.name, global.initial_value);
        }
    }

    for (auto& function : program.functions) {
        optimizeFunction(function, global_constants, options);
    }
    return program;
}

} // namespace toyc::backend