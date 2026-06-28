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

// ---- Jump threading ----
void threadJumps(std::vector<Instruction>& instrs) {
    std::unordered_map<std::string, size_t> label_index;
    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Label) {
            label_index[instrs[i].label] = i;
        }
    }

    // Build jump target to target-of-target map.
    // Only thread when a label is immediately followed by a single jump
    // (i.e., the label is just an alias -- no real code between).
    std::unordered_map<std::string, std::string> jump_redirect;
    for (size_t i = 0; i + 1 < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Label &&
            instrs[i + 1].kind == InstructionKind::Jump) {
            jump_redirect[instrs[i].label] = instrs[i + 1].label;
        }
    }

    // Resolve chains iteratively
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

// ---- Dead jump removal ----
void removeDeadJumps(std::vector<Instruction>& instrs) {
    std::vector<Instruction> result;
    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Jump && i + 1 < instrs.size() &&
            instrs[i + 1].kind == InstructionKind::Label &&
            instrs[i].label == instrs[i + 1].label) {
            continue;
        }
        result.push_back(std::move(instrs[i]));
    }
    instrs = std::move(result);
}

// =====================================================================
// Liveness analysis for copy elimination
// =====================================================================

struct BasicBlock {
    size_t start{0};
    size_t end{0};
    std::vector<size_t> successors;
    std::unordered_set<SlotKey, SlotKeyHash> def;
    std::unordered_set<SlotKey, SlotKeyHash> use;
    std::unordered_set<SlotKey, SlotKeyHash> live_in;
    std::unordered_set<SlotKey, SlotKeyHash> live_out;
};

void buildBlocks(const std::vector<Instruction>& instrs,
                 std::vector<BasicBlock>& blocks) {
    if (instrs.empty()) return;

    std::vector<bool> leader(instrs.size(), false);
    leader[0] = true;

    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].kind == InstructionKind::Label) leader[i] = true;
        if (instrs[i].kind == InstructionKind::Jump ||
            instrs[i].kind == InstructionKind::Branch ||
            instrs[i].kind == InstructionKind::Return) {
            if (i + 1 < instrs.size()) leader[i + 1] = true;
        }
    }

    size_t start = 0;
    for (size_t i = 1; i <= instrs.size(); ++i) {
        if (i == instrs.size() || leader[i]) {
            blocks.push_back({start, i, {}, {}, {}, {}, {}});
            start = i;
        }
    }

    // Map label to block index for successor resolution
    std::unordered_map<std::string, size_t> label_to_block;
    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        for (size_t i = blocks[bi].start; i < blocks[bi].end; ++i) {
            if (instrs[i].kind == InstructionKind::Label) {
                label_to_block[instrs[i].label] = bi;
            }
        }
    }

    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        auto& block = blocks[bi];
        if (block.start >= block.end) continue;

        size_t last_idx = block.end - 1;
        const auto& last = instrs[last_idx];

        if (last.kind == InstructionKind::Jump) {
            auto it = label_to_block.find(last.label);
            if (it != label_to_block.end()) {
                block.successors.push_back(it->second);
            }
        } else if (last.kind == InstructionKind::Branch) {
            // Fall-through (true target is always placed immediately after the branch)
            if (bi + 1 < blocks.size()) block.successors.push_back(bi + 1);
            // False target is the explicit jump target
            auto it = label_to_block.find(last.false_label);
            if (it != label_to_block.end()) {
                block.successors.push_back(it->second);
            }
        } else if (last.kind != InstructionKind::Return) {
            if (bi + 1 < blocks.size()) block.successors.push_back(bi + 1);
        }
    }
}

