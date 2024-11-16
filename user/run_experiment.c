#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/proc_metrics.h"

#define NUM_PROCS_CPUBOUND(x) (6 + x % 9)
#define NUM_PROCS_IOBOUND(x) (20 - NUM_PROCS_CPUBOUND(x))
#define NUM_PROCS 20
#define ROUNDS 30
#define BUFFER_SIZE 128
#define FIXED_PRECISION 100000
#define MAX_UINT64 0xFFFFFFFFFFFFFFFF

struct metrics
{
    uint64 throughput;
    uint64 proc_fairness;
    uint64 fs_efficiency;
    uint64 mem_overhead;
    uint64 performance;
};

uint64 throughput(uint8 *puts_table, int size);
uint64 proc_fairness(struct proc_metrics *metrics, int size);
uint64 fs_efficiency(struct proc_metrics *metrics, int size);
uint64 mem_overhead(struct proc_metrics *metrics, int size);
uint64 compute_metrics(struct metrics *metrics, struct proc_metrics *raw_metrics, int size, uint8 *puts_table, int verbose);

void create_io_bound_proc(int round, int id, int seed, char *buff1, char *buff2);
void create_cpu_bound_proc(int round, int id, int seed, char *buff);

// funções auxiliares
void printf_fixed_precision(uint64 number, int precision);
int count_digits(int num);
void itoa(int num, char *str);
void fixed_precision_to_string(uint64 number, int precision, char *str);
void save_metrics_to_csv(const char *filename, struct metrics *metrics, int size);


int main(int argc, char *argv[])
{

#ifdef OPTIMIZED
    printf("\nOptimized Version Experiment. Run with `make clean && make qemu-original` to see the non-optimized version.\n\n");
#else
    printf("\nNon-Optimized Version Experiment. Run with `make clean && make qemu` to see the optimized version.\n\n");
#endif

    struct proc_metrics raw_metrics[NUM_PROCS];
    struct metrics *metrics = malloc(ROUNDS * sizeof(struct metrics));
    uint8 puts_table[NPROC];
    uint64 perform;
    uint64 avg_perform = 0;
    int seed_ = 0, seed = 0;
    int pid;
    char buffer[BUFFER_SIZE], filename[BUFFER_SIZE];

    for (int i = 0; i < ROUNDS; i++)
    {   
        srand_(seed_);
        uint x = rand_();
        // printf("num_procs_iobound: %d   num_procs_cpubound: %d      x: %d   seed: %d\n", NUM_PROCS_IOBOUND(x), NUM_PROCS_CPUBOUND(x), x, seed);
        observeprocputs();
        for (int j = 0; j < NUM_PROCS_IOBOUND(x); j++)
            create_io_bound_proc(i, j, seed++, buffer, filename);

        for (int j = 0; j < NUM_PROCS_CPUBOUND(x); j++)
            create_cpu_bound_proc(i, j, seed++, buffer);

        for (int j = 0; j < NUM_PROCS; j++) {
            int status;
            if ((pid = waitandgetmetrics(&status, &raw_metrics[j])) < 0)
            {
                printf("Error on waitandgetmetrics\n");
                exit(1);
            }
        }

        getprocputs(puts_table, NUM_PROCS);

        perform = compute_metrics(&metrics[i], raw_metrics, NUM_PROCS, puts_table, 1);
        avg_perform += perform / ROUNDS;
        if (i < 9)
            printf("# Round:  %d/%d        |      # System Performance: ", i + 1, ROUNDS);
        else
            printf("# Round: %d/%d        |      # System Performance: ", i + 1, ROUNDS);
        printf_fixed_precision(perform, FIXED_PRECISION);
    }

    printf("\n# Average System Performance: ");
    printf_fixed_precision(avg_perform, FIXED_PRECISION);
    printf("\n");

#ifdef OPTIMIZED
    save_metrics_to_csv("optimized_metrics.csv", metrics, ROUNDS);
#else
    save_metrics_to_csv("non_optimized_metrics.csv", metrics, ROUNDS);
#endif
    free(metrics);
    exit(0);
}

