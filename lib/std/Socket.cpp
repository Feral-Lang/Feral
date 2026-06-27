#include "Socket.hpp"

#include <inttypes.h>

#include "VM/VM.hpp"

#if defined(FER_OS_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define sockClose(fd) closesocket(fd)
#define sockWouldBlock() (WSAGetLastError() == WSAEWOULDBLOCK)
#define sockInProgress() (WSAGetLastError() == WSAEWOULDBLOCK)
#define PollFd WSAPOLLFD
#define sockPoll(fds, n, ms) WSAPoll(fds, n, ms)
static fer::String sockErrStr() { return "WSA error " + std::to_string(WSAGetLastError()); }
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <unistd.h>
#define sockClose(fd) close(fd)
#define sockWouldBlock() (errno == EAGAIN || errno == EWOULDBLOCK)
#define sockInProgress() (errno == EINPROGRESS)
#define PollFd struct pollfd
#define sockPoll(fds, n, ms) poll(fds, n, ms)
static fer::String sockErrStr() { return strerror(errno); }
#endif

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Helpers //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

static bool setNonBlocking(int fd)
{
#if defined(FER_OS_WINDOWS)
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

// Formats a sockaddr_storage into a "host:port" string.
static String addrToStr(const struct sockaddr_storage &addr)
{
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    int res = getnameinfo((const struct sockaddr *)&addr, sizeof(addr), host, sizeof(host), serv,
                          sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
    if(res != 0) return "<unknown>";
    return String(host) + ":" + serv;
}

// Resolves `host`:`port` using getaddrinfo, matching the given address family and socket type.
// Passing an empty `host` resolves to the wildcard address (for binding).
// NOTE: getaddrinfo performs DNS resolution and may block briefly.
static bool resolveAddr(VirtualMachine &vm, ModuleLoc loc, const String &host, uint16_t port,
                        int domain, int sockType, struct addrinfo **result)
{
    struct addrinfo hints{};
    hints.ai_family   = domain;
    hints.ai_socktype = sockType;
    if(host.empty()) hints.ai_flags = AI_PASSIVE;
    char portStr[8] = {0};
    std::snprintf(portStr, sizeof(portStr) / sizeof(portStr[0]), "%" PRIu16, port);
    int res = getaddrinfo(host.empty() ? nullptr : host.c_str(), portStr, &hints, result);
    if(res != 0) {
#if defined(FER_OS_WINDOWS)
        vm.fail(loc, "failed to resolve '", host, "': error ", WSAGetLastError());
#else
        vm.fail(loc, "failed to resolve '", host, "': ", gai_strerror(res));
#endif
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarSocket //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarSocket::VarSocket(ModuleLoc loc, int fd, int domain, int type)
    : Var(loc), fd(fd), domain(domain), type(type), closed(false)
{}

void VarSocket::onDestroy(VirtualMachine &vm) { closeSocket(); }

void VarSocket::closeSocket()
{
    if(!closed) {
        sockClose(fd);
        closed = true;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// VarAddr ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAddr::VarAddr(ModuleLoc loc) : Var(loc), addrLen(sizeof(struct sockaddr_storage))
{
    memset(&addr, 0, sizeof(addr));
}

VarAddr::VarAddr(ModuleLoc loc, const struct sockaddr_storage &addr, socklen_t addrLen)
    : Var(loc), addr(addr), addrLen(addrLen)
{
    updateRepr();
}

bool VarAddr::onSet(VirtualMachine &vm, Var *from)
{
    VarAddr *src = as<VarAddr>(from);
    addr         = src->addr;
    addrLen      = src->addrLen;
    repr         = src->repr;
    return true;
}

void VarAddr::updateRepr() { repr = addrToStr(addr); }

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(newSocket, 2, false,
           "  fn(domain, type) -> Socket\n"
           "Creates and returns a new non-blocking Socket with the given address `domain`\n"
           "(e.g. AF_INET) and socket `type` (e.g. SOCK_STREAM).")
{
    EXPECT(VarInt, args[1], "domain");
    EXPECT(VarInt, args[2], "type");
    int domain = (int)as<VarInt>(args[1])->getVal();
    int type   = (int)as<VarInt>(args[2])->getVal();
    int fd     = socket(domain, type, 0);
    if(fd < 0) {
        vm.fail(loc, "failed to create socket: ", sockErrStr());
        return nullptr;
    }
    if(!setNonBlocking(fd)) {
        sockClose(fd);
        vm.fail(loc, "failed to set socket non-blocking: ", sockErrStr());
        return nullptr;
    }
    return vm.makeVar<VarSocket>(loc, fd, domain, type);
}

FERAL_FUNC(socketClose, 0, false,
           "  var.fn() -> Nil\n"
           "Closes the Socket `var`.")
{
    as<VarSocket>(args[0])->closeSocket();
    return vm.getNil();
}

FERAL_FUNC(socketIsClosed, 0, false,
           "  var.fn() -> Bool\n"
           "Returns `true` if the Socket `var` has been closed.")
{
    return as<VarSocket>(args[0])->isClosed() ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(socketGetFd, 0, false,
           "  var.fn() -> Int\n"
           "Returns the underlying file descriptor of the Socket `var`.")
{
    return vm.makeVar<VarInt>(loc, as<VarSocket>(args[0])->getFd());
}

// Binds the socket to a local address. Pass an empty string for host to bind to all interfaces.
FERAL_FUNC(socketBind, 2, false,
           "  var.fn(host, port) -> Nil\n"
           "Binds the Socket `var` to `host`:`port`.\n"
           "Pass an empty string for `host` to bind to all interfaces.")
{
    EXPECT(VarStr, args[1], "host");
    EXPECT(VarInt, args[2], "port");
    VarSocket *sock      = as<VarSocket>(args[0]);
    const String &host   = as<VarStr>(args[1])->getVal();
    uint16_t port        = (uint16_t)as<VarInt>(args[2])->getVal();
    struct addrinfo *res = nullptr;
    if(!resolveAddr(vm, loc, host, port, sock->getDomain(), sock->getType(), &res)) return nullptr;
    int bindRes = bind(sock->getFd(), res->ai_addr, (socklen_t)res->ai_addrlen);
    freeaddrinfo(res);
    if(bindRes < 0) {
        vm.fail(loc, "bind failed: ", sockErrStr());
        return nullptr;
    }
    return vm.getNil();
}

FERAL_FUNC(socketListen, 1, false,
           "  var.fn(backlog) -> Nil\n"
           "Puts the Socket `var` into the listening state, with `backlog` pending connections.")
{
    EXPECT(VarInt, args[1], "backlog");
    VarSocket *sock = as<VarSocket>(args[0]);
    if(listen(sock->getFd(), (int)as<VarInt>(args[1])->getVal()) < 0) {
        vm.fail(loc, "listen failed: ", sockErrStr());
        return nullptr;
    }
    return vm.getNil();
}

// Returns nil when no connection is pending (WOULD_BLOCK), otherwise a new connected Socket.
FERAL_FUNC(socketAcceptNative, 0, false,
           "  var.fn() -> Socket | Nil\n"
           "Accepts a pending connection on the listening Socket `var`.\n"
           "Returns the new client Socket, or `nil` if no connection is pending yet.")
{
    VarSocket *sock = as<VarSocket>(args[0]);
    struct sockaddr_storage clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientFd            = accept(sock->getFd(), (struct sockaddr *)&clientAddr, &clientAddrLen);
    if(clientFd < 0) {
        if(sockWouldBlock()) return vm.getNil();
        vm.fail(loc, "accept failed: ", sockErrStr());
        return nullptr;
    }
    if(!setNonBlocking(clientFd)) {
        sockClose(clientFd);
        vm.fail(loc, "failed to set accepted socket non-blocking: ", sockErrStr());
        return nullptr;
    }
    return vm.makeVar<VarSocket>(loc, clientFd, sock->getDomain(), sock->getType());
}

// Initiates a non-blocking connect. Always returns nil — poll with connectReady() for completion.
FERAL_FUNC(socketConnect, 2, false,
           "  var.fn(host, port) -> Nil\n"
           "Initiates a non-blocking connection on Socket `var` to `host`:`port`.\n"
           "Use `connectReady()` to poll until the connection is established.\n"
           "NOTE: DNS resolution in this call may block briefly.")
{
    EXPECT(VarStr, args[1], "host");
    EXPECT(VarInt, args[2], "port");
    VarSocket *sock      = as<VarSocket>(args[0]);
    const String &host   = as<VarStr>(args[1])->getVal();
    uint16_t port        = (uint16_t)as<VarInt>(args[2])->getVal();
    struct addrinfo *res = nullptr;
    if(!resolveAddr(vm, loc, host, port, sock->getDomain(), sock->getType(), &res)) return nullptr;
    int connectRes = connect(sock->getFd(), res->ai_addr, (socklen_t)res->ai_addrlen);
    freeaddrinfo(res);
    if(connectRes < 0 && !sockInProgress()) {
        vm.fail(loc, "connect failed: ", sockErrStr());
        return nullptr;
    }
    return vm.getNil();
}

// Returns false if still connecting, true if connected. Raises on connection error.
FERAL_FUNC(socketConnectReady, 0, false,
           "  var.fn() -> Bool\n"
           "Returns `true` if the non-blocking connection on Socket `var` is complete.\n"
           "Returns `false` if the connection is still in progress.\n"
           "Raises an error if the connection failed.")
{
    VarSocket *sock = as<VarSocket>(args[0]);
    PollFd pfd      = {};
#if defined(FER_OS_WINDOWS)
    pfd.fd = (SOCKET)sock->getFd();
#else
    pfd.fd = sock->getFd();
#endif
    pfd.events = POLLOUT;
    int ready  = sockPoll(&pfd, 1, 0);
    if(ready < 0) {
        vm.fail(loc, "poll failed: ", sockErrStr());
        return nullptr;
    }
    if(ready == 0) return vm.getFalse();
    // Check whether the connection actually succeeded.
    int err       = 0;
    socklen_t len = sizeof(err);
    if(getsockopt(sock->getFd(), SOL_SOCKET, SO_ERROR, (char *)&err, &len) < 0) {
        vm.fail(loc, "getsockopt SO_ERROR failed: ", sockErrStr());
        return nullptr;
    }
    if(err != 0) {
        vm.fail(loc, "connect failed: ", strerror(err));
        return nullptr;
    }
    return vm.getTrue();
}

// Returns nil on WOULD_BLOCK, bytes sent otherwise.
FERAL_FUNC(socketSendNative, 1, false,
           "  var.fn(data) -> Int | Nil\n"
           "Sends `data` (Str or Bytebuffer) on the Socket `var`.\n"
           "Returns the number of bytes sent, or `nil` if the socket is not ready yet.")
{
    EXPECT2(VarStr, VarBytebuffer, args[1], "data to send");
    VarSocket *sock  = as<VarSocket>(args[0]);
    const void *data = nullptr;
    size_t dataLen   = 0;
    if(args[1]->is<VarStr>()) {
        VarStr *s = as<VarStr>(args[1]);
        data      = s->getVal().data();
        dataLen   = s->getVal().size();
    } else {
        VarBytebuffer *bb = as<VarBytebuffer>(args[1]);
        data              = bb->getVal();
        dataLen           = bb->size();
    }
    ssize_t sent = send(sock->getFd(), (const char *)data, dataLen, 0);
    if(sent < 0) {
        if(sockWouldBlock()) return vm.getNil();
        vm.fail(loc, "send failed: ", sockErrStr());
        return nullptr;
    }
    return vm.makeVar<VarInt>(loc, sent);
}

// Returns nil on WOULD_BLOCK, bytes received otherwise (0 = peer closed connection).
FERAL_FUNC(socketRecvNative, 1, false,
           "  var.fn(buf) -> Int | Nil\n"
           "Receives data from Socket `var` into the Bytebuffer `buf`.\n"
           "Returns bytes received, 0 if the peer closed the connection, or `nil` if not ready.")
{
    EXPECT(VarBytebuffer, args[1], "receive buffer");
    VarSocket *sock   = as<VarSocket>(args[0]);
    VarBytebuffer *bb = as<VarBytebuffer>(args[1]);
    ssize_t received  = recv(sock->getFd(), (char *)bb->getVal(), bb->capacity(), 0);
    if(received < 0) {
        if(sockWouldBlock()) return vm.getNil();
        vm.fail(loc, "recv failed: ", sockErrStr());
        return nullptr;
    }
    bb->setLen((size_t)received);
    return vm.makeVar<VarInt>(loc, received);
}

// UDP: send data to a specific address. Returns nil on WOULD_BLOCK, bytes sent otherwise.
FERAL_FUNC(socketSendToNative, 2, false,
           "  var.fn(addr, data) -> Int | Nil\n"
           "Sends `data` (Str or Bytebuffer) to `addr` on the UDP Socket `var`.\n"
           "Returns bytes sent, or `nil` if the socket is not ready yet.")
{
    EXPECT(VarAddr, args[1], "destination address");
    EXPECT2(VarStr, VarBytebuffer, args[2], "data to send");
    VarSocket *sock  = as<VarSocket>(args[0]);
    VarAddr *addr    = as<VarAddr>(args[1]);
    const void *data = nullptr;
    size_t dataLen   = 0;
    if(args[2]->is<VarStr>()) {
        VarStr *s = as<VarStr>(args[2]);
        data      = s->getVal().data();
        dataLen   = s->getVal().size();
    } else {
        VarBytebuffer *bb = as<VarBytebuffer>(args[2]);
        data              = bb->getVal();
        dataLen           = bb->size();
    }
    ssize_t sent = sendto(sock->getFd(), (const char *)data, dataLen, 0,
                          (const struct sockaddr *)&addr->getAddr(), addr->getAddrLen());
    if(sent < 0) {
        if(sockWouldBlock()) return vm.getNil();
        vm.fail(loc, "sendto failed: ", sockErrStr());
        return nullptr;
    }
    return vm.makeVar<VarInt>(loc, sent);
}

// UDP: receive data and fill `addrOut` with the sender's address.
// Returns nil on WOULD_BLOCK, bytes received otherwise.
FERAL_FUNC(socketRecvFromNative, 2, false,
           "  var.fn(addrOut, buf) -> Int | Nil\n"
           "Receives a datagram into `buf` on the UDP Socket `var`, writing the sender's address\n"
           "into `addrOut`.\n"
           "Returns bytes received, or `nil` if no datagram is available yet.")
{
    EXPECT(VarAddr, args[1], "address output");
    EXPECT(VarBytebuffer, args[2], "receive buffer");
    VarSocket *sock   = as<VarSocket>(args[0]);
    VarAddr *addrOut  = as<VarAddr>(args[1]);
    VarBytebuffer *bb = as<VarBytebuffer>(args[2]);
    struct sockaddr_storage senderAddr{};
    socklen_t senderLen = sizeof(senderAddr);
    ssize_t received    = recvfrom(sock->getFd(), (char *)bb->getVal(), bb->capacity(), 0,
                                   (struct sockaddr *)&senderAddr, &senderLen);
    if(received < 0) {
        if(sockWouldBlock()) return vm.getNil();
        vm.fail(loc, "recvfrom failed: ", sockErrStr());
        return nullptr;
    }
    bb->setLen((size_t)received);
    addrOut->getAddr()    = senderAddr;
    addrOut->getAddrLen() = senderLen;
    addrOut->updateRepr();
    return vm.makeVar<VarInt>(loc, received);
}

FERAL_FUNC(socketShutdown, 1, false,
           "  var.fn(how) -> Nil\n"
           "Shuts down part or all of the Socket `var` using `how`\n"
           "(SHUT_RD, SHUT_WR, or SHUT_RDWR).")
{
    EXPECT(VarInt, args[1], "shutdown mode");
    VarSocket *sock = as<VarSocket>(args[0]);
    if(shutdown(sock->getFd(), (int)as<VarInt>(args[1])->getVal()) < 0) {
        vm.fail(loc, "shutdown failed: ", sockErrStr());
        return nullptr;
    }
    return vm.getNil();
}

FERAL_FUNC(socketSetOpt, 3, false,
           "  var.fn(level, optName, val) -> Nil\n"
           "Sets socket option `optName` at `level` to integer value `val` on Socket `var`.\n"
           "Example: var.setOpt(SOL_SOCKET, SO_REUSEADDR, 1)")
{
    EXPECT(VarInt, args[1], "option level");
    EXPECT(VarInt, args[2], "option name");
    EXPECT(VarInt, args[3], "option value");
    VarSocket *sock = as<VarSocket>(args[0]);
    int val         = (int)as<VarInt>(args[3])->getVal();
    if(setsockopt(sock->getFd(), (int)as<VarInt>(args[1])->getVal(),
                  (int)as<VarInt>(args[2])->getVal(), (const char *)&val, sizeof(val)) < 0)
    {
        vm.fail(loc, "setsockopt failed: ", sockErrStr());
        return nullptr;
    }
    return vm.getNil();
}

FERAL_FUNC(socketGetPeerName, 0, false,
           "  var.fn() -> Str\n"
           "Returns the remote address of the connected Socket `var` as a \"host:port\" string.")
{
    VarSocket *sock = as<VarSocket>(args[0]);
    struct sockaddr_storage addr{};
    socklen_t addrLen = sizeof(addr);
    if(getpeername(sock->getFd(), (struct sockaddr *)&addr, &addrLen) < 0) {
        vm.fail(loc, "getpeername failed: ", sockErrStr());
        return nullptr;
    }
    return vm.makeVar<VarStr>(loc, addrToStr(addr));
}

FERAL_FUNC(socketGetSockName, 0, false,
           "  var.fn() -> Str\n"
           "Returns the local address of the Socket `var` as a \"host:port\" string.")
{
    VarSocket *sock = as<VarSocket>(args[0]);
    struct sockaddr_storage addr{};
    socklen_t addrLen = sizeof(addr);
    if(getsockname(sock->getFd(), (struct sockaddr *)&addr, &addrLen) < 0) {
        vm.fail(loc, "getsockname failed: ", sockErrStr());
        return nullptr;
    }
    return vm.makeVar<VarStr>(loc, addrToStr(addr));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarAddr ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(newAddr, 0, true,
           "  fn(host, port) -> Addr\n"
           "  fn() -> Addr\n"
           "Creates and returns an Addr by resolving `host` and `port`.\n"
           "Or, using second signature, returns an empty address.\n"
           "Useful for UDP sendTo targets.\n"
           "NOTE: performs DNS resolution, which may block briefly.")
{
    // no args provided
    if(args.size() == 1) return vm.makeVar<VarAddr>(loc);

    if(args.size() != 3) {
        vm.fail(loc, "expected 2 args (host & port), found: ", args.size() - 1);
        return nullptr;
    }
    EXPECT(VarStr, args[1], "host");
    EXPECT(VarInt, args[2], "port");
    const String &host   = as<VarStr>(args[1])->getVal();
    uint16_t port        = (uint16_t)as<VarInt>(args[2])->getVal();
    struct addrinfo *res = nullptr;
    if(!resolveAddr(vm, loc, host, port, AF_UNSPEC, 0, &res)) return nullptr;
    struct sockaddr_storage storage{};
    socklen_t len = (socklen_t)res->ai_addrlen;
    memcpy(&storage, res->ai_addr, len);
    freeaddrinfo(res);
    return vm.makeVar<VarAddr>(loc, storage, len);
}

FERAL_FUNC(addrStr, 0, false,
           "  var.fn() -> Str\n"
           "Returns the Addr `var` as a \"host:port\" string.")
{
    return vm.makeVar<VarStr>(loc, as<VarAddr>(args[0])->getRepr());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// INIT //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

INIT_DLL(Socket)
{
#if defined(FER_OS_WINDOWS)
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        vm.fail(loc, "WSAStartup failed");
        return false;
    }
#endif

    vm.addLocalType<VarSocket>(loc, "Socket", "A non-blocking TCP or UDP socket.");
    vm.addLocalType<VarAddr>(loc, "Addr", "A resolved network address (host and port).");

    // Module-level constructors
    vm.addLocal(loc, "new", newSocket);
    vm.addLocal(loc, "newAddr", newAddr);

    // Socket type methods
    vm.addTypeFn<VarSocket>(loc, "close", socketClose);
    vm.addTypeFn<VarSocket>(loc, "isClosed", socketIsClosed);
    vm.addTypeFn<VarSocket>(loc, "getFd", socketGetFd);
    vm.addTypeFn<VarSocket>(loc, "bind", socketBind);
    vm.addTypeFn<VarSocket>(loc, "listen", socketListen);
    vm.addTypeFn<VarSocket>(loc, "acceptNative", socketAcceptNative);
    vm.addTypeFn<VarSocket>(loc, "connect", socketConnect);
    vm.addTypeFn<VarSocket>(loc, "connectReady", socketConnectReady);
    vm.addTypeFn<VarSocket>(loc, "sendNative", socketSendNative);
    vm.addTypeFn<VarSocket>(loc, "recvNative", socketRecvNative);
    vm.addTypeFn<VarSocket>(loc, "sendToNative", socketSendToNative);
    vm.addTypeFn<VarSocket>(loc, "recvFromNative", socketRecvFromNative);
    vm.addTypeFn<VarSocket>(loc, "shutdown", socketShutdown);
    vm.addTypeFn<VarSocket>(loc, "setOpt", socketSetOpt);
    vm.addTypeFn<VarSocket>(loc, "getPeerName", socketGetPeerName);
    vm.addTypeFn<VarSocket>(loc, "getSockName", socketGetSockName);

    // Addr type methods
    vm.addTypeFn<VarAddr>(loc, "str", addrStr);

    // Address family
    vm.makeLocal<VarInt>(loc, "AF_INET", "IPv4 address family.", AF_INET);
    vm.makeLocal<VarInt>(loc, "AF_INET6", "IPv6 address family.", AF_INET6);
#if !defined(FER_OS_WINDOWS)
    vm.makeLocal<VarInt>(loc, "AF_UNIX", "Unix domain socket address family.", AF_UNIX);
#endif

    // Socket type
    vm.makeLocal<VarInt>(loc, "SOCK_STREAM", "Connection-oriented byte stream socket (TCP).",
                         SOCK_STREAM);
    vm.makeLocal<VarInt>(loc, "SOCK_DGRAM", "Connectionless datagram socket (UDP).", SOCK_DGRAM);

    // Shutdown modes
#if defined(FER_OS_WINDOWS)
    vm.makeLocal<VarInt>(loc, "SHUT_RD", "Shut down the receive half.", SD_RECEIVE);
    vm.makeLocal<VarInt>(loc, "SHUT_WR", "Shut down the send half.", SD_SEND);
    vm.makeLocal<VarInt>(loc, "SHUT_RDWR", "Shut down both halves.", SD_BOTH);
#else
    vm.makeLocal<VarInt>(loc, "SHUT_RD", "Shut down the receive half.", SHUT_RD);
    vm.makeLocal<VarInt>(loc, "SHUT_WR", "Shut down the send half.", SHUT_WR);
    vm.makeLocal<VarInt>(loc, "SHUT_RDWR", "Shut down both halves.", SHUT_RDWR);
#endif

    // Socket option levels
    vm.makeLocal<VarInt>(loc, "SOL_SOCKET", "Socket-level options.", SOL_SOCKET);
    vm.makeLocal<VarInt>(loc, "IPPROTO_TCP", "TCP protocol-level options.", IPPROTO_TCP);

    // Socket options
    vm.makeLocal<VarInt>(loc, "SO_REUSEADDR", "Allow reuse of a local address for bind.",
                         SO_REUSEADDR);
    vm.makeLocal<VarInt>(loc, "SO_KEEPALIVE", "Enable keep-alive probes on the connection.",
                         SO_KEEPALIVE);
    vm.makeLocal<VarInt>(loc, "SO_RCVBUF", "Size of the receive buffer.", SO_RCVBUF);
    vm.makeLocal<VarInt>(loc, "SO_SNDBUF", "Size of the send buffer.", SO_SNDBUF);
    vm.makeLocal<VarInt>(loc, "TCP_NODELAY", "Disable Nagle's algorithm (send immediately).",
                         TCP_NODELAY);
#if defined(FER_OS_LINUX)
    vm.makeLocal<VarInt>(loc, "SO_REUSEPORT", "Allow multiple sockets to bind the same port.",
                         SO_REUSEPORT);
#endif

    return true;
}

DEINIT_DLL(Socket)
{
#if defined(FER_OS_WINDOWS)
    WSACleanup();
#endif
}

} // namespace fer
