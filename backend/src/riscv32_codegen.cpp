#include "toyc/backend/riscv32_codegen.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace toyc::backend {
namespace {

std::int32_t alignTo(std::int32_t value, std::int32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

bool fitsImmediate12(std::int32_t value) {
    return value >= -2048 && value <= 2047;
}

struct SlotKey {
    ValueKind kind;
    std::uint32_t id;

    bool operator==(const SlotKey& other) const {
        return kind == other.kind && id == other.id;
    }
};

struct SlotKeyHash {
    std::size_t operator()(const SlotKey& key) const {
        return (static_cast<std::size_t>(key.kind) << 32U) ^ key.id;
    }
};

using SlotMap = std::unordered_map<SlotKey, std::int32_t, SlotKeyHash>;
using RegisterMap = std::unordered_map<SlotKey, std::string, SlotKeyHash>;

void collectValue(const std::optional<Value>& value, SlotMap& slots,
                  const RegisterMap& registers, std::int32_t& next_offset) {
    if (!value ||
        (value->kind != ValueKind::Local &&
         value->kind != ValueKind::VirtualRegister)) {
        return;
    }
    const SlotKey key{value->kind, value->id};
    if (registers.count(key)) {
        return;
    }
    if (!slots.count(key)) {
        slots.emplace(key, next_offset);
        next_offset -= 4;
    }
}

void collectRegisterCandidate(const std::optional<Value>& value,
                              RegisterMap& registers) {
    static const std::vector<std::string> available{
        "s1", "s2", "s3", "s4", "s5", "s6",
        "s7", "s8", "s9", "s10", "s11",
    };
    if (!value || (value->kind != ValueKind::VirtualRegister &&
                   value->kind != ValueKind::Local)) {
        return;
    }
    const SlotKey key{value->kind, value->id};
    if (!registers.count(key) && registers.size() < available.size()) {
        registers.emplace(key, available[registers.size()]);
    }
}
RegisterMap assignRegisters(const Function& function) {
    RegisterMap registers;
    for (const auto& instruction : function.instructions) {
        collectRegisterCandidate(instruction.destination, registers);
        collectRegisterCandidate(instruction.left, registers);
        collectRegisterCandidate(instruction.right, registers);
        for (const auto& argument : instruction.arguments) {
            collectRegisterCandidate(argument, registers);
        }
    }
    return registers;
}

SlotMap assignSlots(const Function& function, const RegisterMap& registers,
                    std::int32_t& frame_size) {
    SlotMap slots;
    // -4: ra, -8: previous s0, followed by allocated callee-saved registers.
    std::int32_t next_offset =
        -12 - static_cast<std::int32_t>(registers.size() * 4);

    for (const auto& parameter : function.parameters) {
        collectValue(Value::local(parameter.local_id, parameter.name), slots,
                     registers, next_offset);
    }
    for (const auto& instruction : function.instructions) {
        collectValue(instruction.destination, slots, registers, next_offset);
        collectValue(instruction.left, slots, registers, next_offset);
        collectValue(instruction.right, slots, registers, next_offset);
        for (const auto& argument : instruction.arguments) {
            collectValue(argument, slots, registers, next_offset);
        }
    }

    const auto bytes_used = -next_offset - 4;
    frame_size = alignTo(std::max<std::int32_t>(16, bytes_used), 16);
    return slots;
}

std::int32_t slotOffset(const Value& value, const SlotMap& slots) {
    const auto iterator = slots.find(SlotKey{value.kind, value.id});
    if (iterator == slots.end()) {
        throw std::runtime_error("missing stack slot for " + value.toString());
    }
    return iterator->second;
}

std::optional<std::string> assignedRegister(const Value& value,
                                            const RegisterMap& registers) {
    if (value.kind != ValueKind::VirtualRegister &&
        value.kind != ValueKind::Local) {
        return std::nullopt;
    }
    const auto iterator = registers.find(SlotKey{value.kind, value.id});
    if (iterator == registers.end()) {
        return std::nullopt;
    }
    return iterator->second;
}
void emitStackLoad(std::ostringstream& output, const std::string& destination,
                   std::int32_t offset) {
    if (fitsImmediate12(offset)) {
        output << "  lw " << destination << ", " << offset << "(s0)\n";
    } else {
        output << "  li t6, " << offset << "\n"
               << "  add t6, s0, t6\n"
               << "  lw " << destination << ", 0(t6)\n";
    }
}

void emitStackStore(std::ostringstream& output, const std::string& source,
                    std::int32_t offset) {
    if (fitsImmediate12(offset)) {
        output << "  sw " << source << ", " << offset << "(s0)\n";
    } else {
        output << "  li t6, " << offset << "\n"
               << "  add t6, s0, t6\n"
               << "  sw " << source << ", 0(t6)\n";
    }
}

void emitSpAdjustment(std::ostringstream& output, std::int32_t amount) {
    if (amount == 0) {
        return;
    }
    if (fitsImmediate12(amount)) {
        output << "  addi sp, sp, " << amount << "\n";
    } else {
        output << "  li t6, " << amount << "\n"
               << "  add sp, sp, t6\n";
    }
}

void loadValue(std::ostringstream& output, const Value& value,
               const std::string& destination, const SlotMap& slots,
               const RegisterMap& registers) {
    switch (value.kind) {
    case ValueKind::Immediate:
        output << "  li " << destination << ", " << value.immediate << "\n";
        break;
    case ValueKind::VirtualRegister: {
        const auto iterator =
            registers.find(SlotKey{value.kind, value.id});
        if (iterator != registers.end()) {
            if (iterator->second != destination) {
                output << "  mv " << destination << ", " << iterator->second
                       << "\n";
            }
            break;
        }
        emitStackLoad(output, destination, slotOffset(value, slots));
        break;
    }
    case ValueKind::Local: {
        const auto iterator =
            registers.find(SlotKey{value.kind, value.id});
        if (iterator != registers.end()) {
            if (iterator->second != destination) {
                output << "  mv " << destination << ", " << iterator->second
                       << "\n";
            }
            break;
        }
        emitStackLoad(output, destination, slotOffset(value, slots));
        break;
    }    case ValueKind::Global:
        output << "  la t6, " << value.name << "\n"
               << "  lw " << destination << ", 0(t6)\n";
        break;
    }
}

void storeValue(std::ostringstream& output, const Value& destination,
                const std::string& source, const SlotMap& slots,
                const RegisterMap& registers) {
    switch (destination.kind) {
    case ValueKind::VirtualRegister: {
        const auto iterator =
            registers.find(SlotKey{destination.kind, destination.id});
        if (iterator != registers.end()) {
            if (iterator->second != source) {
                output << "  mv " << iterator->second << ", " << source
                       << "\n";
            }
            break;
        }
        emitStackStore(output, source, slotOffset(destination, slots));
        break;
    }
    case ValueKind::Local: {
        const auto iterator =
            registers.find(SlotKey{destination.kind, destination.id});
        if (iterator != registers.end()) {
            if (iterator->second != source) {
                output << "  mv " << iterator->second << ", " << source
                       << "\n";
            }
            break;
        }
        emitStackStore(output, source, slotOffset(destination, slots));
        break;
    }    case ValueKind::Global:
        output << "  la t6, " << destination.name << "\n"
               << "  sw " << source << ", 0(t6)\n";
        break;
    case ValueKind::Immediate:
        throw std::runtime_error("an immediate cannot be an IR destination");
    }
}

bool positivePowerOfTwo(std::int32_t value, int& shift) {
    if (value <= 0) return false;
    const auto unsigned_value = static_cast<std::uint32_t>(value);
    if ((unsigned_value & (unsigned_value - 1U)) != 0U) return false;
    shift = 0;
    while ((1U << shift) != unsigned_value) ++shift;
    return true;
}

void emitBinaryTo(std::ostringstream& output, const std::string& destination,
                  BinaryOp op, const Value& left, const Value& right,
                  const SlotMap& slots, const RegisterMap& registers) {
    const auto load_left = [&]() {
        if (const auto reg = assignedRegister(left, registers)) return *reg;
        loadValue(output, left, "t0", slots, registers);
        return std::string{"t0"};
    };
    const auto load_right = [&]() {
        if (const auto reg = assignedRegister(right, registers)) return *reg;
        loadValue(output, right, "t1", slots, registers);
        return std::string{"t1"};
    };

    if (op == BinaryOp::Add) {
        if (right.kind == ValueKind::Immediate && fitsImmediate12(right.immediate)) {
            const auto source = load_left();
            output << "  addi " << destination << ", " << source
                   << ", " << right.immediate << "\n";
            return;
        }
        if (left.kind == ValueKind::Immediate && fitsImmediate12(left.immediate)) {
            const auto source = load_right();
            output << "  addi " << destination << ", " << source
                   << ", " << left.immediate << "\n";
            return;
        }
    }

    if (op == BinaryOp::Subtract && right.kind == ValueKind::Immediate) {
        const auto negated = static_cast<std::int64_t>(-right.immediate);
        if (negated >= -2048 && negated <= 2047) {
            const auto source = load_left();
            output << "  addi " << destination << ", " << source
                   << ", " << negated << "\n";
            return;
        }
    }

    if (op == BinaryOp::Less && right.kind == ValueKind::Immediate &&
        fitsImmediate12(right.immediate)) {
        const auto source = load_left();
        output << "  slti " << destination << ", " << source
               << ", " << right.immediate << "\n";
        return;
    }

    if ((op == BinaryOp::Equal || op == BinaryOp::NotEqual) &&
        right.kind == ValueKind::Immediate && right.immediate == 0) {
        const auto source = load_left();
        output << "  " << (op == BinaryOp::Equal ? "seqz " : "snez ")
               << destination << ", " << source << "\n";
        return;
    }
    if ((op == BinaryOp::Equal || op == BinaryOp::NotEqual) &&
        left.kind == ValueKind::Immediate && left.immediate == 0) {
        const auto source = load_right();
        output << "  " << (op == BinaryOp::Equal ? "seqz " : "snez ")
               << destination << ", " << source << "\n";
        return;
    }

    int shift = 0;
    if (op == BinaryOp::Multiply && right.kind == ValueKind::Immediate &&
        positivePowerOfTwo(right.immediate, shift)) {
        const auto source = load_left();
        output << "  slli " << destination << ", " << source
               << ", " << shift << "\n";
        return;
    }
    if (op == BinaryOp::Multiply && left.kind == ValueKind::Immediate &&
        positivePowerOfTwo(left.immediate, shift)) {
        const auto source = load_right();
        output << "  slli " << destination << ", " << source
               << ", " << shift << "\n";
        return;
    }

    const auto left_reg = load_left();
    const auto right_reg = load_right();
    switch (op) {
    case BinaryOp::Add:
        output << "  add " << destination << ", " << left_reg << ", " << right_reg << "\n";
        break;
    case BinaryOp::Subtract:
        output << "  sub " << destination << ", " << left_reg << ", " << right_reg << "\n";
        break;
    case BinaryOp::Multiply:
        output << "  mul " << destination << ", " << left_reg << ", " << right_reg << "\n";
        break;
    case BinaryOp::Divide:
        output << "  div " << destination << ", " << left_reg << ", " << right_reg << "\n";
        break;
    case BinaryOp::Remainder:
        output << "  rem " << destination << ", " << left_reg << ", " << right_reg << "\n";
        break;
    case BinaryOp::Equal:
        output << "  xor " << destination << ", " << left_reg << ", " << right_reg << "\n"
               << "  seqz " << destination << ", " << destination << "\n";
        break;
    case BinaryOp::NotEqual:
        output << "  xor " << destination << ", " << left_reg << ", " << right_reg << "\n"
               << "  snez " << destination << ", " << destination << "\n";
        break;
    case BinaryOp::Less:
        output << "  slt " << destination << ", " << left_reg << ", " << right_reg << "\n";
        break;
    case BinaryOp::LessEqual:
        output << "  slt " << destination << ", " << right_reg << ", " << left_reg << "\n"
               << "  xori " << destination << ", " << destination << ", 1\n";
        break;
    case BinaryOp::Greater:
        output << "  slt " << destination << ", " << right_reg << ", " << left_reg << "\n";
        break;
    case BinaryOp::GreaterEqual:
        output << "  slt " << destination << ", " << left_reg << ", " << right_reg << "\n"
               << "  xori " << destination << ", " << destination << ", 1\n";
        break;
    }
}
bool isComparison(BinaryOp op) {
    return op == BinaryOp::Equal || op == BinaryOp::NotEqual ||
           op == BinaryOp::Less || op == BinaryOp::LessEqual ||
           op == BinaryOp::Greater || op == BinaryOp::GreaterEqual;
}

void emitCompareBranch(std::ostringstream& output, BinaryOp op,
                       const Value& left, const Value& right,
                       const std::string& true_label,
                       const std::string& false_label,
                       const SlotMap& slots, const RegisterMap& registers) {
    const auto left_reg = assignedRegister(left, registers);
    const auto right_reg = assignedRegister(right, registers);
    const auto lhs = left_reg ? *left_reg : std::string{"t0"};
    const auto rhs = right_reg ? *right_reg : std::string{"t1"};
    if (!left_reg) {
        loadValue(output, left, "t0", slots, registers);
    }
    if (!right_reg) {
        loadValue(output, right, "t1", slots, registers);
    }
    switch (op) {
    case BinaryOp::Equal:
        output << "  beq " << lhs << ", " << rhs << ", " << true_label << "\n";
        break;
    case BinaryOp::NotEqual:
        output << "  bne " << lhs << ", " << rhs << ", " << true_label << "\n";
        break;
    case BinaryOp::Less:
        output << "  blt " << lhs << ", " << rhs << ", " << true_label << "\n";
        break;
    case BinaryOp::LessEqual:
        output << "  bge " << rhs << ", " << lhs << ", " << true_label << "\n";
        break;
    case BinaryOp::Greater:
        output << "  blt " << rhs << ", " << lhs << ", " << true_label << "\n";
        break;
    case BinaryOp::GreaterEqual:
        output << "  bge " << lhs << ", " << rhs << ", " << true_label << "\n";
        break;
    default:
        break;
    }
    output << "  j " << false_label << "\n";
}
void emitFunction(std::ostringstream& output, const Function& function,
                  const CodegenOptions& options) {
    std::int32_t frame_size = 0;
    const auto registers = assignRegisters(function);
    const auto slots = assignSlots(function, registers, frame_size);
    const auto epilogue = ".L_" + function.name + "_epilogue";
    const auto tail_entry = ".L_" + function.name + "_tail_entry";

    output << "\n  .text\n"
           << "  .align 2\n"
           << "  .globl " << function.name << "\n"
           << "  .type " << function.name << ", @function\n"
           << function.name << ":\n";
    if (options.emit_comments) {
        output << "  # frame: " << frame_size
               << " bytes; s0 points to caller's sp\n";
    }

    output << "  mv t5, sp\n";
    emitSpAdjustment(output, -frame_size);
    output << "  sw ra, -4(t5)\n"
           << "  sw s0, -8(t5)\n";
    for (std::size_t index = 0; index < registers.size(); ++index) {
        output << "  sw s" << index + 1 << ", "
               << -12 - static_cast<std::int32_t>(index * 4) << "(t5)\n";
    }
    output << "  mv s0, t5\n";

    for (std::size_t index = 0; index < function.parameters.size(); ++index) {
        const auto parameter =
            Value::local(function.parameters[index].local_id,
                         function.parameters[index].name);
        if (index < 8) {
            storeValue(output, parameter, "a" + std::to_string(index), slots,
                       registers);
        } else {
            const auto incoming_offset = static_cast<std::int32_t>((index - 8) * 4);
            if (fitsImmediate12(incoming_offset)) {
                output << "  lw t0, " << incoming_offset << "(s0)\n";
            } else {
                output << "  li t6, " << incoming_offset << "\n"
                       << "  add t6, s0, t6\n"
                       << "  lw t0, 0(t6)\n";
            }
            storeValue(output, parameter, "t0", slots, registers);
        }
    }
    output << tail_entry << ":\n";
    for (std::size_t pc = 0; pc < function.instructions.size(); ++pc) {
        const auto& instruction = function.instructions[pc];
        switch (instruction.kind) {
        case InstructionKind::Label:
            output << instruction.label << ":\n";
            break;
        case InstructionKind::Copy: {
            const auto destination_register =
                assignedRegister(*instruction.destination, registers);
            const auto source_register = assignedRegister(*instruction.left,
                                                          registers);
            if (destination_register) {
                if (instruction.left->kind == ValueKind::Immediate) {
                    output << "  li " << *destination_register << ", "
                           << instruction.left->immediate << "\n";
                } else if (source_register) {
                    if (*destination_register != *source_register) {
                        output << "  mv " << *destination_register << ", "
                               << *source_register << "\n";
                    }
                } else if (instruction.left->kind == ValueKind::Global) {
                    output << "  la t6, " << instruction.left->name << "\n"
                           << "  lw " << *destination_register << ", 0(t6)\n";
                } else {
                    emitStackLoad(output, *destination_register,
                                  slotOffset(*instruction.left, slots));
                }
            } else if (source_register) {
                storeValue(output, *instruction.destination, *source_register,
                           slots, registers);
            } else {
                loadValue(output, *instruction.left, "t0", slots, registers);
                storeValue(output, *instruction.destination, "t0", slots,
                           registers);
            }
            break;
        }        case InstructionKind::Unary:
            loadValue(output, *instruction.left, "t0", slots, registers);
            if (*instruction.unary_op == UnaryOp::Negate) {
                output << "  neg t2, t0\n";
            } else {
                output << "  seqz t2, t0\n";
            }
            storeValue(output, *instruction.destination, "t2", slots,
                       registers);
            break;
        case InstructionKind::Binary:
            if (pc + 1 < function.instructions.size()) {
                const auto& next = function.instructions[pc + 1];
                if (next.kind == InstructionKind::Branch && next.left &&
                    instruction.destination &&
                    next.left->kind == instruction.destination->kind &&
                    next.left->id == instruction.destination->id &&
                    isComparison(*instruction.binary_op)) {
                    emitCompareBranch(output, *instruction.binary_op,
                                      *instruction.left, *instruction.right,
                                      next.label, next.false_label,
                                      slots, registers);
                    ++pc;
                    break;
                }
            }
            if (const auto destination_register =
                    assignedRegister(*instruction.destination, registers)) {
                emitBinaryTo(output, *destination_register,
                             *instruction.binary_op,
                             *instruction.left, *instruction.right,
                             slots, registers);
            } else {
                emitBinaryTo(output, "t2", *instruction.binary_op,
                             *instruction.left, *instruction.right,
                             slots, registers);
                storeValue(output, *instruction.destination, "t2", slots,
                           registers);
            }
            break;        case InstructionKind::Jump:
            output << "  j " << instruction.label << "\n";
            break;
        case InstructionKind::Branch: {
            const auto condition_register = assignedRegister(*instruction.left,
                                                             registers);
            const auto condition = condition_register ? *condition_register : "t0";
            if (!condition_register) {
                loadValue(output, *instruction.left, "t0", slots, registers);
            }
            output << "  bnez " << condition << ", " << instruction.label << "\n"
                   << "  j " << instruction.false_label << "\n";
            break;
        }        case InstructionKind::Call: {
            if (instruction.callee == function.name && instruction.destination &&
                instruction.arguments.size() == function.parameters.size() &&
                instruction.arguments.size() <= 4 &&
                pc + 1 < function.instructions.size()) {
                const auto& next = function.instructions[pc + 1];
                if (next.kind == InstructionKind::Return && next.left &&
                    next.left->kind == instruction.destination->kind &&
                    next.left->id == instruction.destination->id) {
                    static const char* scratch[] = {"t0", "t1", "t2", "t3"};
                    for (std::size_t index = 0; index < instruction.arguments.size(); ++index) {
                        loadValue(output, instruction.arguments[index], scratch[index],
                                  slots, registers);
                    }
                    for (std::size_t index = 0; index < instruction.arguments.size(); ++index) {
                        const auto parameter = Value::local(
                            function.parameters[index].local_id,
                            function.parameters[index].name);
                        storeValue(output, parameter, scratch[index], slots, registers);
                    }
                    output << "  j " << tail_entry << "\n";
                    ++pc;
                    break;
                }
            }
            const auto extra_count =
                instruction.arguments.size() > 8
                    ? instruction.arguments.size() - 8
                    : 0;
            const auto outgoing_size =
                alignTo(static_cast<std::int32_t>(extra_count * 4), 16);
            emitSpAdjustment(output, -outgoing_size);

            for (std::size_t index = 0; index < instruction.arguments.size(); ++index) {
                if (index < 8) {
                    const auto argument_register = "a" + std::to_string(index);
                    if (instruction.arguments[index].kind == ValueKind::Immediate) {
                        output << "  li " << argument_register << ", "
                               << instruction.arguments[index].immediate << "\n";
                    } else if (const auto source_register =
                                   assignedRegister(instruction.arguments[index], registers)) {
                        if (*source_register != argument_register) {
                            output << "  mv " << argument_register << ", "
                                   << *source_register << "\n";
                        }
                    } else {
                        loadValue(output, instruction.arguments[index],
                                  argument_register, slots, registers);
                    }
                } else {
                    loadValue(output, instruction.arguments[index], "t0", slots,
                              registers);
                    const auto offset =
                        static_cast<std::int32_t>((index - 8) * 4);
                    if (fitsImmediate12(offset)) {
                        output << "  sw t0, " << offset << "(sp)\n";
                    } else {
                        output << "  li t6, " << offset << "\n"
                               << "  add t6, sp, t6\n"
                               << "  sw t0, 0(t6)\n";
                    }
                }
            }            output << "  call " << instruction.callee << "\n";
            emitSpAdjustment(output, outgoing_size);
            if (instruction.destination) {
                storeValue(output, *instruction.destination, "a0", slots,
                           registers);
            }
            break;
        }
        case InstructionKind::Return:
            if (instruction.left) {
                if (const auto source_register =
                        assignedRegister(*instruction.left, registers)) {
                    if (*source_register != "a0") {
                        output << "  mv a0, " << *source_register << "\n";
                    }
                } else {
                    loadValue(output, *instruction.left, "a0", slots, registers);
                }
            }
            output << "  j " << epilogue << "\n";
            break;
        }
    }

    // A semantic-valid void function may fall through.
    if (!function.returns_value) {
        output << "  j " << epilogue << "\n";
    }
    output << epilogue << ":\n"
           << "  lw ra, -4(s0)\n";
    for (std::size_t index = 0; index < registers.size(); ++index) {
        output << "  lw s" << index + 1 << ", "
               << -12 - static_cast<std::int32_t>(index * 4) << "(s0)\n";
    }
    output << "  lw t6, -8(s0)\n"
           << "  mv sp, s0\n"
           << "  mv s0, t6\n"
           << "  ret\n"
           << "  .size " << function.name << ", .-" << function.name << "\n";
}

} // namespace

RiscV32CodeGenerator::RiscV32CodeGenerator(CodegenOptions options)
    : options_(options) {}

std::string RiscV32CodeGenerator::generate(const Program& program) const {
    const auto errors = validate(program);
    if (!errors.empty()) {
        throw std::runtime_error("invalid IR: " + errors.front());
    }

    std::ostringstream output;
    output << "  .option norvc\n";
    for (const auto& global : program.globals) {
        output << "\n  .section " << (global.is_constant ? ".rodata" : ".data")
               << "\n"
               << "  .align 2\n"
               << "  .globl " << global.name << "\n"
               << "  .type " << global.name << ", @object\n"
               << "  .size " << global.name << ", 4\n"
               << global.name << ":\n"
               << "  .word " << global.initial_value << "\n";
    }

    for (const auto& function : program.functions) {
        emitFunction(output, function, options_);
    }
    output << "\n  .section .note.GNU-stack,\"\",@progbits\n";
    return output.str();
}

} // namespace toyc::backend
