#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/proc_metrics.h"

#define NUM_PROCS_CPUBOUND 4
#define NUM_PROCS_IOBOUND (4 - NUM_PROCS_CPUBOUND)
#define NUM_PROCS (NUM_PROCS_CPUBOUND + NUM_PROCS_IOBOUND)
#define ROUNDS 2
#define BUFFER_SIZE 50
#define FIXED_PRECISION 100000

struct metrics
{
    uint64 io_latency;
    uint64 throughput;
    uint64 proc_fairness;
    uint64 fs_efficiency;
    uint64 mem_overhead;
    uint64 overall;
};

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
    uint64 max_lat = 1, min_lat = 10000000;
    uint64 total_lat = 0;

    for (int i = 0; i < size; i++)
    {
        if (metrics[i].io_metrics.total_ticks > max_lat)
            max_lat = metrics[i].io_metrics.total_ticks;
        if (metrics[i].io_metrics.total_ticks < min_lat)
            min_lat = metrics[i].io_metrics.total_ticks;
        total_lat += metrics[i].io_metrics.total_ticks;
    }
    // return metrics[0].io_metrics.total_ticks;
    uint64 num = FIXED_PRECISION * (total_lat - min_lat);
    printf("--------------%ld %ld", num, max_lat - min_lat);
    return num / (max_lat - min_lat);
}

uint64 throughput(struct proc_metrics *metrics, int size)
{
    return 0;
}

uint64 proc_fairness(struct proc_metrics *metrics, int size)
{
    // unsigned long x_, x = 0, x2 = 0;
    // for (int i = 0; i < size; i++)
    // {
    //     x_ = metrics[i].end_ticks - metrics[i].start_ticks;
    //     x += x_;
    //     x2 += x_ * x_;
    // }

    // return (uint64)(x / (size * x2));
    return 0;
}

uint64 fs_efficiency(struct proc_metrics *metrics, int size)
{
    // metrics[0].fs_metrics.;
    return 0;
}

uint64 mem_overhead(struct proc_metrics *metrics, int size)
{
    return 0;
}

void compute_metrics(struct proc_metrics *raw_metrics, struct metrics *metrics, int size)
{
    printf("debug\n");
    metrics->io_latency = io_lat_norm(raw_metrics, size);
    metrics->throughput = throughput(raw_metrics, size);
    metrics->proc_fairness = proc_fairness(raw_metrics, size);
    metrics->fs_efficiency = fs_efficiency(raw_metrics, size);
    metrics->mem_overhead = mem_overhead(raw_metrics, size);
    metrics->overall = ((2 * metrics->io_latency) +
                        (3 * metrics->throughput) +
                        (25 * metrics->proc_fairness) +
                        (15 * metrics->fs_efficiency) +
                        (1 * metrics->mem_overhead));

    printf("io_latency: %ld\n", metrics->io_latency);
    printf("throughput: %ld\n", metrics->throughput);
    printf("proc_fairness: %ld\n", metrics->proc_fairness);
    printf("fs_efficiency: %ld\n", metrics->fs_efficiency);
    printf("mem_overhead: %ld\n", metrics->mem_overhead);
    printf("overall: %ld\n", metrics->overall);
    printf("\n--------\n\n");
}

int main(int argc, char *argv[])
{
    struct metrics metrics[ROUNDS];
    struct proc_metrics raw_metrics[NUM_PROCS];
    int num_raw_metrics = 0,
        //  num_metrics = 0,
        seed = 0;
    char buffer[BUFFER_SIZE], filename[BUFFER_SIZE], pool[NUM_PROCS][BUFFER_SIZE];

    // for (int i = 0; i < ROUNDS; i++)
    // {
    //     metrics[i].io_latency = 0.0;
    //     metrics[i].throughput = 0.0;
    //     metrics[i].proc_fairness = 0.0;
    //     metrics[i].fs_efficiency = 0.0;
    //     metrics[i].mem_overhead = 0.0;
    //     metrics[i].overall = 0.0;
    // }

    for (int i = 0; i < ROUNDS; i++)
    {
        for (int j = 0; j < NUM_PROCS; j++)
        {
            memset(pool[j], 0, BUFFER_SIZE);
        }

        for (int j = 0; j < NUM_PROCS_IOBOUND; j++)
        {
            memset(buffer, 0, BUFFER_SIZE);
            memset(filename, 0, BUFFER_SIZE);

            int_to_str(i, buffer);
            strcpy(filename, buffer);
            strcat(filename, "_");

            int_to_str(j, buffer);
            strcat(filename, buffer);
            strcat(filename, "_IOBOUND");

            strcpy(pool[j], filename);
            strcat(pool[j], ".metrics");

            int_to_str(seed++, buffer);
            int pid = fork();
            if (pid < 0)
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
            int_to_str(i, buffer);
            strcpy(filename, buffer);
            strcat(filename, "_");

            int_to_str(j, buffer);
            strcat(filename, buffer);
            strcat(filename, "_CPUBOUND");

            strcpy(pool[j + NUM_PROCS_IOBOUND], filename);
            strcat(pool[j + NUM_PROCS_IOBOUND], ".metrics");

            int_to_str(seed++, buffer);

            int pid = fork();
            if (pid < 0)
                exit(1);

            if (pid == 0)
            {
                // shortest_path <seed> <filename>
                char *args[] = {"shortest_path", buffer, filename, (char *)0};
                if (exec("shortest_path", args) < 0)
                    exit(1);
            }
        }
        int count = 0;
        for (int k = 0; k < NUM_PROCS; k++)
        {
            int status;
            wait(&status);
            if (status != 0)
            exit(1);
            count++;
        }
        printf("count: %d\n", count);
        sleep(10);


        for (int k = NUM_PROCS - 1; k >= 0; k--)
        {
            printf("pool[%d]: %s\n", k, pool[k]);
            read_metrics(pool[k], &raw_metrics[num_raw_metrics]);
            // uint64 b = 50;
            //  double a = (double)(b);
            // printf("%f\n", a);
            // printf("%f", 5.5);
            // printf("io_metrics.total_ticks: %ld\n", raw_metrics[num_raw_metrics].io_metrics.total_ticks);
            // printf("io_metrics.num_io_calls: %d\n", raw_metrics[num_raw_metrics].io_metrics.num_io_calls);
            // printf("fs_metrics.total_ticks_write: %ld\n", raw_metrics[num_raw_metrics].fs_metrics.total_ticks_write);
            // printf("fs_metrics.n_write: %d\n", raw_metrics[num_raw_metrics].fs_metrics.n_write);
            // printf("fs_metrics.total_ticks_read: %ld\n", raw_metrics[num_raw_metrics].fs_metrics.total_ticks_read);
            num_raw_metrics++;
        }
        // printf("degub---\n");
        // struct metrics m = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        // m.io_latency =
        // printf("io_latency: %f\n", m.io_latency);
        // io_lat_norm(raw_metrics, num_raw_metrics);
        compute_metrics(raw_metrics, &metrics[i], num_raw_metrics);
    }

    exit(0);
}