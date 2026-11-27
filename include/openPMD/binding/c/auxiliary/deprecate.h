#ifndef OPENPMD_DEPRECATE_H
#define OPENPMD_DEPRECATE_H

#ifndef _MSC_VER
#define OPENPMD_DEPRECATED __attribute__((deprecated))
#else
#define OPENPMD_DEPRECATED
#endif

#endif
