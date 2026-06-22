#pragma once

#include "ast/AstNode.h"
#include "ast/Decl.h"
#include "ast/FuncDef.h"

#include <memory>
#include <variant>
#include <vector>

using TopLevelItem = std::variant<std::unique_ptr<VarDecl>,
                                  std::unique_ptr<ConstDecl>,
                                  std::unique_ptr<FuncDef>>;

class CompUnit : public AstNode {
public:
    explicit CompUnit(SourceLocation loc, std::vector<TopLevelItem> items)
        : AstNode(AstNodeKind::CompUnit, loc), items_(std::move(items)) {}

    const std::vector<TopLevelItem>& items() const { return items_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::vector<TopLevelItem> items_;
};
