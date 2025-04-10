#include "Args.hpp"
#include "AST/Parser.hpp"
#include "AST/Passes/Codegen.hpp"
#include "AST/Passes/Simplify.hpp"
#include "Core.hpp"
#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Logger.hpp"
#include "VM/Interpreter.hpp"

using namespace fer;

// Uses its own AST Allocator which gets deleted when control leaves the function.
// `bc` is the output variable here.
bool ParseSource(VirtualMachine &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
		 StringRef code, bool exprOnly);

int main(int argc, char **argv)
{
	ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("tokens").setShort("t").setHelp("shows lexical tokens");
	args.add("parse").setShort("p").setHelp("shows AST");
	args.add("optparse").setShort("P").setHelp("shows optimized AST (AST after passes)");
	args.add("ir").setShort("i").setHelp("shows codegen IR");
	args.add("dry").setShort("d").setHelp("dry run - generate IR but don't run the VM");
	args.add("logerr").setShort("e").setHelp("show logs on stderr");
	args.add("verbose").setShort("V").setHelp("show verbose compiler output");
	args.add("trace").setShort("T").setHelp("show trace (even more verbose) compiler output");
	if(!args.parse()) return 1;

	if(args.has("help")) {
		args.printHelp(std::cout);
		return 0;
	}

	if(args.has("version")) {
		std::cout << PROJECT_NAME << " " << PROJECT_MAJOR << "." << PROJECT_MINOR << "."
			  << PROJECT_PATCH << " (" << REPO_URL << " " << COMMIT_ID << " "
			  << TREE_STATUS << ")\nBuilt with " << BUILD_COMPILER << "\nOn "
			  << BUILD_DATE << "\n";
		return 0;
	}

	if(args.has("logerr")) logger.addSink(&std::cerr, true, false);
	if(args.has("verbose")) logger.setLevel(LogLevels::INFO);
	else if(args.has("trace")) logger.setLevel(LogLevels::TRACE);

	if(args.getSource().empty()) {
		// args.setSource("<repl>");
		// return ExecInteractive(args);
		std::cout << "FATAL: Unimplemented interactive mode\n";
		return 1;
	}

	const char *file = args.getSource().c_str();

	if(!fs::exists(file)) {
		String binfile(fs::parentDir(env::getProcPath()));
#if defined(FER_OS_WINDOWS)
		binfile += "\\";
#else
		binfile += "/";
#endif
		binfile += file;
		binfile += ".fer";
		if(!fs::exists(binfile)) {
			err.fail({}, "File ", file, " does not exist");
			return 1;
		}
		args.setSource(binfile);
		file = args.getSource().c_str();
	}

	Interpreter ip(args, ParseSource);
	return ip.runFile({}, fs::absPath(file).c_str());
}

bool ParseSource(VirtualMachine &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
		 StringRef code, bool exprOnly)
{
	Vector<lex::Lexeme> tokens;
	if(!lex::tokenize(moduleId, path, code, tokens)) {
		std::cout << "Failed to tokenize file: " << path << "\n";
		return false;
	}

	ArgParser &args = vm.getArgParser();

	if(args.has("tokens")) {
		std::cout << "====================== Tokens for: " << path
			  << " ======================\n";
		lex::dumpTokens(std::cout, tokens);
	}

	MemoryManager &mem = vm.getMemoryManager();

	// Separate allocator for AST since we don't want the AST nodes (Stmt) to persist outside
	// this function - because this function is supposed to generate IR for the VM to consume.
	Allocator astallocator(mem, utils::toString("AST(", path, ")"));
	ast::Stmt *ptree = nullptr;
	if(!ast::parse(astallocator, tokens, ptree, exprOnly)) {
		std::cout << "Failed to parse tokens for file: " << path << "\n";
		return false;
	}
	if(args.has("parse")) {
		std::cout << "====================== AST for: " << path
			  << " ======================\n";
		ast::dumpTree(std::cout, ptree);
	}

	ast::PassManager pm;

	pm.add<ast::SimplifyPass>(astallocator);
	pm.add<ast::CodegenPass>(astallocator, bc);

	if(!pm.visit((ast::Stmt *&)ptree)) {
		logger.fatal("Failed to perform passes on AST for file: ", path);
		return false;
	}
	if(args.has("optparse")) {
		std::cout << "====================== Optimized AST for: " << path
			  << " ======================\n";
		ast::dumpTree(std::cout, ptree);
	}
	if(args.has("ir")) {
		std::cout << "====================== IR for: " << path
			  << " ======================\n";
		bc.dump(std::cout);
	}
	return true;
}