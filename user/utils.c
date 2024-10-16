#include "kernel/proc_metrics.h"
#include "user/user.h"
#include "kernel/fcntl.h"

static unsigned long seed = 0;

// unsigned int
// rand()
// {
//     const unsigned int a = 1103515245;
//     const unsigned int c = 12345;
//     const unsigned int m = 0x80000000;

//     seed = (a * seed + c) % m;

//     return seed;
// }

void srand(unsigned long new_seed)
{
    seed = new_seed;
}

unsigned long get_seed()
{
    return seed;
}

// int do_rand(unsigned long *ctx)
// {
    /*
     * Compute x = (7^5 * x) mod (2^31 - 1)
     * without overflowing 31 bits:
     *      (2^31 - 1) = 127773 * (7^5) + 2836
     * From "Random number generators: good ones are hard to find",
     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
     * October 1988, p. 1195.
     */

// }

// unsigned long rand_next = 1;

int rand()
{
    long hi, lo, x;

    /* Transform to [1, 0x7ffffffe] range. */
    x = (seed % 0x7ffffffe) + 1;
    hi = x / 127773;
    lo = x % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    /* Transform to [0, 0x7ffffffd] range. */
    x--;
    seed = x;
    return (x);
}

void save_metrics(char *save_path, struct proc_metrics *metrics)
{
    int file;

    file = open(save_path, O_RDWR | O_CREATE);

    if (file <= 0)
    {
        printf("Could not create the file %s\n", save_path);
        exit(1);
    }

    if (write(file, metrics, sizeof(struct proc_metrics)) < sizeof(struct proc_metrics))
    {
        printf("Error on write\n");
        close(file);
        exit(1);
    }

    close(file);
}

void read_metrics(const char *file_path, struct proc_metrics *metrics)
{
    int file = open(file_path, O_RDONLY);
    if (file < 0)
    {
        printf("Could not open the file");
        exit(1);
    }

    if (read(file, metrics, sizeof(struct proc_metrics)) < 0)
    {
        printf("Error reading from file");
        close(file);
        exit(1);
    }

    close(file);
}

void int_to_str(int num, char *str)
{
    int i = 0, rem, len = 0, n;
    n = num;

    while (n != 0)
    {
        len++;
        n /= 10;
    }

    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }

    str[len] = '\0';
}
