#include "toyc/backend/text_ir.hpp"

#include <charconv>
#include <cstdint>
#include <istream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace toyc::backend {
namespace {

std::vector<std::string> split(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        if (!token.empty() && token.front() == '#') {
            break;
        }
        tokens.push_back(std::move(token));
    }
    return tokens;
}

std::optional<std::int32_t> parseInteger(std::string_view text) {
    std::int32_t value = 0;
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} ||
        result.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::uint32_t> parseId(std::string_view text,
                                     std::string_view prefix) {
    if (text.size() < prefix.size() || text.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    const auto digits = text.substr(prefix.size());
    const auto result =
        std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (digits.empty() || result.ec != std::errc{} ||
        result.ptr != digits.data() + digits.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<Value> parseValue(const std::string& text) {
    if (const auto integer = parseInteger(text)) {
        return Value::imm(*integer);
    }
    if (const auto id = parseId(text, "%t")) {
        return Value::vreg(*id);
    }
    if (const auto id = parseId(text, "%l")) {
        return Value::local(*id);
    }
    if (text.size() > 1 && text.front() == '@') {
        return Value::global(text.substr(1));
    }
    return std::nullopt;
}

std::optional<UnaryOp> parseUnaryOp(const std::string& text) {
    if (text == "neg") {
        return UnaryOp::Negate;
    }
    if (text == "not") {
        return UnaryOp::LogicalNot;
    }
    return std::nullopt;
}

std::optional<BinaryOp> parseBinaryOp(const std::string& text) {
    static const std::unordered_map<std::string, BinaryOp> operations{
        {"add", BinaryOp::Add},
        {"sub", BinaryOp::Subtract},
        {"mul", BinaryOp::Multiply},
        {"div", BinaryOp::Divide},
        {"rem", BinaryOp::Remainder},
        {"eq", BinaryOp::Equal},
        {"ne", BinaryOp::NotEqual},
        {"lt", BinaryOp::Less},
        {"le", BinaryOp::LessEqual},
        {"gt", BinaryOp::Greater},
        {"ge", BinaryOp::GreaterEqual},
    };
    const auto iterator = operations.find(text);
    return iterator == operations.end()
               ? std::nullopt
               : std::optional<BinaryOp>(iterator->second);
}

} // namespace

ParseResult parseTextIr(std::istream& input) {
    ParseResult result;
    Function* current = nullptr;
    std::string line;
    std::size_t line_number = 0;

    const auto error = [&](const std::string& message) {
        result.errors.push_back("line " + std::to_string(line_number) +
                                ": " + message);
    };

    while (std::getline(input, line)) {
        ++line_number;
        const auto tokens = split(line);
        if (tokens.empty()) {
            continue;
        }

        const auto& command = tokens[0];
        if (command == "global") {
            if (current) {
                error("global declaration inside a function");
                continue;
            }
            if (tokens.size() < 3 || tokens.size() > 4 ||
                tokens[1].size() < 2 || tokens[1].front() != '@') {
                error("expected: global @name value [const]");
                continue;
            }
            const auto value = parseInteger(tokens[2]);
            if (!value || (tokens.size() == 4 && tokens[3] != "const")) {
                error("invalid global declaration");
                continue;
            }
            result.program.globals.push_back(
                Global{tokens[1].substr(1), *value, tokens.size() == 4});
            continue;
        }

        if (command == "function") {
            if (current || tokens.size() < 2 || tokens.size() > 3 ||
                (tokens.size() == 3 && tokens[2] != "void")) {
                error("expected: function name [void]");
                continue;
            }
            result.program.functions.push_back(Function{});
            current = &result.program.functions.back();
            current->name = tokens[1];
            current->returns_value = tokens.size() != 3;
            continue;
        }

        if (command == "end") {
            if (!current || tokens.size() != 1) {
                error("unexpected end");
            } else {
                current = nullptr;
            }
            continue;
        }

        if (!current) {
            error("instruction outside a function");
            continue;
        }

        if (command == "param") {
            if (tokens.size() < 2 || tokens.size() > 3) {
                error("expected: param %lN [name]");
                continue;
            }
            const auto id = parseId(tokens[1], "%l");
            if (!id) {
                error("parameter must use a local value such as %l0");
                continue;
            }
            current->parameters.push_back(
                Parameter{*id, tokens.size() == 3 ? tokens[2] : ""});
        } else if (command == "label") {
            if (tokens.size() != 2) {
                error("expected: label name");
            } else {
                current->instructions.push_back(
                    Instruction::makeLabel(tokens[1]));
            }
        } else if (command == "copy") {
            if (tokens.size() != 3) {
                error("expected: copy destination source");
                continue;
            }
            const auto destination = parseValue(tokens[1]);
            const auto source = parseValue(tokens[2]);
            if (!destination || !source) {
                error("invalid copy operand");
            } else {
                current->instructions.push_back(
                    Instruction::copy(*destination, *source));
            }
        } else if (command == "unary") {
            if (tokens.size() != 4) {
                error("expected: unary destination op operand");
                continue;
            }
            const auto destination = parseValue(tokens[1]);
            const auto operation = parseUnaryOp(tokens[2]);
            const auto operand = parseValue(tokens[3]);
            if (!destination || !operation || !operand) {
                error("invalid unary instruction");
            } else {
                current->instructions.push_back(
                    Instruction::unary(*destination, *operation, *operand));
            }
        } else if (command == "binary") {
            if (tokens.size() != 5) {
                error("expected: binary destination op left right");
                continue;
            }
            const auto destination = parseValue(tokens[1]);
            const auto operation = parseBinaryOp(tokens[2]);
            const auto left = parseValue(tokens[3]);
            const auto right = parseValue(tokens[4]);
            if (!destination || !operation || !left || !right) {
                error("invalid binary instruction");
            } else {
                current->instructions.push_back(Instruction::binary(
                    *destination, *operation, *left, *right));
            }
        } else if (command == "jump") {
            if (tokens.size() != 2) {
                error("expected: jump label");
            } else {
                current->instructions.push_back(
                    Instruction::jump(tokens[1]));
            }
        } else if (command == "branch") {
            if (tokens.size() != 4) {
                error("expected: branch condition true-label false-label");
                continue;
            }
            const auto condition = parseValue(tokens[1]);
            if (!condition) {
                error("invalid branch condition");
            } else {
                current->instructions.push_back(Instruction::branch(
                    *condition, tokens[2], tokens[3]));
            }
        } else if (command == "call") {
            if (tokens.size() < 3) {
                error("expected: call destination|- callee [arguments...]");
                continue;
            }
            std::optional<Value> destination;
            if (tokens[1] != "-") {
                destination = parseValue(tokens[1]);
                if (!destination) {
                    error("invalid call destination");
                    continue;
                }
            }
            std::vector<Value> arguments;
            bool valid = true;
            for (std::size_t index = 3; index < tokens.size(); ++index) {
                const auto argument = parseValue(tokens[index]);
                if (!argument) {
                    valid = false;
                    break;
                }
                arguments.push_back(*argument);
            }
            if (!valid) {
                error("invalid call argument");
            } else {
                current->instructions.push_back(Instruction::call(
                    destination, tokens[2], std::move(arguments)));
            }
        } else if (command == "return") {
            if (tokens.size() > 2) {
                error("expected: return [value]");
                continue;
            }
            if (tokens.size() == 1) {
                current->instructions.push_back(Instruction::ret());
            } else if (const auto value = parseValue(tokens[1])) {
                current->instructions.push_back(Instruction::ret(*value));
            } else {
                error("invalid return value");
            }
        } else {
            error("unknown command: " + command);
        }
    }

    if (current) {
        result.errors.push_back("unexpected end of input inside function " +
                                current->name);
    }
    const auto validation_errors = validate(result.program);
    result.errors.insert(result.errors.end(), validation_errors.begin(),
                         validation_errors.end());
    return result;
}

} // namespace toyc::backend
