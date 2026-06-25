#ifndef TOYC_TYPE_H
#define TOYC_TYPE_H

enum class TypeKind { INT, VOID };

struct Type {
    TypeKind kind;

    static Type* intType() {
        static Type t{TypeKind::INT};
        return &t;
    }
    static Type* voidType() {
        static Type t{TypeKind::VOID};
        return &t;
    }

    bool isInt()  const { return kind == TypeKind::INT; }
    bool isVoid() const { return kind == TypeKind::VOID; }

    bool operator==(const Type& o) const { return kind == o.kind; }
    bool operator!=(const Type& o) const { return kind != o.kind; }
};

#endif
