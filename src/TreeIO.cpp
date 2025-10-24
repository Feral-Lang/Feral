#include "TreeIO.hpp"

#include <iostream>

namespace fer::tio
{

static Vector<bool> &_tab()
{
    static Vector<bool> tabs;
    return tabs;
}

static void tabApply(bool hasNext)
{
    for(size_t i = 0; i < _tab().size(); ++i) {
        if(i == _tab().size() - 1) {
            std::cout << (hasNext ? " ├─" : " └─");
        } else {
            std::cout << (_tab()[i] ? " │" : "  ");
        }
    }
}

void taba(bool show) { _tab().push_back(show); }

void tabr(size_t num)
{
    if(num > _tab().size()) return;
    for(size_t i = 0; i < num; ++i) _tab().pop_back();
}

void print(bool hasNext, InitList<StringRef> data)
{
    tabApply(hasNext);
    for(auto &d : data) std::cout << d;
}
void printf(InitList<StringRef> data)
{
    for(auto &d : data) std::cout << d;
}

} // namespace fer::tio
