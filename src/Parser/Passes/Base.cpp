#include "AST/Passes/Base.hpp"

namespace fer::ast
{

Pass::Pass(const size_t &passid, ManagedAllocator &allocator) : passid(passid), allocator(allocator)
{}
Pass::~Pass() {}

PassManager::PassManager() {}
PassManager::~PassManager()
{
    for(auto &p : passes) delete p;
}
bool PassManager::visit(Stmt *&ptree)
{
    for(auto &p : passes) {
        if(!p->visit(ptree, &ptree)) return false;
    }
    return true;
}

} // namespace fer::ast