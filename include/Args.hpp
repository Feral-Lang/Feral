#pragma once

#include "Core.hpp"

namespace fer
{

class ArgInfo
{
	StringRef shrt, lng; // short and long names
	StringRef val;	     // value for argument
	StringRef help;	     // help string for argument
	bool reqd;	     // required argument(?)
	bool val_reqd;	     // value required for argument(?)

public:
	ArgInfo();

	inline ArgInfo &setShort(StringRef name)
	{
		shrt = name;
		return *this;
	}
	inline ArgInfo &setLong(StringRef name)
	{
		lng = name;
		return *this;
	}
	inline ArgInfo &setHelp(StringRef val)
	{
		help = val;
		return *this;
	}
	inline ArgInfo &setReqd(bool req)
	{
		reqd = req;
		return *this;
	}
	inline ArgInfo &setValReqd(bool req)
	{
		val_reqd = req;
		return *this;
	}

	friend class ArgParser;
};

class ArgParser
{
	Vector<StringRef> argv;
	Map<StringRef, ArgInfo> arg_defs; // what/how to parse
	Map<StringRef, StringRef> opts;	  // parsed args
	StringRef source;		  // provided source (file) name
	Vector<StringRef> args;		  // args to interpreter (after source)
					  // (can use '--' to force start these)

public:
	ArgParser(int argc, const char **argv);

	ArgInfo &add(StringRef argname);
	bool parse();
	void printHelp(OStream &os);

	// retrieve info
	inline bool has(StringRef argname) { return opts.find(argname) != opts.end(); }
	inline StringRef val(StringRef argname)
	{
		if(has(argname)) return opts[argname];
		return "";
	}
	inline StringRef getSource() { return source; }
	inline void setSource(StringRef src) { source = src; }
	inline Vector<StringRef> &getCodeExecArgs() { return args; }
	inline const Vector<StringRef> &getArgv() const { return argv; }
};

} // namespace fer