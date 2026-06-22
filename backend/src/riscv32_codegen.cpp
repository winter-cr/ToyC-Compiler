#include "toyc/backend/riscv32_codegen.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

void collectValue(const std::optional<Value>& value, SlotMap& slots,
                  std::int32_t& next_offset) {
    if (!value ||
        (value->kind != ValueKind::Local &&
         value->kind != ValueKind::VirtualRegister)) {
        return;
    }
    const SlotKey key{value->kind, value->id};
    if (!slots.count(key)) {
        slots.emplace(key, next_offset);
        next_offset -= 4;
    }
}

SlotMap assignSlots(const Function& function, std::int32_t& frame_size) {
    SlotMap slots;
    std::int32_t next_offset = -12; // -4: ra, -8: previous s0

    for (const auto& parameter : function.parameters) {
        collectValue(Value::local(parameter.local_id, parameter.name), slots,
                     next_offset);
    }
    for (const auto& instruction : function.instructions) {
        collectValue(instruction.destination, slots, next_offset);
        collectValue(instruction.left, slots, next_offset);
        collectValue(instruction.right, slots, next_offset);
        for (const auto& argument : instruction.arguments) {
            collectValue(argument, slots, next_offset);
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
               const std::string& destination, const SlotMap& slots) {
    switch (value.kind) {
    case ValueKind::Immediate:
        output << "  li " << destination << ", " << value.immediate << "\n";
        break;
    case ValueKind::VirtualRegister:
    case ValueKind::Local:
        emitStackLoad(output, destination, slotOffset(value, slots));
        break;
    case ValueKind::Global:
        output << "  la t6, " << value.name << "\n"
               << "  lw " << destination << ", 0(t6)\n";
        break;
    }
}

void storeValue(std::ostringstream& output, const Value& destination,
                const std::string& source, const SlotMap& slots) {
    switch (destination.kind) {
    case ValueKind::VirtualRegister:
    case ValueKind::Local:
        emitStackStore(output, source, slotOffset(destination, slots));
        break;
    case ValueKind::Global:
        output << "  la t6, " << destination.name << "\n"
               << "  sw " << source << ", 0(t6)\n";
        break;
    case ValueKind::Immediate:
        throw std::runtime_error("an immediate cannot be an IR destination");
    }
}

void emitBinary(std::ostringstream& output, BinaryOp op) {
    switch (op) {
    case BinaryOp::Add:
        output << "  add t2, t0, t1\n";
        break;
    case BinaryOp::Subtract:
        output << "  sub t2, t0, t1\n";
        break;
    case BinaryOp::Multiply:
        output << "  mul t2, t0, t1\n";
        break;
    case BinaryOp::Divide:
        output << "  div t2, t0, t1\n";
        break;
    case BinaryOp::Remainder:
        output << "  rem t2, t0, t1\n";
        break;
    case BinaryOp::Equal:
        output << "  xor t2, t0, t1\n"
               << "  seqz t2, t2\n";
        break;
    case BinaryOp::NotEqual:
        output << "  xor t2, t0, t1\n"
               << "  snez t2, t2\n";
        break;
    case BinaryOp::Less:
        output << "  slt t2, t0, t1\n";
        break;
    case BinaryOp::LessEqual:
        output << "  slt t2, t1, t0\n"
               << "  xori t2, t2, 1\n";
        break;
    case BinaryOp::Greater:
        output << "  slt t2, t1, t0\n";
        break;
    case BinaryOp::GreaterEqual:
        output << "  slt t2, t0, t1\n"
               << "  xori t2, t2, 1\n";
        break;
    }
}

void emitFunction(std::ostringstream& output, const Function& function,
                  const CodegenOptions& options) {
    std::int32_t frame_size = 0;
    const auto slots = assignSlots(function, frame_size);
    const auto epilogue = ".L_" + function.name + "_epilogue";

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
           << "  sw s0, -8(t5)\n"
           << "  mv s0, t5\n";

    for (std::size_t index = 0; index < function.parameters.size(); ++index) {
        const auto parameter =
            Value::local(function.parameters[index].local_id,
                         function.parameters[index].name);
        if (index < 8) {
            emitStackStore(output, "a" + std::to_string(index),
                           slotOffset(parameter, slots));
        } else {
            const auto incoming_offset = static_cast<std::int32_t>((index - 8) * 4);
            if (fitsImmediate12(incoming_offset)) {
                output << "  lw t0, " << incoming_offset << "(s0)\n";
            } else {
                output << "  li t6, " << incoming_offset << "\n"
                       << "  add t6, s0, t6\n"
                       << "  lw t0, 0(t6)\n";
            }
            emitStackStore(output, "t0", slotOffset(parameter, slots));
        }
    }

    for (const auto& instruction : function.instructions) {
        switch (instruction.kind) {
        case InstructionKind::Label:
            output << instruction.label << ":\n";
            break;
        case InstructionKind::Copy:
            loadValue(output, *instruction.left, "t0", slots);
            storeValue(output, *instruction.destination, "t0", slots);
            break;
        case InstructionKind::Unary:
            loadValue(output, *instruction.left, "t0", slots);
            if (*instruction.unary_op == UnaryOp::Negate) {
                output << "  neg t2, t0\n";
            } else {
                output << "  seqz t2, t0\n";
            }
            storeValue(output, *instruction.destination, "t2", slots);
            break;
        case InstructionKind::Binary:
            loadValue(output, *instruction.left, "t0", slots);
            loadValue(output, *instruction.right, "t1", slots);
            emitBinary(output, *instruction.binary_op);
            storeValue(output, *instruction.destination, "t2", slots);
            break;
        case InstructionKind::Jump:
            output << "  j " << instruction.label << "\n";
            break;
        case InstructionKind::Branch:
            loadValue(output, *instruction.left, "t0", slots);
            output << "  bnez t0, " << instruction.label << "\n"
                   << "  j " << instruction.false_label << "\n";
            break;
        case InstructionKind::Call: {
            const auto extra_count =
                instruction.arguments.size() > 8
                    ? instruction.arguments.size() - 8
                    : 0;
            const auto outgoing_size =
                alignTo(static_cast<std::int32_t>(extra_count * 4), 16);
            emitSpAdjustment(output, -outgoing_size);

            for (std::size_t index = 0; index < instruction.arguments.size(); ++index) {
                loadValue(output, instruction.arguments[index], "t0", slots);
                if (index < 8) {
                    output << "  mv a" << index << ", t0\n";
                } else {
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
            }
            output << "  call " << instruction.callee << "\n";
            emitSpAdjustment(output, outgoing_size);
            if (instruction.destination) {
                storeValue(output, *instruction.destination, "a0", slots);
            }
            break;
        }
        case InstructionKind::Return:
            if (instruction.left) {
                loadValue(output, *instruction.left, "a0", slots);
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
           << "  lw ra, -4(s0)\n"
           << "  lw t6, -8(s0)\n"
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
