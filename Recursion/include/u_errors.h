#ifndef U_ERRORS_H
#define U_ERRORS_H

#include "u_types.h"

enum e_retcode
{
    RET_ERRCODE_OK              = (0),
    RET_ERRCODE_NG_ARGNULL      = (-1),
    RET_ERRCODE_NG_PARAM        = (-2),
    RET_ERRCODE_NG_SYSTEM       = (-3),
    RET_ERRCODE_NG_DUPLICATE    = (-4)
};

#endif