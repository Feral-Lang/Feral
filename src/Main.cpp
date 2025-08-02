#include "AST/Parser.hpp"
#include "AST/Passes/Codegen.hpp"
#include "AST/Passes/Simplify.hpp"
#include "VM/Interpreter.hpp"

using namespace fer;

// Uses its own AST Allocator which gets deleted when control leaves the function.
// `bc` is the output variable here.
bool ParseSource(VirtualMachine &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
		 StringRef code, bool exprOnly);

int main(int argc, char **argv)
{
	args::ArgParser args(argc, (const char **)argv);
	args.add("version").addOpt("--version").addOpt("-v").setHelp("prints program version");
	args.add("tokens").addOpt("--tokens").addOpt("-t").setHelp("shows lexical tokens");
	args.add("parse").addOpt("--parse").addOpt("-p").setHelp("shows AST");
	args.add("optparse")
	.addOpt("--optparse")
	.addOpt("-P")
	.setHelp("shows optimized AST (AST after passes)");
	args.add("ir").addOpt("--ir").addOpt("-i").setHelp("shows codegen IR");
	args.add("dry").addOpt("--dry").addOpt("-d").setHelp(
	"dry run - generate IR but don't run the VM");
	args.add("logerr").addOpt("--logerr").addOpt("-e").setHelp("show logs on stderr");
	args.add("verbose")
	.addOpt("--verbose")
	.addOpt("-V")
	.setHelp("show verbose compiler output");
	args.add("trace").addOpt("--trace").addOpt("-T").setHelp(
	"show trace (even more verbose) compiler output");
	args.add("source").setHelp("Source file to compile/run");
	args.setLastArg("source");
	Status<bool> argsRes = args.parse();
	if(!argsRes.getCode()) {
		std::cerr << "Failed to parse cli args: " << argsRes.getMsg() << "\n";
		return 1;
	}

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

	String srcFile(args.getValue("source"));
	if(srcFile.empty()) {
		// args.setSource("<repl>");
		// return ExecInteractive(args);
		std::cerr << "FATAL: Unimplemented interactive mode\n";
		return 1;
	}

	if(!fs::exists(srcFile)) {
		String binFile(fs::parentDir(env::getProcPath()));
#if defined(CORE_OS_WINDOWS)
		binFile += "\\";
#else
		binFile += "/";
#endif
		binFile += srcFile;
		binFile += ".fer";
		if(!fs::exists(binFile)) {
			err.fail({}, "File ", srcFile, " does not exist");
			return 1;
		}
		srcFile = binFile;
	}

	Interpreter ip(args, ParseSource);
	return ip.runFile({}, fs::absPath(srcFile.c_str()).c_str());
}

bool ParseSource(VirtualMachine &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
		 StringRef code, bool exprOnly)
{
	Vector<lex::Lexeme> tokens;
	if(!lex::tokenize(moduleId, path, code, tokens)) {
		std::cout << "Failed to tokenize file: " << path << "\n";
		return false;
	}

	args::ArgParser &args = vm.getArgParser();

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