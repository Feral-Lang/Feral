#pragma once

#include "VM/VarTypes.hpp"

#if defined(FER_OS_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

namespace fer
{

class VarSocket : public Var
{
    int fd;
    int domain;
    int type;
    bool closed;

    void onDestroy(VirtualMachine &vm) override;

public:
    VarSocket(ModuleLoc loc, int fd, int domain, int type);

    void closeSocket();

    inline int getFd() const { return fd; }
    inline int getDomain() const { return domain; }
    inline int getType() const { return type; }
    inline bool isClosed() const { return closed; }
};

class VarAddr : public Var
{
    struct sockaddr_storage addr;
    socklen_t addrLen;
    String repr;

    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarAddr(ModuleLoc loc);
    VarAddr(ModuleLoc loc, const struct sockaddr_storage &addr, socklen_t addrLen);

    void updateRepr();

    inline struct sockaddr_storage &getAddr() { return addr; }
    inline socklen_t &getAddrLen() { return addrLen; }
    inline const String &getRepr() const { return repr; }
};

} // namespace fer
