#include <iostream>
#include <string>

int main(int argc, char** argv) {
    bool optimize = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-opt") {
            optimize = true;
        }
    }

    std::string source;
    std::string line;
    while (std::getline(std::cin, line)) {
        source += line;
        source += '\n';
    }

    (void)source;

    std::cout << "    .text\n";
    std::cout << "    .globl main\n";
    std::cout << "main:\n";
    std::cout << "    li a0, 0\n";
    std::cout << "    ret\n";

    if (optimize) {
        std::cerr << "ToyC bootstrap compiler: -opt accepted\n";
    }

    return 0;
}
