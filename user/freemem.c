#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    int before = freemem();
    printf("free: %d bytes (%d pages)\n", before, before / 4096);

    sbrk(10 * 4096);
    int after = freemem();
    printf("after sbrk of 10 pages: %d bytes, used %d pages\n", after, (before - after) / 4096);
    exit(0);
}