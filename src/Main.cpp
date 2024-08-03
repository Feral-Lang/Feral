#include "Args.hpp"
#include "Config.hpp"
#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "RAIIParser.hpp"
#include "VM/Interpreter.hpp"

using namespace fer;

int execInteractive(ArgParser &cmdargs);

void showVersion();

int main(int argc, char **argv)
{
	ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("lex").setShort("l").setHelp("shows lexical tokens");
	args.add("parse").setShort("p").setHelp("shows AST");
	args.add("optparse").setShort("P").setHelp("shows optimized AST (AST after passes)");
	args.add("ir").setShort("i").setHelp("shows codegen IR");
	args.add("dry").setShort("d").setHelp("dry run - generate IR but don't run the VM");
	args.add("verbose").setShort("V").setHelp("show verbose compiler output");
	if(!args.parse()) return 1;

	if(args.has("help")) {
		args.printHelp(std::cout);
		return 0;
	}

	if(args.has("version")) {
		showVersion();
		return 0;
	}

	if(args.getSource().empty()) {
		args.setSource("<prompt>");
		return execInteractive(args);
	}

	String file		    = String(args.getSource());
	Vector<StringRef> &codeargs = args.getCodeExecArgs();

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
			err::out(nullptr, "File ", file, " does not exist");
			return 1;
		}
		args.setSource(binfile);
		file = binfile;
	}
	file = fs::absPath(file.c_str());

	RAIIParser parser(args);
	Interpreter vm(parser);
	return vm.compileAndRun(nullptr, std::move(file), true);
}

int execInteractive(ArgParser &cmdargs)
{
	showVersion();
	// TODO:
	return 0;
}

void showVersion()
{
	std::cout << PROJECT_NAME << " " << VERSION_MAJOR << "." << VERSION_MINOR << "."
		  << VERSION_PATCH << " (" << REPO_URL << " " << COMMIT_ID << " " << TREE_STATUS
		  << ")\nBuilt with " << BUILD_CXX_COMPILER << "\nOn " << BUILD_DATE << "\n";
}