uint64 throughput(uint8 *puts_table, int size)
{
    uint64 max_puts = 0, min_puts = MAX_UINT64;

    if (puts_table[0] != 0)
        min_puts = 0;

    int i = 1, total = 0, sum = puts_table[0];
    while (total < size)
    {
        if (puts_table[i])
        {
            if (i > max_puts)
                max_puts = i;
            if (i < min_puts)
                min_puts = i;
            sum += puts_table[i];
        }
        total += i * puts_table[i];
        i++;
    }
    uint64 throughput_mean = (FIXED_PRECISION * NUM_PROCS) / sum;
    return ((throughput_mean - min_puts * FIXED_PRECISION) / (max_puts - min_puts));
}

uint64 proc_fairness(struct proc_metrics *metrics, int size)
{
    uint64 s2x = 0, sx2 = 0, x;
    for (int i = 0; i < size; i++)
    {
        // printf("ticks_: %ld     %d\n", metrics[i].ticks, metrics[i].fs_metrics.n_write);
        // x = metrics[i].ticks;
        x = metrics[i].end_ticks - metrics[i].start_ticks;
        s2x += x;
        sx2 += x * x;
    }

    s2x *= FIXED_PRECISION * s2x;
    return s2x / (size * sx2);
}

uint64 fs_efficiency(struct proc_metrics *metrics, int size)
{
    uint64 total_read = 0, total_write = 0, total_delete = 0;
    for (int i = 0; i < size; i++)
    {
        total_read += metrics[i].fs_metrics.total_ticks_read;
        total_write += metrics[i].fs_metrics.total_ticks_write;
        total_delete += metrics[i].fs_metrics.total_ticks_delete;
    }
    int factor = 10;
    return ((FIXED_PRECISION * factor) / (total_read + total_write + total_delete));
}

uint64 mem_overhead(struct proc_metrics *metrics, int size)
{
    uint64 total_access = 0, total_alloc = 0, total_free = 0;
    for (int i = 0; i < size; i++)
    {
        total_access += metrics[i].mem_metrics.total_cycles_access;
        total_alloc += metrics[i].mem_metrics.total_cycles_alloc;
        total_free += metrics[i].mem_metrics.total_cycles_free;
    }
    uint64 factor = 1000000;
    return ((FIXED_PRECISION * factor) / (total_access + total_alloc + total_free));
}

uint64 compute_metrics(struct metrics *metrics,
                       struct proc_metrics *raw_metrics,
                       int size,
                       uint8 *puts_table,
                       int verbose)
{
    metrics->throughput = throughput(puts_table, size);
    metrics->proc_fairness = proc_fairness(raw_metrics, size);
    metrics->fs_efficiency = fs_efficiency(raw_metrics, size);
    metrics->mem_overhead = mem_overhead(raw_metrics, size);
    metrics->performance = ((25 * metrics->throughput) +
                            (25 * metrics->proc_fairness) +
                            (25 * metrics->fs_efficiency) +
                            (25 * metrics->mem_overhead)) /
                           100;

    if (verbose)
    {
        printf("throughput: ");
        printf_fixed_precision(metrics->throughput, FIXED_PRECISION);

        printf("proc_fairness: ");
        printf_fixed_precision(metrics->proc_fairness, FIXED_PRECISION);

        printf("fs_efficiency: ");
        printf_fixed_precision(metrics->fs_efficiency, FIXED_PRECISION);

        printf("mem_overhead: ");
        printf_fixed_precision(metrics->mem_overhead, FIXED_PRECISION);

        printf("performance: ");
        printf_fixed_precision(metrics->performance, FIXED_PRECISION);
    }
    return metrics->performance;
}

