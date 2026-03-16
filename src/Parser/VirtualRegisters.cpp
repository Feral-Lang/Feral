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

size_t VRegManager::pushName(StringRef name)
{
    if(data.empty()) return -1;
    if(!blocks.back().empty()) {
        size_t existing = getIndex(name, blocks.back().back());
        if(existing != -1) return existing;
    }
    data.back().push_back(name);
    if(maxCount.back() < data.back().size()) ++maxCount.back();
    return data.back().size() - 1;
}
void VRegManager::setName(size_t index, StringRef name)
{
    if(data.empty()) return;
    data.back()[index] = name;
}

size_t VRegManager::getIndex(StringRef name, size_t from)
{
    if(data.empty() || data.back().empty()) return -1;
    for(size_t i = data.back().size() - 1; i >= from; --i) {
        if(data.back()[i] == name) return i;
        if(i == 0) break;
    }
    return -1;
}

size_t VRegManager::getLastIndex()
{
    if(data.empty()) return -1;
    return data.back().size() - 1;
}

} // namespace fer::ast