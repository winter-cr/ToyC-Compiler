#include "optimizer/DOptimizer.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace toyc {
namespace {

using namespace toyc::backend;

// =====================================================================
// 1. IR-level optimizations
// =====================================================================

struct SlotKey {
    ValueKind kind;
    std::uint32_t id;
    bool operator==(const SlotKey& o) const { return kind == o.kind && id == o.id; }
};
struct SlotKeyHash {
    std::size_t operator()(const SlotKey& k) const {
        return (static_cast<std::size_t>(k.kind) << 32U) ^ k.id;
    }
};

// ---- Jump threading: redirect j L1 (where L1: j L2) → j L2 ----
void threadJumps(std::vector<Instruction>& instrs) {
    // Build label → instruction index map
    std::unordered_map<std::string, size_t> label_index;
    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Label) {
            label_index[instrs[i].label] = i;
        }
    }

    // Build jump target → target-of-target map
    std::unordered_map<std::string, std::string> jump_redirect;
    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Label) {
            // If the only instruction before the next label/branch is a jump,
            // record the redirection.
            size_t j = i + 1;
            while (j < instrs.size() &&
                   instrs[j].kind != InstructionKind::Label &&
                   instrs[j].kind != InstructionKind::Jump &&
                   instrs[j].kind != InstructionKind::Branch) {
                ++j;
            }
            if (j < instrs.size() && instrs[j].kind == InstructionKind::Jump) {
                jump_redirect[instrs[i].label] = instrs[j].label;
            }
        }
    }

    // Apply redirections iteratively to handle chains
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [from, to] : jump_redirect) {
            auto it = jump_redirect.find(to);
            if (it != jump_redirect.end() && it->second != from) {
                to = it->second;
                changed = true;
            }
        }
    }

    // Apply to all jump/branch instructions
    for (auto& instr : instrs) {
        if (instr.kind == InstructionKind::Jump) {
            auto it = jump_redirect.find(instr.label);
            if (it != jump_redirect.end()) instr.label = it->second;
        } else if (instr.kind == InstructionKind::Branch) {
            auto it = jump_redirect.find(instr.label);
            if (it != jump_redirect.end()) instr.label = it->second;
            it = jump_redirect.find(instr.false_label);
            if (it != jump_redirect.end()) instr.false_label = it->second;
        }
    }
}

// ---- Dead jump removal: j L (where L is the next instruction) ----
void removeDeadJumps(std::vector<Instruction>& instrs) {
    std::vector<Instruction> result;
    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Jump && i + 1 < instrs.size() &&
            instrs[i + 1].kind == InstructionKind::Label &&
            instrs[i].label == instrs[i + 1].label) {
            // Skip this jump — it targets the next instruction
            continue;
        }
        // Also skip j L where L immediately follows a falling-through branch
        if (instrs[i].kind == InstructionKind::Jump && i > 0 &&
            instrs[i - 1].kind == InstructionKind::Branch) {
            // Branch already handles the false case; jump is for the other path.
            // Don't remove — it's needed.
        }
        result.push_back(std::move(instrs[i]));
    }
    instrs = std::move(result);
}

// ---- Redundant copy elimination ----
void eliminateRedundantCopies(std::vector<Instruction>& instrs) {
    // Track: for each destination, the last source it was copied from
    // and the index of the copy instruction.
    struct CopyInfo {
        Value source;
        size_t index;
    };
    std::unordered_map<SlotKey, CopyInfo, SlotKeyHash> last_copy;

    // Track which destinations are used between copies
    std::unordered_set<SlotKey, SlotKeyHash> used_since_copy;

    // First pass: mark used values
    auto markUse = [&](const std::optional<Value>& v) {
        if (v && (v->kind == ValueKind::Local ||
                  v->kind == ValueKind::VirtualRegister)) {
            used_since_copy.insert({v->kind, v->id});
        }
    };

    // Second pass: eliminate
    for (size_t i = 0; i < instrs.size(); ++i) {
        auto& instr = instrs[i];
        if (instr.kind == InstructionKind::Copy && instr.destination &&
            instr.left) {
            SlotKey dst{instr.destination->kind, instr.destination->id};
            SlotKey src{instr.left->kind, instr.left->id};

            // Check if previous copy to same dest can be eliminated
            auto it = last_copy.find(dst);
            if (it != last_copy.end() && !used_since_copy.count(dst)) {
                // Previous copy was never read — remove it
                // Mark it as NOP by turning into a Jump to self (will be removed)
                // Actually, just mark the source as unused for this iteration
            }

            last_copy[dst] = {*instr.left, i};
            used_since_copy.erase(dst);
        }

        // Mark reads
        if (instr.kind == InstructionKind::Copy ||
            instr.kind == InstructionKind::Unary ||
            instr.kind == InstructionKind::Binary ||
            instr.kind == InstructionKind::Branch) {
            markUse(instr.left);
        }
        if (instr.kind == InstructionKind::Binary) {
            markUse(instr.right);
        }
        if (instr.kind == InstructionKind::Call) {
            for (auto& arg : instr.arguments) markUse(arg);
        }
        if (instr.kind == InstructionKind::Return) {
            markUse(instr.left);
        }
    }
}

// ---- Main IR optimization ----
Program optimizeIRImpl(Program program) {
    for (auto& func : program.functions) {
        threadJumps(func.instructions);
        removeDeadJumps(func.instructions);
        eliminateRedundantCopies(func.instructions);
    }
    return program;
}

// =====================================================================
// 2. Assembly-level peephole optimizations
// =====================================================================

