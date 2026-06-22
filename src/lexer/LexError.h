#pragma once

#include <stdexcept>
#include <string>

struct LexLocation {
    int line = 1;
    int column = 1;
};

class LexError : public std::runtime_error {
public:
    LexError(const std::string& message, LexLocation loc)
        : std::runtime_error(format_message(message, loc)), loc_(loc) {}

    const LexLocation& location() const { return loc_; }

private:
    static std::string format_message(const std::string& message, LexLocation loc) {
        return message + " at line " + std::to_string(loc.line) + ", column " +
               std::to_string(loc.column);
    }

    LexLocation loc_;
};
