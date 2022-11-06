#include "Args.hpp"
#include "Config.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "RAIIParser.hpp"

using namespace fer;

int execInteractive(ArgParser &cmdargs);
int compileAndRun(RAIIParser &parser, const String &file);

int main(int argc, char **argv)
{
	ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("lex").setShort("l").setHelp("shows lexical tokens");
	args.add("parse").setShort("p").setHelp("shows AST");
	args.add("optparse").setShort("P").setHelp("shows optimized AST (AST after passes)");
	args.add("ir").setShort("i").setHelp("shows codegen IR");
	args.add("testdir").setShort("t").setHelp("test all feral programs in a directory");
	args.add("verbose").setShort("V").setHelp("show verbose compiler output");
	if(!args.parse()) return 1;

	if(args.has("help")) {
		args.printHelp(std::cout);
		return 0;
	}

	if(args.has("version")) {
		std::cout << PROJECT_NAME << " " << COMPILER_MAJOR << "." << COMPILER_MINOR << "."
			  << COMPILER_PATCH << " (" << REPO_URL << " " << COMMIT_ID << " "
			  << TREE_STATUS << "\nBuilt with " << BUILD_CXX_COMPILER << "\nOn "
			  << BUILD_DATE << "\n";
		return 0;
	}

	if(args.getSource().empty()) {
		args.setSource("<prompt>");
		return execInteractive(args);
	}

	String file = String(args.getSource());

	if(!fs::exists(file)) {
		err::out({"File ", file, " does not exist"});
		return 1;
	}
	file = fs::absPath(file);

	RAIIParser parser(args);
	return compileAndRun(parser, file);
}

int compileAndRun(RAIIParser &parser, const String &file)
{
	ArgParser &args = parser.getCommandArgs();
	Module *mod	= parser.createModule(file, true);
	if(!mod) return 1;
	if(!mod->tokenize()) return 1;
	if(args.has("lex")) mod->dumpTokens();
	if(!mod->parseTokens()) return 1;
	if(args.has("parse")) mod->dumpParseTree();
	if(!parser.applyDefaultParserPasses(mod)) {
		err::out({"Failed to apply parser passes on module: ", mod->getPath()});
		return 1;
	}
	if(args.has("optparse")) mod->dumpParseTree();

	// convert to bytecode
	// apply bytecode passes
	// execute on the VM

	return 0;
}

int execInteractive(ArgParser &cmdargs)
{
	std::cout << PROJECT_NAME << " compiler " << COMPILER_MAJOR << "." << COMPILER_MINOR << "."
		  << COMPILER_PATCH << "(" << REPO_URL << " " << COMMIT_ID << " " << TREE_STATUS
		  << "\nBuilt with " << BUILD_CXX_COMPILER << "\nOn " << BUILD_DATE << "\n";
	// TODO:
	return 0;
}