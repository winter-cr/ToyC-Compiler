#pragma once

#include <string>

enum class TypeKind {
    Int,
    Void,
};

class Type {
public:
    TypeKind kind() const { return kind_; }
    const std::string& name() const { return name_; }

    bool is_int() const { return kind_ == TypeKind::Int; }
    bool is_void() const { return kind_ == TypeKind::Void; }

    static Type* intType();
    static Type* voidType();

private:
    Type(TypeKind kind, std::string name);

    TypeKind kind_;
    std::string name_;
};
