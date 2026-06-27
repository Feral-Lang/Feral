#pragma once

#include "Status.hpp"

namespace fer
{

template<typename T, typename E> class FER_API Result
{
    Variant<T, Status<E>> data;

public:
    // okay value
    Result(T &&obj) : data(std::move(obj)) {}
    // err value
    template<typename... Args>
    explicit Result(E &&ec, Args &&...msgArgs) : data(Status<E>(ec, std::forward<Args>(msgArgs)...))
    {}
    Result(Status<E> &&obj) : data(std::move(obj)) {}

    inline T &&val() { return std::get<T>(std::move(data)); }
    inline T &valRef() { return std::get<T>(data); }

    inline Status<E> &&err() { return std::get<Status<E>>(std::move(data)); }
    inline Status<E> &errRef() { return std::get<Status<E>>(data); }

    inline bool isErr() const { return std::holds_alternative<Status<E>>(data); }
    inline bool isOk() const { return !isErr(); }
};

} // namespace fer