#ifndef GRAPH_UTILITY_HPP
#define GRAPH_UTILITY_HPP
#include <cstddef>
#include <cctype>
#include <cassert>
#include <string>
#include <utility>
#include <memory>

namespace
{
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
        //std::cout << s << std::endl;
#endif
        return s;
    }

}
#endif
