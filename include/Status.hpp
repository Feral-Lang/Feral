#pragma once

#include "Utils.hpp"

namespace fer
{

template<typename T> class FER_API Status
{
    T ret;
    String msg;

public:
    // Allocates memory for string. If the message is a const char*, the other - non variadic -
    // constructor should be used which doesn't allocate memory for message.
    template<typename... Args> explicit Status(T &&ret, Args &&...msgArgs) : ret(std::move(ret))
    {
        utils::appendToString(msg, std::forward<Args>(msgArgs)...);
    }

    inline const T &getCode() { return ret; }
    inline StringRef getMsg() { return msg; }
};

} // namespace fer