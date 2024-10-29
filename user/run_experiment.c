#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/proc_metrics.h"

#define NUM_PROCS_CPUBOUND 10
#define NUM_PROCS_IOBOUND (20 - NUM_PROCS_CPUBOUND)
#define NUM_PROCS (NUM_PROCS_CPUBOUND + NUM_PROCS_IOBOUND)
#define ROUNDS 30
#define BUFFER_SIZE 50
#define FIXED_PRECISION 100000
#define MAX_UINT64 0xFFFFFFFFFFFFFFFF

struct metrics
{
    uint64 io_latency;
    uint64 throughput;
    uint64 proc_fairness;
    uint64 fs_efficiency;
    uint64 mem_overhead;
    uint64 overall;
};

void printf_fixed_precision(uint64 number, int precision)
{
    int integer = number / precision;
    int decimal = number % precision;
    printf("%d.%d\n", integer, decimal);
}

int contar_digitos(int num)
{
    int count = 0;
    if (num == 0)
        return 1; // O número '0' tem 1 dígito
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
    {
        str[index++] = '0'; // Lida com o caso de número 0.
    }
    else
    {
        int length = contar_digitos(num);
        index = length - 1;
        while (num != 0)
        {
            str[index--] = (num % 10) + '0';
            num /= 10;
        }
        index = length; // Ajusta para a próxima posição.
    }

    if (is_negative)
    {
        str[0] = '-';
        index++;
    }
    str[index] = '\0'; // Adiciona o terminador nulo.
}

uint64 io_lat_norm(struct proc_metrics *metrics, int size)
{
    uint64 max_lat = 0, min_lat = MAX_UINT64, tlat;
    for (int i = 0; i < size; i++)
    {
        tlat = metrics[i].io_metrics.total_ticks;
        if (tlat > max_lat)
            max_lat = tlat;
        if (tlat < min_lat)
            min_lat = tlat;
    }
    uint64 io_lat_norm_mean = 0;
    for (int i = 0; i < size; i++)
    {
        tlat = metrics[i].io_metrics.total_ticks;
        io_lat_norm_mean += (FIXED_PRECISION - ((tlat - min_lat) * FIXED_PRECISION) / (max_lat - min_lat)) / size;
    }
    return io_lat_norm_mean;
}

uint64 throughput(uint8 *puts_table, int size)
{
    uint64 max_puts = 0, min_puts = MAX_UINT64;

    if (!puts_table[0])
        min_puts = 0;

    int i = 1, total = 0;
    while (total < size)
    {
        if (puts_table[i])
        {
            if (i > max_puts)
                max_puts = i;
            if (i < min_puts)
                min_puts = i;
        }
        total += i * puts_table[i];
        i++;
    }
    uint64 throughput_mean = 0;
    for (int j = 1; j < i; j++)
    {
        throughput_mean += puts_table[j] * ((FIXED_PRECISION -
                                             ((j - min_puts) * FIXED_PRECISION) / (max_puts - min_puts)));
    }
    throughput_mean /= (size + puts_table[0]);
    return throughput_mean;
}

uint64 proc_fairness(struct proc_metrics *metrics, int size)
{
    uint64 sx = 0, sx2 = 0, x;
    for (int i = 0; i < size; i++)
    {
        x = metrics[i].end_ticks - metrics[i].start_ticks;
        sx += x;
        sx2 += x * x;
    }

    sx *= FIXED_PRECISION * sx;
    return sx / (size * sx2);
}

uint64 fs_efficiency(struct proc_metrics *metrics, int size)
{
    uint64 efs_min = MAX_UINT64, efs_max = 0, tefs;
    for (int i = 0; i < size; i++)
    {
        tefs = metrics[i].fs_metrics.total_ticks_read +
               metrics[i].fs_metrics.total_ticks_write +
               metrics[i].fs_metrics.total_ticks_delete;
        if (tefs > efs_max)
            efs_max = tefs;
        if (tefs < efs_min)
            efs_min = tefs;
    }
    uint64 fs_efficiency_mean = 0;
    for (int i = 0; i < size; i++)
    {
        tefs = metrics[i].fs_metrics.total_ticks_read +
               metrics[i].fs_metrics.total_ticks_write +
               metrics[i].fs_metrics.total_ticks_delete;
        fs_efficiency_mean += (FIXED_PRECISION -
                               ((tefs - efs_min) * FIXED_PRECISION) / (efs_max - efs_min)) /
                              size;
    }
    return fs_efficiency_mean;
}

