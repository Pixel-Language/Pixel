#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <cstdio>
#include <filesystem>
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
#include "TypeChecker.h"

#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
#else
    #define PLATFORM_LINUX 1
#endif

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::string source_file;
    bool use_interpreter = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--use-interpreter") {
            use_interpreter = true;
        } else if (source_file.empty()) {
            source_file = arg;
        }
    }

    if (source_file.empty()) {
        std::cerr << "usage: pixel <source_file> [--use-interpreter]\n";
        return 1;
    }

    std::ifstream in_file(source_file);
    if (!in_file.is_open()) {
        std::cerr << "error: could not open file '" << source_file << "'\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << in_file.rdbuf();
    std::string source_code = buffer.str();
    in_file.close();

    size_t last_slash = source_file.find_last_of("\\/");
    std::string source_dir = (last_slash == std::string::npos) ? "." : source_file.substr(0, last_slash);

    std::cout << "pixel: compiling " << source_file << "...\n";

    Lexer lexer(source_code, source_file);
    std::vector<Token> tokens = lexer.tokenize();
    std::cout << "lexer tokenized\n";

    Parser parser(std::move(tokens));
    parser.set_source_dir(source_dir);
    std::vector<std::unique_ptr<ASTNode>> ast = parser.parse_program();

    if (parser.has_errors()) {
        std::cerr << "pixel: compilation failed due to parser errors\n";
        return 1;
    }

    // Type checker disabled for now
    // std::cout << "type checking...\n";
    // TypeChecker type_checker;
    // if (!type_checker.check(ast)) {
    //     std::cerr << "pixel: compilation failed due to type errors\n";
    //     return 1;
    // }
    // std::cout << "type checking passed\n";

    // interpreter backend

    std::cout << "pixel: type checking...\n";

    TypeChecker typechecker;

    bool t = typechecker.check_program(ast);

    if (!t) {
        return 1;
    }

    std::cout << "pixel: types checked\n";

    std::cout << "pixel: running interpreter...\n";
    Interpreter interpreter;
    interpreter.interpret(ast);
    std::cout << "\npixel: interpreter finished\n";
    
    return 0;
}