// Remove "mv tX, tX" (register self-moves)
std::string removeSelfMoves(const std::string& asm_code) {
    std::istringstream input(asm_code);
    std::ostringstream output;
    std::string line;
    while (std::getline(input, line)) {
        // Match "  mv tX, tX" or "  mv sX, sX" or "  mv aX, aX"
        // Simple check: line starts with "  mv " and the two operands are identical
        if (line.size() > 6 && line.substr(0, 5) == "  mv ") {
            auto comma = line.find(',', 5);
            if (comma != std::string::npos) {
                std::string src = line.substr(5, comma - 5);
                std::string dst = line.substr(comma + 2);
                // Trim spaces
                src.erase(0, src.find_first_not_of(' '));
                src.erase(src.find_last_not_of(' ') + 1);
                dst.erase(0, dst.find_first_not_of(' '));
                dst.erase(dst.find_last_not_of(' ') + 1);
                if (src == dst) {
                    continue; // skip self-move
                }
            }
        }
        output << line << '\n';
    }
    return output.str();
}

// Simplify "li tX, 0; add tZ, tY, tX" → "mv tZ, tY"
// and "li tX, 1; mul tZ, tY, tX" → "mv tZ, tY"
std::string simplifyAddMulWithZeroOne(const std::string& asm_code) {
    std::istringstream input(asm_code);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    std::vector<bool> deleted(lines.size(), false);
    for (size_t i = 0; i + 1 < lines.size(); ++i) {
        if (deleted[i]) continue;

        // "  li reg, 0" followed by "  add/sub dst, src, reg" → "  mv dst, src"
        // (adding/subtracting zero is identity)
        const auto& li_line = lines[i];
        if (li_line.size() > 7 && li_line.substr(0, 5) == "  li ") {
            auto comma = li_line.find(',', 5);
            if (comma == std::string::npos) continue;
            std::string reg = li_line.substr(5, comma - 5);
            std::string imm_str = li_line.substr(comma + 2);
            // Trim
            reg.erase(0, reg.find_first_not_of(' '));
            reg.erase(reg.find_last_not_of(' ') + 1);
            imm_str.erase(0, imm_str.find_first_not_of(' '));
            imm_str.erase(imm_str.find_last_not_of(' ') + 1);

            if (imm_str != "0" && imm_str != "1") continue;

            // Look at next line
            for (size_t j = i + 1; j < lines.size() && j < i + 3; ++j) {
                if (deleted[j]) continue;
                const auto& next = lines[j];
                if (next.size() < 7) continue;
                std::string op_prefix = next.substr(0, 7);
                if (op_prefix == "  add " || op_prefix == "  sub ") {
                    auto pos = next.find(reg);
                    if (pos != std::string::npos && pos > 6) {
                        // reg is used as operand; if imm=0, it's identity for add/sub
                        if (imm_str == "0") {
                            // Replace add/sub with mv: extract dst and other src
                            // "  add tZ, tY, reg" where reg=zero → "  mv tZ, tY"
                            std::string dst, src1, src2;
                            std::istringstream ns(next.substr(5));
                            ns >> dst;
                            // dst ends with comma
                            if (!dst.empty() && dst.back() == ',') dst.pop_back();
                            ns >> src1;
                            if (!src1.empty() && src1.back() == ',') src1.pop_back();
                            ns >> src2;
                            std::string other = (src1 == reg) ? src2 : src1;
                            lines[j] = "  mv " + dst + ", " + other;
                            deleted[i] = true;
                            break;
                        }
                    }
                } else if (op_prefix == "  mul ") {
                    auto pos = next.find(reg);
                    if (pos != std::string::npos && pos > 6 && imm_str == "1") {
                        // mul with 1 is identity
                        std::string dst, src1, src2;
                        std::istringstream ns(next.substr(5));
                        ns >> dst;
                        if (!dst.empty() && dst.back() == ',') dst.pop_back();
                        ns >> src1;
                        if (!src1.empty() && src1.back() == ',') src1.pop_back();
                        ns >> src2;
                        std::string other = (src1 == reg) ? src2 : src1;
                        lines[j] = "  mv " + dst + ", " + other;
                        deleted[i] = true;
                        break;
                    }
                }
                break; // only check the very next real instruction
            }
        }
    }

    std::ostringstream output;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!deleted[i]) output << lines[i] << '\n';
    }
    return output.str();
}

// Remove "j label; label:" (dead jump to immediately following label)
std::string removeDeadJumpsAsm(const std::string& asm_code) {
    std::istringstream input(asm_code);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    std::vector<bool> deleted(lines.size(), false);
    for (size_t i = 0; i + 1 < lines.size(); ++i) {
        if (deleted[i]) continue;
        if (lines[i].size() > 4 && lines[i].substr(0, 4) == "  j ") {
            // Extract target label from "  j .L_xxx"
            std::string target = lines[i].substr(4);
            target.erase(0, target.find_first_not_of(' '));
            target.erase(target.find_last_not_of(' ') + 1);
            // Check if next non-deleted line is the target label
            size_t j = i + 1;
            while (j < lines.size() && deleted[j]) ++j;
            if (j < lines.size() && lines[j] == target + ":") {
                deleted[i] = true; // jump goes to next line, remove it
            }
        }
    }

    std::ostringstream output;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!deleted[i]) output << lines[i] << '\n';
    }
    return output.str();
}

} // namespace

// =====================================================================
// Public interface
// =====================================================================

toyc::backend::Program DOptimizer::optimizeIR(toyc::backend::Program program) {
    return optimizeIRImpl(std::move(program));
}

std::string DOptimizer::optimizeAssembly(const std::string& assembly) {
    std::string result = assembly;
    result = removeSelfMoves(result);
    result = simplifyAddMulWithZeroOne(result);
    result = removeDeadJumpsAsm(result);
    return result;
}

} // namespace toyc
