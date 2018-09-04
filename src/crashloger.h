#ifndef CRASHLOGER_H
#define CRASHLOGER_H

#include <qglobal.h>

#ifndef PRIxPTR
#    if __WORDSIZE == 64
#        define PRIxPTR "lx"
#    else
#        define PRIxPTR "x"
#    endif
#endif

#endif  // CRASHLOGER_H
