#include "Args.hpp"

#include "Config.hpp"
#include "Error.hpp"

namespace fer
{

ArgInfo::ArgInfo() : reqd(false), val_reqd(false) {}

ArgParser::ArgParser(int argc, const char **argv)
{
	for(int i = 0; i < argc; ++i) this->argv.push_back(argv[i]);

	arg_defs["help"].setLong("help").setShort("h").setHelp("prints help "
							       "information for program");
}

ArgInfo &ArgParser::add(StringRef argname)
{
	arg_defs[argname].lng = argname;
	return arg_defs[argname];
}

bool ArgParser::parse()
{
	String expect_key;
	bool expect_val	 = false;
	bool source_done = false;
	for(size_t i = 0; i < argv.size(); ++i) {
		StringRef arg = argv[i];
		if(expect_val) {
			opts[expect_key] = arg;
			expect_val	 = false;
			continue;
		}
		if(arg.rfind("--", 0) == 0) {
			if(arg.size() == 2) {
				source_done = true;
				continue;
			}
			arg = arg.substr(2);
			for(auto &a : arg_defs) {
				if(a.second.lng == arg) {
					opts.insert({a.first, ""});
					if(a.second.reqd) a.second.reqd = false;
					if(a.second.val_reqd) {
						expect_key = a.first;
						expect_val = true;
					}
				}
			}
			continue;
		}
		if(arg.rfind("-", 0) == 0) {
			arg = arg.substr(1);
			for(auto &a : arg_defs) {
				if(a.second.shrt == arg) {
					opts.insert({a.first, ""});
					if(a.second.reqd) a.second.reqd = false;
					if(a.second.val_reqd) {
						expect_key = a.first;
						expect_val = true;
					}
				}
			}
			continue;
		}
		if(!source_done) source = arg;
		else args.push_back(arg);
	}
	if(expect_val) {
		err::out({"Expected value to be provided for argument: ", expect_key});
		return false;
	}
	for(auto &a : arg_defs) {
		if(a.second.reqd && opts.find(a.first) == opts.end()) {
			err::out({"Required argument: ", a.first, " was not provided"});
			return false;
		}
	}
	return true;
}

void ArgParser::printHelp(OStream &os)
{
	os << PROJECT_NAME << " compiler " << COMPILER_MAJOR << "." << COMPILER_MINOR << "."
	   << COMPILER_PATCH << "\n";

	os << "usage: " << argv[0];
	for(auto &arg : arg_defs) {
		if(arg.second.reqd) {
			os << " [" << arg.first << "]";
		}
	}
	os << " <args>\n\n";
	for(auto &arg : arg_defs) {
		if(!arg.second.shrt.empty()) {
			os << "-" << arg.second.shrt << ", --" << arg.second.lng << "\t\t"
			   << arg.second.help << "\n";
		} else {
			os << "--" << arg.second.lng << "\t\t" << arg.second.help << "\n";
		}
	}
}

} // namespace fer