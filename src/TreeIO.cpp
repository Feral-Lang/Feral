#include "TreeIO.hpp"

#include <iostream>

namespace fer
{
namespace tio
{

static Vector<bool> &_tab()
{
	static Vector<bool> tabs;
	return tabs;
}

static void _tab_apply(bool has_next)
{
	for(size_t i = 0; i < _tab().size(); ++i) {
		if(i == _tab().size() - 1) {
			std::cout << (has_next ? " ├─" : " └─");
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

void print(bool has_next, InitList<StringRef> data)
{
	_tab_apply(has_next);
	for(auto &d : data) std::cout << d;
}
void printf(InitList<StringRef> data)
{
	for(auto &d : data) std::cout << d;
}

} // namespace tio
} // namespace fer