void computeDefUse(std::vector<BasicBlock>& blocks,
                   const std::vector<Instruction>& instrs) {
    for (auto& block : blocks) {
        std::unordered_set<SlotKey, SlotKeyHash> defined;

        for (size_t i = block.start; i < block.end; ++i) {
            const auto& inst = instrs[i];

            auto record_use = [&](const std::optional<Value>& v) {
                if (!v || (v->kind != ValueKind::Local &&
                           v->kind != ValueKind::VirtualRegister)) return;
                SlotKey key{v->kind, v->id};
                if (!defined.count(key)) block.use.insert(key);
            };

            switch (inst.kind) {
            case InstructionKind::Copy:
            case InstructionKind::Unary:
                record_use(inst.left); break;
            case InstructionKind::Binary:
                record_use(inst.left);
                record_use(inst.right); break;
            case InstructionKind::Branch:
                record_use(inst.left); break;
            case InstructionKind::Call:
                for (const auto& arg : inst.arguments) record_use(arg); break;
            case InstructionKind::Return:
                record_use(inst.left); break;
            default: break;
            }

            if (inst.destination &&
                (inst.destination->kind == ValueKind::Local ||
                 inst.destination->kind == ValueKind::VirtualRegister)) {
                SlotKey key{inst.destination->kind, inst.destination->id};
                defined.insert(key);
                block.def.insert(key);
            }
        }
    }
}

void computeLiveness(std::vector<BasicBlock>& blocks) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int bi = static_cast<int>(blocks.size()) - 1; bi >= 0; --bi) {
            auto& b = blocks[bi];

            // out[B] = union of in[S] for all successors S
            std::unordered_set<SlotKey, SlotKeyHash> new_out;
            for (size_t s : b.successors) {
                for (const auto& key : blocks[s].live_in) {
                    new_out.insert(key);
                }
            }
            if (new_out != b.live_out) {
                b.live_out = std::move(new_out);
                changed = true;
            }

            // in[B] = use[B] U (out[B] - def[B])
            std::unordered_set<SlotKey, SlotKeyHash> new_in = b.live_out;
            for (const auto& key : b.def) new_in.erase(key);
            for (const auto& key : b.use) new_in.insert(key);

            if (new_in != b.live_in) {
                b.live_in = std::move(new_in);
                changed = true;
            }
        }
    }
}

bool valueMatchesSlot(const Value& v, const SlotKey& key) {
    return v.kind == key.kind && v.id == key.id;
}

