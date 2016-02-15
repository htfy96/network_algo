#ifndef DEBUG_HPP
#define DEBUG_HPP

#ifdef MYDEBUG
    #define HIDDEN hidden
    //#define protected public
    //#define private public
#else
    #define HIDDEN
#endif

#endif