void create_io_bound_proc(int round, int id, int seed, char *buff1, char *buff2)
{
    int pid;
    memset(buff1, 0, BUFFER_SIZE);
    memset(buff2, 0, BUFFER_SIZE);

    itoa(round, buff1);
    strcpy(buff2, buff1);
    strcat(buff2, "_");

    itoa(id, buff1);
    strcat(buff2, buff1);
    strcat(buff2, "_IOBOUND");

    itoa(seed, buff1);

    if ((pid = fork()) < 0)
        exit(1);

    if (pid == 0)
    {
        // lines <seed> <filename>
        char *args[] = {"lines", buff1, buff2, (char *)0};
        if (exec("lines", args) < 0)
            exit(1);
    }
}

void create_cpu_bound_proc(int round, int id, int seed, char *buff)
{
    int pid;
    itoa(seed, buff);

    if ((pid = fork()) < 0)
        exit(1);

    if (pid == 0)
    {
        // shortest_path <seed>
        char *args[] = {"shortest_path", buff, (char *)0};
        if (exec("shortest_path", args) < 0)
            exit(1);
    }
}

void printf_fixed_precision(uint64 number, int precision)
{
    char buffer[BUFFER_SIZE];
    fixed_precision_to_string(number, precision, buffer);
    printf("%s\n", buffer);
}

int count_digits(int num)
{
    int count = 0;
    if (num == 0)
        return 1;
    while (num != 0)
    {
        count++;
        num /= 10;
    }
    return count;
}

void itoa(int num, char *str)
{
    int is_negative = 0;
    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    int index = 0;
    if (num == 0)
        str[index++] = '0';
    else
    {
        int length = count_digits(num);
        index = length - 1;
        while (num != 0)
        {
            str[index--] = (num % 10) + '0';
            num /= 10;
        }
        index = length;
    }

    if (is_negative)
    {
        str[0] = '-';
        index++;
    }
    str[index] = '\0';
}

void fixed_precision_to_string(uint64 number, int precision, char *str)
{
    int integer = number / precision;
    int decimal = number % precision;

    int len = 0;
    int temp_integer = integer;
    char temp_str[20];
    do
    {
        temp_str[len++] = '0' + (temp_integer % 10);
        temp_integer /= 10;
    } while (temp_integer > 0);

    for (int i = 0; i < len; i++)
        str[i] = temp_str[len - i - 1];

    str[len++] = '.';

    int decimal_places = 0;
    int temp_precision = precision;
    while (temp_precision > 1)
    {
        temp_precision /= 10;
        decimal_places++;
    }

    int factor = 1;
    for (int i = 1; i < decimal_places; i++)
    {
        factor *= 10;
        if (decimal < factor)
            str[len++] = '0';
    }

    temp_integer = decimal;
    int decimal_len = 0;
    do
    {
        temp_str[decimal_len++] = '0' + (temp_integer % 10);
        temp_integer /= 10;
    } while (temp_integer > 0);

    for (int i = 0; i < decimal_len; i++)
        str[len + decimal_len - i - 1] = temp_str[i];
    len += decimal_len;
    str[len] = '\0';
}

void save_metrics_to_csv(const char *filename, struct metrics *metrics, int size)
{
    int fd = open(filename, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd < 0)
    {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    write(fd, "throughput,proc_fairness,fs_efficiency,mem_overhead,performance\n", 64);

    char buffer[BUFFER_SIZE];

    for (int i = 0; i < size; ++i)
    {
        int len = 0;

        fixed_precision_to_string(metrics[i].throughput, FIXED_PRECISION, buffer);
        len += strlen(buffer);
        buffer[len++] = ',';

        fixed_precision_to_string(metrics[i].proc_fairness, FIXED_PRECISION, buffer + len);
        len += strlen(buffer + len);
        buffer[len++] = ',';

        fixed_precision_to_string(metrics[i].fs_efficiency, FIXED_PRECISION, buffer + len);
        len += strlen(buffer + len);
        buffer[len++] = ',';

        fixed_precision_to_string(metrics[i].mem_overhead, FIXED_PRECISION, buffer + len);
        len += strlen(buffer + len);
        buffer[len++] = ',';

        fixed_precision_to_string(metrics[i].performance, FIXED_PRECISION, buffer + len);
        len += strlen(buffer + len);
        buffer[len++] = '\n';

        write(fd, buffer, len);
    }

    close(fd);
}
