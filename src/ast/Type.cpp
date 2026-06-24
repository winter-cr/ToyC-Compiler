#include "ast/Type.h"

Type::Type(TypeKind kind, std::string name) : kind_(kind), name_(std::move(name)) {}

Type* Type::intType() {
    static Type instance(TypeKind::Int, "int");
    return &instance;
}

Type* Type::voidType() {
    static Type instance(TypeKind::Void, "void");
    return &instance;
}
