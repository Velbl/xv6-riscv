#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    pgdump();
    sbrk(1);
    pgdump();

    exit(0);
}