#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static unsigned long seed = 0;

unsigned int
rand_()
{
    long hi, lo, x;
    x = (seed % 0x7ffffffe) + 1;
    hi = x / 127773;
    lo = x % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    x--;
    seed = x;
    return seed % 0x7fffffff;
}

void srand_(unsigned long new_seed)
{
    seed = new_seed;
}
