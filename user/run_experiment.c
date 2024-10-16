#include "kernel/types.h"
#include "kernel/stat.h"
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
#define HZ 100

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

void read_metrics(const char *file_path, struct proc_metrics *metrics)
{
    int file = open(file_path, O_RDONLY);
    if (file < 0)
    {
        printf("Could not open the file %s\n", file_path);
        close(file);
        exit(1);
    }

    if (read(file, metrics, sizeof(struct proc_metrics)) != sizeof(struct proc_metrics) && 1 == 0)
    {
        printf("Error reading from file %s\n", file_path);
        close(file);
        exit(1);
    }

    close(file);
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

void int_to_str(int num, char *str)
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
    uint64 max_lat = 0, min_lat = MAX_UINT64;
    uint64 total_lat = 0;

    for (int i = 0; i < size; i++)
    {
        if (metrics[i].io_metrics.total_ticks > max_lat)
            max_lat = metrics[i].io_metrics.total_ticks;
        if (metrics[i].io_metrics.total_ticks < min_lat)
            min_lat = metrics[i].io_metrics.total_ticks;
        total_lat += metrics[i].io_metrics.total_ticks;
    }
    total_lat *= FIXED_PRECISION;
    total_lat /= size;
    uint64 num = total_lat - min_lat * FIXED_PRECISION;
    uint64 dem = max_lat - min_lat;
    if (dem == 0)
        return FIXED_PRECISION;
    return FIXED_PRECISION - (num / dem);
}

uint64 throughput(struct proc_metrics *metrics, int size, int xticks)
{
    uint64 min_ticks = MAX_UINT64, max_ticks = 0;
    for (int i = 0; i < size; i++)
    {
        uint64 ticks = metrics[i].end_ticks - metrics[i].start_ticks;
        if (xticks > max_ticks)
            max_ticks = ticks;
        if (xticks < min_ticks)
            min_ticks = ticks;
    }
    uint64 num = FIXED_PRECISION * (xticks - min_ticks);
    uint64 dem = max_ticks - min_ticks;
    return FIXED_PRECISION - (num / dem);
}

uint64 proc_fairness(struct proc_metrics *metrics, int size)
{
    uint64 sx = 0, sx2 = 0;
    for (int i = 0; i < size; i++)
    {
        sx += metrics[i].ticks;
        sx2 += metrics[i].ticks * metrics[i].ticks;
    }

    sx *= FIXED_PRECISION * HZ;
    return sx / (size * sx2);
}

uint64 fs_efficiency(struct proc_metrics *metrics, int size)
{
    uint64 efs_min = MAX_UINT64, efs_max = 0;
    uint64 avg = 0;
    for (int i = 0; i < size; i++)
    {
        uint64 efs = metrics[i].fs_metrics.total_ticks_read + metrics[i].fs_metrics.total_ticks_write + metrics[i].fs_metrics.total_ticks_delete;
        if (efs > efs_max)
            efs_max = efs;
        if (efs < efs_min)
            efs_min = efs;
        avg += efs;
    }
    avg *= FIXED_PRECISION;
    avg /= size;
    uint64 num = avg - efs_min * FIXED_PRECISION;
    uint64 dem = efs_max - efs_min;
    return FIXED_PRECISION - (num / dem);
}

uint64 mem_overhead(struct proc_metrics *metrics, int size)
{
    uint64 mo_min = MAX_UINT64, mo_max = 0;
    uint64 avg = 0;
    for (int i = 0; i < size; i++)
    {
        uint64 mo = metrics[i].mem_metrics.total_ticks_access + metrics[i].mem_metrics.total_ticks_alloc + metrics[i].mem_metrics.total_ticks_free;
        if (mo > mo_max)
            mo_max = mo;
        if (mo < mo_min)
            mo_min = mo;
        avg += mo;
    }
    avg *= FIXED_PRECISION;
    avg /= size;
    return FIXED_PRECISION - (avg - mo_min * FIXED_PRECISION) / (mo_max - mo_min);
}