uint64 mem_overhead(struct proc_metrics *metrics, int size)
{
    uint64 mo_min = MAX_UINT64, mo_max = 0, tmo;
    for (int i = 0; i < size; i++)
    {
        tmo = metrics[i].mem_metrics.total_ticks_access +
              metrics[i].mem_metrics.total_ticks_alloc +
              metrics[i].mem_metrics.total_ticks_free;
        if (tmo > mo_max)
            mo_max = tmo;
        if (tmo < mo_min)
            mo_min = tmo;
    }
    uint64 mem_overhead_mean = 0;
    for (int i = 0; i < size; i++)
    {
        tmo = metrics[i].mem_metrics.total_ticks_access +
              metrics[i].mem_metrics.total_ticks_alloc +
              metrics[i].mem_metrics.total_ticks_free;
        mem_overhead_mean += (FIXED_PRECISION -
                              ((tmo - mo_min) * FIXED_PRECISION) / (mo_max - mo_min)) /
                             size;
    }
    return mem_overhead_mean;
}

uint64 compute_metrics(struct proc_metrics *raw_metrics, int size, uint8 *puts_table, int verbose)
{
    struct metrics metrics;
    metrics.io_latency = io_lat_norm(raw_metrics, size);
    metrics.throughput = throughput(puts_table, size);
    metrics.proc_fairness = proc_fairness(raw_metrics, size);
    metrics.fs_efficiency = fs_efficiency(raw_metrics, size);
    metrics.mem_overhead = mem_overhead(raw_metrics, size);
    metrics.overall = ((20 * metrics.io_latency) +
                       (30 * metrics.throughput) +
                       (25 * metrics.proc_fairness) +
                       (15 * metrics.fs_efficiency) +
                       (10 * metrics.mem_overhead));

    if (verbose)
    {
        printf("io_latency: ");
        printf_fixed_precision(metrics.io_latency, 100 * FIXED_PRECISION);

        printf("throughput: ");
        printf_fixed_precision(metrics.throughput, 100 * FIXED_PRECISION);

        printf("proc_fairness: ");
        printf_fixed_precision(metrics.proc_fairness, 100 * FIXED_PRECISION);

        printf("fs_efficiency: ");
        printf_fixed_precision(metrics.fs_efficiency, 100 * FIXED_PRECISION);

        printf("mem_overhead: ");
        printf_fixed_precision(metrics.mem_overhead, 100 * FIXED_PRECISION);

        printf("overall: ");
        printf_fixed_precision(metrics.overall, 100 * FIXED_PRECISION);
    }
    return metrics.overall;
}
int main(int argc, char *argv[])
{

#ifdef OPTIMIZED
    printf("\nOptimized Version Experiment. Run with `make clean && make qemu` to see the non-optimized version.\n\n");
#else
    printf("\nNon-Optimized Version Experiment. Run with `make clean && make qemu-optimized` to see the optimized version.\n\n");
#endif

    struct proc_metrics raw_metrics[NUM_PROCS];
    uint8 puts_table[NPROC];
    uint64 perform;
    uint64 avg_perform = 0;
    int seed = 0;
    int pid;
    char buffer[BUFFER_SIZE], filename[BUFFER_SIZE];

    for (int i = 0; i < ROUNDS; i++)
    {
        observeprocputs();
        for (int j = 0; j < NUM_PROCS_IOBOUND; j++)
        {
            memset(buffer, 0, BUFFER_SIZE);
            memset(filename, 0, BUFFER_SIZE);

            itoa(i, buffer);
            strcpy(filename, buffer);
            strcat(filename, "_");

            itoa(j, buffer);
            strcat(filename, buffer);
            strcat(filename, "_IOBOUND");

            itoa(seed++, buffer);

            if ((pid = fork()) < 0)
                exit(1);

            if (pid == 0)
            {
                // lines <seed> <filename>
                char *args[] = {"lines", buffer, filename, (char *)0};
                if (exec("lines", args) < 0)
                    exit(1);
            }
        }

        for (int j = 0; j < NUM_PROCS_CPUBOUND; j++)
        {
            itoa(seed++, buffer);

            if ((pid = fork()) < 0)
                exit(1);

            if (pid == 0)
            {
                // shortest_path <seed>
                char *args[] = {"shortest_path", buffer, (char *)0};
                if (exec("shortest_path", args) < 0)
                    exit(1);
            }
        }

        for (int j = 0; j < NUM_PROCS; j++)
        {
            int status;
            if ((pid = waitandgetmetrics(&status, &raw_metrics[j])) < 0)
            {
                printf("Error on waitandgetmetrics\n");
                exit(1);
            }
        }

        getprocputs(puts_table, NUM_PROCS);

        perform = compute_metrics(raw_metrics, NUM_PROCS, puts_table, 0);
        avg_perform += perform / ROUNDS;
        if (i < 9)
            printf("# Round:  %d/%d        |      # System Performance: ", i + 1, ROUNDS);
        else
            printf("# Round: %d/%d        |      # System Performance: ", i + 1, ROUNDS);
        printf_fixed_precision(perform, 100 * FIXED_PRECISION);
    }

    printf("\n# Average System Performance: ");
    printf_fixed_precision(avg_perform, 100 * FIXED_PRECISION);
    printf("\n");

    exit(0);
}