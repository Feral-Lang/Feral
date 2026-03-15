#include "AST/VirtualRegisters.hpp"

namespace fer::ast
{

void VRegManager::pushFunc()
{
    data.push_back({});
    blocks.push_back({});
    maxCount.push_back(0);
    // kw, va, self are permanent
    pushName(".kw."); // kw placeholder
    pushName(".va."); // va placeholder
    pushName("self");
}
size_t VRegManager::popFunc()
{
    size_t res = maxCount.back();
    maxCount.pop_back();
    blocks.pop_back();
    data.pop_back();
    return res;
}

void VRegManager::pushBlk()
{
    if(data.empty()) return;
    blocks.back().push_back(data.back().size());
}
void VRegManager::popBlk()
{
    if(data.empty()) return;
    while(data.back().size() > blocks.back().back()) data.back().pop_back();
    blocks.back().pop_back();
}

bool VRegManager::pushName(StringRef name)
{
    if(data.empty()) return false;
    data.back().push_back(name);
    if(maxCount.back() < data.back().size()) ++maxCount.back();
    return true;
}
void VRegManager::setName(size_t index, StringRef name)
{
    if(data.empty()) return;
    data.back()[index] = name;
}

size_t VRegManager::getIndex(StringRef name)
{
    if(data.empty()) return -1;
    auto loc = std::find(data.back().begin(), data.back().end(), name);
    if(loc == data.back().end()) return -1;
    return loc - data.back().begin();
}

size_t VRegManager::getLastIndex()
{
    if(data.empty()) return -1;
    return data.back().size() - 1;
}

} // namespace fer::ast