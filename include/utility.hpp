#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <cstddef>
#include <cctype>
#include <string>
#include <iostream>
template<typename T, int C>
size_t arrayLen(T (&)[C])
{
    return C;
}

std::string strLower(std::string s)
{
    for(char& c: s)
      c = std::tolower(c);
#ifdef MYDEBUG
    std::cout << s << std::endl;
#endif
    return s;
}
    
#endif
