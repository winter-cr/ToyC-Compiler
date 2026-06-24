#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace toyc::backend {

enum class ValueKind {
    Immediate,
    VirtualRegister,
    Local,
    Global,
};

struct Value {
    ValueKind kind{ValueKind::Immediate};
    std::int32_t immediate{0};
    std::uint32_t id{0};
    std::string name;

    static Value imm(std::int32_t value);
    static Value vreg(std::uint32_t id);
    static Value local(std::uint32_t id, std::string debug_name = {});
    static Value global(std::string name);

    [[nodiscard]] std::string toString() const;
};

enum class UnaryOp {
    Negate,
    LogicalNot,
};

enum class BinaryOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    Remainder,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
};

enum class InstructionKind {
    Label,
    Copy,
    Unary,
    Binary,
    Jump,
    Branch,
    Call,
    Return,
};

struct Instruction {
    InstructionKind kind{InstructionKind::Label};
    std::optional<Value> destination;
    std::optional<Value> left;
    std::optional<Value> right;
    std::optional<UnaryOp> unary_op;
    std::optional<BinaryOp> binary_op;
    std::string label;
    std::string false_label;
    std::string callee;
    std::vector<Value> arguments;

    static Instruction makeLabel(std::string label);
    static Instruction copy(Value destination, Value source);
    static Instruction unary(Value destination, UnaryOp op, Value operand);
    static Instruction binary(Value destination, BinaryOp op, Value left, Value right);
    static Instruction jump(std::string target);
    static Instruction branch(Value condition, std::string true_target, std::string false_target);
    static Instruction call(std::optional<Value> destination, std::string callee,
                            std::vector<Value> arguments);
    static Instruction ret(std::optional<Value> value = std::nullopt);
};

struct Global {
    std::string name;
    std::int32_t initial_value{0};
    bool is_constant{false};
};

struct Parameter {
    std::uint32_t local_id{0};
    std::string name;
};

struct Function {
    std::string name;
    bool returns_value{true};
    std::vector<Parameter> parameters;
    std::vector<Instruction> instructions;
};

struct Program {
    std::vector<Global> globals;
    std::vector<Function> functions;
};

class FunctionBuilder {
public:
    using ConditionEmitter = std::function<Value(FunctionBuilder&)>;
    using StatementEmitter = std::function<void(FunctionBuilder&)>;

    explicit FunctionBuilder(std::string name, bool returns_value = true);

    Value addParameter(std::string name = {});
    Value createLocal(std::string debug_name = {});
    Value createTemporary();
    std::string createLabel(std::string prefix = "L");
    void emit(Instruction instruction);

    // Structured helpers used by an AST-to-IR lowering pass. The right-hand
    // side of logical operations is emitted only on the path where it is
    // needed, preserving C short-circuit semantics.
    Value emitLogicalAnd(Value left, const ConditionEmitter& emit_right);
    Value emitLogicalOr(Value left, const ConditionEmitter& emit_right);
    void emitIf(Value condition, const StatementEmitter& emit_then,
                const StatementEmitter& emit_else = {});
    void emitWhile(const ConditionEmitter& emit_condition,
                   const StatementEmitter& emit_body);
    void emitBreak();
    void emitContinue();

    [[nodiscard]] Function finish() &&;

private:
    struct LoopTargets {
        std::string continue_target;
        std::string break_target;
    };

    Function function_;
    std::uint32_t next_local_{0};
    std::uint32_t next_temporary_{0};
    std::uint32_t next_label_{0};
    std::vector<LoopTargets> loop_targets_;
};

// Checks structural backend assumptions. Semantic/type validity remains the
// responsibility of the semantic-analysis stage.
[[nodiscard]] std::vector<std::string> validate(const Program& program);

} // namespace toyc::backend
