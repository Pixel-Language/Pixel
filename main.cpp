#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "Codegen.h"

// Detect OS at compile time
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
#else
    #define PLATFORM_LINUX 1
#endif

// um well i havent tested this thing on linux yet, oh well. (it probably wont work)

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "usage: pixel <source_file>\n";
		return 1;
	}

	std::string filename = argv[1];
	std::string source_code;

	std::ifstream in_file(filename);
	if (!in_file.is_open()) {
		std::cerr << "error: could not open file '" << filename << "'\n";
		return 1;
	}

	std::stringstream buffer;
	buffer << in_file.rdbuf();
	source_code = buffer.str();
	in_file.close();

	size_t last_slash = filename.find_last_of("\\/");
    std::string source_dir = (last_slash == std::string::npos) ? "." : filename.substr(0, last_slash);

	std::cout << "pixel: compiling " << filename << "...\n";

	Lexer lexer(source_code);
	std::vector<Token> tokens = lexer.tokenize();
	std::cout << "lexer tokenized\n";

	Parser parser(std::move(tokens));
	parser.set_source_dir(source_dir);
	std::vector<std::unique_ptr<ASTNode>> ast = parser.parse_program();

	Codegen codegen;
	codegen.set_source_dir(source_dir);
	codegen.set_stdlib_path("lib/");
	std::string c_code = codegen.generate_c_code(ast);

	std::ofstream out_file("output.c");
	out_file << c_code;
	out_file.close();

	std::cout << "\n\n" << c_code << "\n\n";
	std::cout << "pixel: generated output.c\n";

	std::string compile_cmd;
	std::string exe_name;

	#ifdef PLATFORM_WINDOWS
		compile_cmd = "g++ -std=c++17 -I\"lib\" output.c -L\"lib\" -lraylib -lopengl32 -lgdi32 -lwinmm -o pixel_program.exe";
		exe_name = "pixel_program.exe";
	#else
		compile_cmd = "g++ -std=c++17 -I./lib output.c -L./lib -lraylib -lGL -lm -o pixel_program";
		exe_name = "./pixel_program";
	#endif

	int compile_result = std::system(compile_cmd.c_str());

	if (compile_result == 0) {
		std::cout << "pixel: execution output\n";
		std::system(exe_name.c_str());
		std::cout << "\n";
	}
	else {
		std::cerr << "pixel: error: gcc failed to compile\n";
	}

	return 0;
}