// ---- Liveness-aware copy elimination ----
void eliminateRedundantCopies(std::vector<Instruction>& instrs) {
    if (instrs.size() < 2) return;

    // 1. Build CFG
    std::vector<BasicBlock> blocks;
    buildBlocks(instrs, blocks);

    // 2. Compute def/use per block
    computeDefUse(blocks, instrs);

    // 3. Backward dataflow for liveness
    computeLiveness(blocks);

    // 4. Per-instruction live_after by backward scan within each block
    std::vector<std::unordered_set<SlotKey, SlotKeyHash>> live_after(instrs.size());
    for (auto& block : blocks) {
        std::unordered_set<SlotKey, SlotKeyHash> live = block.live_out;

        for (size_t idx = block.end; idx > block.start; --idx) {
            size_t i = idx - 1;
            live_after[i] = live;

            const auto& inst = instrs[i];

            // Remove defs from live set
            if (inst.destination &&
                (inst.destination->kind == ValueKind::Local ||
                 inst.destination->kind == ValueKind::VirtualRegister)) {
                live.erase({inst.destination->kind, inst.destination->id});
            }

            // Add uses to live set
            auto add_use = [&](const std::optional<Value>& v) {
                if (v && (v->kind == ValueKind::Local ||
                          v->kind == ValueKind::VirtualRegister)) {
                    live.insert({v->kind, v->id});
                }
            };

            switch (inst.kind) {
            case InstructionKind::Copy: case InstructionKind::Unary:
                add_use(inst.left); break;
            case InstructionKind::Binary:
                add_use(inst.left); add_use(inst.right); break;
            case InstructionKind::Branch:
                add_use(inst.left); break;
            case InstructionKind::Call:
                for (const auto& arg : inst.arguments) add_use(arg); break;
            case InstructionKind::Return:
                add_use(inst.left); break;
            default: break;
            }
        }
    }

    // 5. Build instruction to block mapping
    std::vector<size_t> block_of(instrs.size(), 0);
    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        for (size_t i = blocks[bi].start; i < blocks[bi].end; ++i) {
            block_of[i] = bi;
        }
    }

    // 6. Eliminate dead copies + intra-block copy propagation
    std::vector<bool> to_remove(instrs.size(), false);

    for (size_t i = 0; i < instrs.size(); ++i) {
        if (to_remove[i]) continue;

        auto& inst = instrs[i];
        if (inst.kind != InstructionKind::Copy || !inst.destination || !inst.left) continue;

        if (inst.destination->kind != ValueKind::Local &&
            inst.destination->kind != ValueKind::VirtualRegister) continue;

        const SlotKey dst_key{inst.destination->kind, inst.destination->id};

        // Dead copy: destination not live after this instruction
        if (!live_after[i].count(dst_key)) {
            to_remove[i] = true;
            continue;
        }

        // Intra-block copy propagation
        const Value src_val = *inst.left;
        const bool src_stable =
            (src_val.kind == ValueKind::Immediate || src_val.kind == ValueKind::Global);
        const SlotKey src_key{src_val.kind, src_val.id};
        const size_t my_block = block_of[i];
        const size_t block_end = blocks[my_block].end;

        bool can_eliminate = true;
        bool src_clobbered = false;

        for (size_t j = i + 1; j < block_end; ++j) {
            if (to_remove[j]) continue;
            auto& later = instrs[j];

            // Check if src is clobbered (redefined)
            if (!src_stable && later.destination &&
                later.destination->kind == src_key.kind &&
                later.destination->id == src_key.id) {
                src_clobbered = true;
                can_eliminate = false;
                break;
            }

            // Replace uses of dst with src (must happen before dst-redef check,
            // so a single instruction that both uses and redefines dst gets its
            // use replaced before we stop tracking)
            auto replace = [&](std::optional<Value>& operand) {
                if (operand && valueMatchesSlot(*operand, dst_key)) {
                    operand = src_val;
                }
            };

            switch (later.kind) {
            case InstructionKind::Copy:
            case InstructionKind::Unary:
            case InstructionKind::Branch:
            case InstructionKind::Return:
                replace(later.left); break;
            case InstructionKind::Binary:
                replace(later.left); replace(later.right); break;
            case InstructionKind::Call:
                for (auto& arg : later.arguments) replace(arg); break;
            default: break;
            }

            // If dst itself is redefined, stop tracking
            if (later.destination &&
                later.destination->kind == dst_key.kind &&
                later.destination->id == dst_key.id) {
                break;
            }
        }

        if (can_eliminate && !src_clobbered &&
            !blocks[my_block].live_out.count(dst_key)) {
            to_remove[i] = true;
        }
    }

    // 7. Filter out removed instructions
    bool any_removed = false;
    for (size_t i = 0; i < to_remove.size(); ++i) {
        if (to_remove[i]) { any_removed = true; break; }
    }
    if (any_removed) {
        std::vector<Instruction> result;
        result.reserve(instrs.size());
        for (size_t i = 0; i < instrs.size(); ++i) {
            if (!to_remove[i]) result.push_back(std::move(instrs[i]));
        }
        instrs = std::move(result);
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

std::string removeSelfMoves(const std::string& asm_code) {
    std::istringstream input(asm_code);
    std::ostringstream output;
    std::string line;
    while (std::getline(input, line)) {
        if (line.size() > 6 && line.substr(0, 5) == "  mv ") {
            auto comma = line.find(',', 5);
            if (comma != std::string::npos) {
                std::string src = line.substr(5, comma - 5);
                std::string dst = line.substr(comma + 2);
                src.erase(0, src.find_first_not_of(' '));
                src.erase(src.find_last_not_of(' ') + 1);
                dst.erase(0, dst.find_first_not_of(' '));
                dst.erase(dst.find_last_not_of(' ') + 1);
                if (src == dst) {
                    continue;
                }
            }
        }
        output << line << '\n';
    }
    return output.str();
}

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
            std::string target = lines[i].substr(4);
            target.erase(0, target.find_first_not_of(' '));
            target.erase(target.find_last_not_of(' ') + 1);
            size_t j = i + 1;
            while (j < lines.size() && deleted[j]) ++j;
            if (j < lines.size() && lines[j] == target + ":") {
                deleted[i] = true;
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
    result = removeDeadJumpsAsm(result);
    return result;
}

} // namespace toyc