void compute_metrics(struct proc_metrics *raw_metrics, struct metrics *metrics, int size, int xticks)
{
    // printf("debug\n");
    metrics->io_latency = io_lat_norm(raw_metrics, size);
    metrics->throughput = throughput(raw_metrics, size, xticks);
    metrics->proc_fairness = proc_fairness(raw_metrics, size);
    metrics->fs_efficiency = fs_efficiency(raw_metrics, size);
    metrics->mem_overhead = mem_overhead(raw_metrics, size);
    metrics->overall = ((20 * metrics->io_latency) +
                        (30 * metrics->throughput) +
                        (25 * metrics->proc_fairness) +
                        (15 * metrics->fs_efficiency) +
                        (10 * metrics->mem_overhead));

    // printf("io_latency: %ld\n", metrics->io_latency);
    // printf("throughput: %ld\n", metrics->throughput);
    // printf("proc_fairness: %ld\n", metrics->proc_fairness);
    // printf("fs_efficiency: %ld\n", metrics->fs_efficiency);
    // printf("mem_overhead: %ld\n", metrics->mem_overhead);
    printf("overall: ");
    printf_fixed_precision(metrics->overall, 100 * FIXED_PRECISION);
    // printf("\n--------\n\n");
}

int main(int argc, char *argv[])
{
    struct metrics *metrics = malloc(sizeof(struct metrics) * ROUNDS);
    // printf("---------%ld\n", sizeof(metrics));
    struct proc_metrics raw_metrics[NUM_PROCS];
    int
        num_raw_metrics = 0,
        //  num_metrics = 0,
        seed = 0;
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE], filename[BUFFER_SIZE];
    int pipe_fd[NUM_PROCS][2];
    int xticks;

    for (int i = 0; i < ROUNDS; i++)
    {
        xticks = uptime();

        for (int j = 0; j < NUM_PROCS_IOBOUND; j++)
        {
            if (pipe(pipe_fd[j]) < 0)
            {
                printf("Could not create pipe for the iobound proc %d\n", j);
                exit(1);
            }

            memset(buffer1, 0, BUFFER_SIZE);
            memset(filename, 0, BUFFER_SIZE);

            int_to_str(i, buffer1);
            strcpy(filename, buffer1);
            strcat(filename, "_");

            int_to_str(j, buffer1);
            strcat(filename, buffer1);
            strcat(filename, "_IOBOUND");

            int_to_str(seed++, buffer1);
            int pid = fork();
            if (pid < 0)
                exit(1);

            if (pid == 0)
            {
                // printf("--------\n");
                close(pipe_fd[j][0]);
                int_to_str(pipe_fd[j][1], buffer2);
                // lines <seed> <filename> <pipe_fd>
                char *args[] = {"lines", buffer1, filename, buffer2, (char *)0};
                if (exec("lines", args) < 0)
                    exit(1);
            }
            else
            {
                close(pipe_fd[j][1]);
            }
        }

        for (int j = 0; j < NUM_PROCS_CPUBOUND; j++)
        {
            if (pipe(pipe_fd[j + NUM_PROCS_IOBOUND]) < 0)
            {
                printf("Could not create pipe for the cpubound proc %d\n", i + j);
                exit(1);
            }

            int_to_str(seed++, buffer1);
            int_to_str(pipe_fd[j + NUM_PROCS_IOBOUND][1], buffer2);

            int pid = fork();
            if (pid < 0)
                exit(1);

            if (pid == 0)
            {
                // printf("--------\n");
                close(pipe_fd[j + NUM_PROCS_IOBOUND][0]);
                // shortest_path <seed> <pipe_fd>
                char *args[] = {"shortest_path", buffer1, buffer2, (char *)0};
                if (exec("shortest_path", args) < 0)
                    exit(1);
            }
            else
            {
                close(pipe_fd[j + NUM_PROCS_IOBOUND][1]);
            }
        }
        // printf("dale dele dole\n");
        int count = 0;
        for (int k = 0; k < NUM_PROCS; k++)
        {
            int status;
            wait(&status);
            if (status != 0)
                exit(1);
            count++;
        }

        xticks = uptime() - xticks;

        printf("count: %d\n", count);
        sleep(10);

        for (int k = NUM_PROCS - 1; k >= 0; k--)
        {
            int fd = pipe_fd[k][0];
            // struct proc_metrics m;
            if (read(fd, &raw_metrics[k], sizeof(struct proc_metrics)) != sizeof(struct proc_metrics))
            {
                printf("Error reading from pipe\n");
                close(fd);
                exit(1);
            }
            close(fd);
            num_raw_metrics++;
        }
        printf("Round %d/%d\n", i + 1, ROUNDS);
        compute_metrics(raw_metrics, &metrics[i], NUM_PROCS, xticks);
    }

    exit(0);
}