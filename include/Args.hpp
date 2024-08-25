#pragma once

#include "Core.hpp"

namespace fer
{

class ArgInfo
{
	String shrt, lng; // short and long names
	String val;	  // value for argument
	String help;	  // help string for argument
	bool reqd;	  // required argument(?)
	bool val_reqd;	  // value required for argument(?)

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
	Vector<StringRef> args;		  // args to interpreter (after source)
					  // (can use '--' to force start these)
	String source;			  // provided source (file) name

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
	inline const String &getSource() { return source; }
	inline void setSource(StringRef src) { source = src; }
	inline Span<StringRef> getCodeExecArgs() { return args; }
	inline Span<StringRef> getArgv() { return argv; }
};

} // namespace fer