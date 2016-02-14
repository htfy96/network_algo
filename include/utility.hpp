#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <cstddef>
template<typename T, int C>
size_t arrayLen(T (&)[C])
{
    return C;
}
#endif
