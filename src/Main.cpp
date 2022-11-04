#include "Args.hpp"
#include "Config.hpp"

using namespace fer;

int execInterpreter(args::ArgParser &cmdargs);

int main(int argc, char **argv)
{
	args::ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("lex").setShort("l").setHelp("shows lexical tokens");
	args.add("parse").setShort("p").setHelp("shows AST");
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
		return execInterpreter(args);
	}

	StringRef file = args.getSource();

	return 0;
}

int execInterpreter(args::ArgParser &cmdargs)
{
	std::cout << PROJECT_NAME << " compiler " << COMPILER_MAJOR << "." << COMPILER_MINOR << "."
		  << COMPILER_PATCH << "(" << REPO_URL << " " << COMMIT_ID << " " << TREE_STATUS
		  << "\nBuilt with " << BUILD_CXX_COMPILER << "\nOn " << BUILD_DATE << "\n";
	// TODO:
	return 0;
}