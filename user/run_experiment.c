#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/proc_metrics.h"

#define NUM_PROCS_CPUBOUND 10
#define NUM_PROCS_IOBOUND (20 - NUM_PROCS_CPUBOUND)
#define NUM_PROCS (NUM_PROCS_CPUBOUND + NUM_PROCS_IOBOUND)
#define ROUNDS 3
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

void printf_fixed_precision(uint64 number, int precision)
{
    int integer = number / precision;
    int decimal = number % precision;

    int decimal_places = 0;
    int temp_precision = precision;
    while (temp_precision > 1)
    {
        temp_precision /= 10;
        decimal_places++;
    }

    printf("%d.", integer);

    int factor = 1;
    for (int i = 1; i < decimal_places; i++)
    {
        factor *= 10;
        if (decimal < factor)
        {
            printf("0");
        }
    }

    if (decimal > 0)
    {
        printf("%d", decimal);
    }
    else
    {
        printf("0");
    }

    printf("\n");
}

int contar_digitos(int num)
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
    {
        str[index++] = '0';
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

    // Converte a parte inteira para string manualmente
    int len = 0;
    int temp_integer = integer;
    char temp_str[20]; // Armazena a parte inteira temporariamente para manipulação reversa

    // Converte cada dígito da parte inteira para caracteres em ordem reversa
    do
    {
        temp_str[len++] = '0' + (temp_integer % 10);
        temp_integer /= 10;
    } while (temp_integer > 0);

    // Copia a parte inteira para o buffer final `str` na ordem correta
    for (int i = 0; i < len; i++)
    {
        str[i] = temp_str[len - i - 1];
    }

    // Adiciona o ponto decimal
    str[len++] = '.';

    // Calcula o número de dígitos necessários para a parte decimal
    int decimal_places = 0;
    int temp_precision = precision;
    while (temp_precision > 1)
    {
        temp_precision /= 10;
        decimal_places++;
    }

    // Adiciona zeros à esquerda na parte decimal, se necessário
    int factor = 1;
    for (int i = 1; i < decimal_places; i++)
    {
        factor *= 10;
        if (decimal < factor)
        {
            str[len++] = '0';
        }
    }

    // Converte a parte decimal para string manualmente
    temp_integer = decimal;
    int decimal_len = 0;
    do
    {
        temp_str[decimal_len++] = '0' + (temp_integer % 10);
        temp_integer /= 10;
    } while (temp_integer > 0);

    // Adiciona a parte decimal na ordem correta
    for (int i = 0; i < decimal_len; i++)
    {
        str[len + decimal_len - i - 1] = temp_str[i];
    }
    len += decimal_len;

    // Finaliza a string com o terminador nulo
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
    printf("mean: %ld   min: %ld   max: %ld\n", throughput_mean, min_puts, max_puts);
    return (
            // FIXED_PRECISION -
           (throughput_mean - min_puts * FIXED_PRECISION) / (max_puts - min_puts));
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
    // uint64 efs_min = MAX_UINT64, efs_max = 0, tefs, fs_eff_mean = 0;
    // for (int i = 0; i < size; i++)
    // {
    //     tefs = metrics[i].fs_metrics.total_ticks_read +
    //            metrics[i].fs_metrics.total_ticks_write +
    //            metrics[i].fs_metrics.total_ticks_delete;
    //     if (tefs > efs_max)
    //         efs_max = tefs;
    //     if (tefs < efs_min)
    //         efs_min = tefs;
    //     fs_eff_mean += (FIXED_PRECISION * tefs) / size;
    // }
    // return FIXED_PRECISION - (fs_eff_mean - efs_min * FIXED_PRECISION) / (efs_max - efs_min);
    uint64 total_read = 0, total_write = 0, total_delete = 0;
    for (int i = 0; i < size; i++)
    {
        // printf("pid:    %d      start_tick: %ld\n", metrics[i].pid, metrics[i].start_ticks);
        total_read += metrics[i].fs_metrics.total_ticks_read;
        total_write += metrics[i].fs_metrics.total_ticks_write;
        total_delete += metrics[i].fs_metrics.total_ticks_delete;
    }
    uint64 a = 10;
    total_delete /= a;
    total_read /= a;
    total_write /= a;
    printf("total_read: %ld\n", total_read);
    printf("total_write: %ld\n", total_write);
    printf("total_delete: %ld\n", total_delete);
    return ((FIXED_PRECISION) / (total_read + total_write + total_delete + 1));
}

uint64 mem_overhead(struct proc_metrics *metrics, int size)
{
    // uint64 mo_min = MAX_UINT64, mo_max = 0, tmo, mem_overhead_mean = 0;
    // for (int i = 0; i < size; i++)
    // {
    //     tmo = metrics[i].mem_metrics.total_ticks_access +
    //           metrics[i].mem_metrics.total_ticks_alloc +
    //           metrics[i].mem_metrics.total_ticks_free;
    //     if (tmo > mo_max)
    //         mo_max = tmo;
    //     if (tmo < mo_min)
    //         mo_min = tmo;
    //     mem_overhead_mean += (FIXED_PRECISION * tmo) / size;
    // }
    // return (FIXED_PRECISION -
    //        (mem_overhead_mean - FIXED_PRECISION * mo_min) / (mo_max - mo_min));
    uint64 total_access = 0, total_alloc = 0, total_free = 0;
    for (int i = 0; i < size; i++)
    {   
        total_access += metrics[i].mem_metrics.total_cycles_access;
        total_alloc += metrics[i].mem_metrics.total_cycles_alloc;
        total_free += metrics[i].mem_metrics.total_cycles_free;
        // printf("access: %d     alloc: %d     free: %d", metrics[i].mem_metrics.n_access, metrics[i].mem_metrics.n_alloc, metrics[i].mem_metrics.n_free);
        // printf("     access_ticks: %ld     alloc_ticks: %ld     free_ticks: %ld\n", metrics[i].mem_metrics.total_ticks_access, metrics[i].mem_metrics.total_ticks_alloc, metrics[i].mem_metrics.total_ticks_free);
    }
    uint64 factor = 1000000;
    printf("dem mem: %ld\n", (total_access + total_alloc + total_free));
    return ((FIXED_PRECISION * factor) / (total_access + total_alloc + total_free));
}

uint64 compute_metrics(struct metrics* metrics,
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
                       (25 * metrics->mem_overhead)) / 100;

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