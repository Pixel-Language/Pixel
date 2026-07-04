#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <cstdio>  // for std::remove
#include <filesystem> // for path manipulations 'n stuff
#include "Lexer.h"
#include "Parser.h"
#include "Codegen.h"
#include "TypeChecker.h"  // <-- ADD THIS

// Detect OS at compile time (i havent tested this thing on linux yet, oh well)
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
#else
    #define PLATFORM_LINUX 1
#endif

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string source_file;
    bool keep_c_file = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--keep-c-file") {
            keep_c_file = true;
        } else if (source_file.empty()) {
            source_file = arg;
        }
    }

    if (source_file.empty()) {
        std::cerr << "usage: pixel <source_file> [--keep-c-file]\n";
        return 1;
    }

    // Read source code
    std::ifstream in_file(source_file);
    if (!in_file.is_open()) {
        std::cerr << "error: could not open file '" << source_file << "'\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << in_file.rdbuf();
    std::string source_code = buffer.str();
    in_file.close();

    // Determine source directory for includes
    size_t last_slash = source_file.find_last_of("\\/");
    std::string source_dir = (last_slash == std::string::npos) ? "." : source_file.substr(0, last_slash);

    std::cout << "pixel: compiling " << source_file << "...\n";

    // Lexical analysis
    Lexer lexer(source_code, source_file);
    std::vector<Token> tokens = lexer.tokenize();
    std::cout << "lexer tokenized\n";

    // Parsing
    Parser parser(std::move(tokens));
    parser.set_source_dir(source_dir);
    std::vector<std::unique_ptr<ASTNode>> ast = parser.parse_program();

    // errors n stuff
    if (parser.has_errors()) {
        std::cerr << "pixel: compilation failed due to parser errors\n";
        return 1;
    }

    // type checking stuff (removed for now because the typechecker is a load of work rn, will add back in a bit)
    // std::cout << "type checking...\n";
    // TypeChecker type_checker;
    // if (!type_checker.check(ast)) {
    //     std::cerr << "pixel: compilation failed due to type errors\n";
    //     return 1;
    // }
    // std::cout << "type checking passed\n";

    // Code generation
    Codegen codegen;
    codegen.set_source_dir(source_dir);
    codegen.set_stdlib_path("lib/");
    std::string c_code = codegen.generate_c_code(ast);

    // Write C file
    const std::string c_filename = "output.c";
    std::ofstream out_file(c_filename);
    out_file << c_code;
    out_file.close();

    std::cout << "\n\n" << c_code << "\n\n";
    std::cout << "pixel: generated " << c_filename << "\n";

    // Build executable name from input file stem
    fs::path src_path(source_file);
    std::string exe_stem = src_path.stem().string();   // main from main.pixel

    std::string compile_cmd;
    std::string run_cmd;

#ifdef PLATFORM_WINDOWS
    std::string exe_name = exe_stem + ".exe";
    compile_cmd = "g++ -std=c++17 -I\"lib\" " + c_filename +
                  " -L\"lib\" -lraylib -lopengl32 -lgdi32 -lwinmm -o " + exe_name;
    run_cmd = exe_name;
#else
    std::string exe_name = exe_stem;
    compile_cmd = "g++ -std=c++17 -I./lib " + c_filename +
                  " -L./lib -lraylib -lGL -lm -o " + exe_name;
    run_cmd = "./" + exe_name;
#endif

    // Compile
    int compile_result = std::system(compile_cmd.c_str());

    if (compile_result == 0) {
        std::cout << "pixel: execution output\n";
        std::system(run_cmd.c_str());
        std::cout << "\n";
    } else {
        std::cerr << "pixel: error: g++ failed to compile\n";
    }

    // Delete the intermediate C file unless --keep-c-file was given
    if (!keep_c_file) {
        if (std::remove(c_filename.c_str()) != 0) {
            std::cerr << "pixel: warning: could not delete " << c_filename << "\n";
        } else {
            std::cout << "pixel: removed " << c_filename << "\n";
        }
    }

    return 0;
}