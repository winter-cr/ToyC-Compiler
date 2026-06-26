CXX ?= c++
TARGET ?= toyc

CPPFLAGS += -Isrc -Ibackend/include -I.
CXXFLAGS ?= -std=c++20 -O2 -pipe -Wall -Wextra -Wpedantic

SOURCES := \
    src/main.cpp \
    src/lexer/Token.cpp \
    src/lexer/Lexer.cpp \
    src/ast/Type.cpp \
    src/ast/AST.cpp \
    src/ast/AstPrinter.cpp \
    src/parser/Parser.cpp \
    src/semantic/Symbol.cpp \
    src/semantic/SymbolTable.cpp \
    src/semantic/SemanticAnalyzer.cpp \
    src/semantic/ConstExprEvaluator.cpp \
    src/codegen/AstToIr.cpp \
    src/optimizer/DOptimizer.cpp \
    backend/src/ir.cpp \
    backend/src/optimizer.cpp \
    backend/src/riscv32_codegen.cpp \
    backend/src/text_ir.cpp

OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) -opt < tests/performance/loop_sum.tc > /tmp/toyc_loop_sum.s
	./$(TARGET) -opt < tests/performance/fib_heavy.tc > /tmp/toyc_fib_heavy.s

clean:
	rm -f $(TARGET) $(OBJECTS)