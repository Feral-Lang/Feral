/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Error.hpp"

#include "FS.hpp"

namespace fer
{

ErrorHandler err(10);

ModuleLoc::ModuleLoc() : id((ModuleId)-1), offset(static_cast<ModuleId>(-1)) {}
ModuleLoc::ModuleLoc(ModuleId id, uint64_t offset) : id(id), offset(offset) {}

ErrorHandler::ErrorHandler(size_t maxErrors) : maxErrors(maxErrors) {}
const char *ErrorHandler::getPathForId(ModuleId id)
{
	auto loc = paths.find(id);
	if(loc == paths.end()) return "";
	return loc->second.c_str();
}
const char *ErrorHandler::getCodeForId(ModuleId id)
{
	auto loc = codes.find(id);
	if(loc == codes.end()) return "";
	return loc->second.c_str();
}

void ErrorHandler::outputString(ModuleLoc loc, bool iswarn, const String &e)
{
	static size_t errCount = 0;

	if(errCount >= maxErrors) return;

	String path    = getPathForId(loc.id);
	StringRef code = getCodeForId(loc.id);
	String spacingCaret;
	StringRef errLine;
	size_t lineNum, column;
	size_t tabCount = 0;

	// Just show the error
	if(loc.id == (ModuleId)-1 || path.empty()) goto justError;
	if(!fs::exists(path) && code.empty()) goto justError;
	if(code.empty()) {
		String codeData;
		if(!fs::read(path.c_str(), codeData, true)) goto justError;
		setCodeForId(loc.id, std::move(codeData));
		code = getCodeForId(loc.id);
	}
	if(!getLineAndColumnFromData(code, loc.offset, errLine, lineNum, column)) goto justError;

	for(auto &c : errLine) {
		if(c == '\t') ++tabCount;
	}
	spacingCaret = String(column, ' ');
	while(tabCount--) {
		spacingCaret.pop_back();
		spacingCaret.insert(spacingCaret.begin(), '\t');
	}

	std::cout << path << " (" << lineNum << ":" << column + 1 << "): ";
	std::cout << (iswarn ? "Warning" : "Failure") << ": ";
	std::cout << e;
	std::cout << "\n";
	std::cout << errLine << "\n";
	std::cout << spacingCaret << "^\n";

	if(!iswarn) ++errCount;
	if(errCount >= maxErrors) std::cout << "Failure: Too many errors encountered\n";
	return;
justError:
	std::cout << (iswarn ? "Warning" : "Failure") << ": ";
	std::cout << e;
	std::cout << "\n";
	if(!iswarn) ++errCount;
	if(errCount >= maxErrors) std::cout << "Failure: Too many errors encountered\n";
}

bool ErrorHandler::getLineAndColumnFromData(StringRef data, size_t offset, StringRef &line,
					    size_t &lineNum, size_t &column)
{
	line	= "";
	lineNum = 1;
	column	= -1;

	size_t lineStart = 0, lineEnd = 0;
	bool breakOnNewline = false;
	for(size_t i = 0; i < data.size(); ++i) {
		if(data[i] == '\n') {
			if(breakOnNewline) break;
			lineStart = i + 1;
			lineEnd	  = i;
			++lineNum;
			continue;
		}
		if(i == offset) {
			breakOnNewline = true;
			column	       = i - lineStart;
		}
		++lineEnd;
	}
	if(column == -1) {
		lineNum = 0;
		return false;
	}
	line = StringRef(&data[lineStart], lineEnd - lineStart + 1);
	return true;
}

} // namespace fer