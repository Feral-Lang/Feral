#pragma once

#include "Core.hpp"

namespace fer::ast
{

using namespace core;

class VRegManager
{
    Vector<Vector<StringRef>> data;
    Vector<Vector<size_t>> blocks;
    Vector<size_t> maxCount;

public:
    void pushFunc();
    size_t popFunc(); // returns maxCount for the function

    void pushBlk();
    void popBlk();

    bool pushName(StringRef name);

    size_t getIndex(StringRef name);

    size_t getLastIndex();
};

} // namespace fer::ast