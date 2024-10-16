#include "types.h"

struct io_latency_metrics
{
    int num_io_calls;   // numero de syscalls de IO
    uint64 total_ticks; // total de ticks de syscalls de IO
};

struct fs_eficiency_metrics
{
    int n_write;  // numero de syscalls write
    int n_read;   // numero de syscalls read
    int n_delete; // numero de syscalls delete

    uint64 total_ticks_write;  // total de ticks de syscalls write
    uint64 total_ticks_read;   // total de ticks de syscalls read
    uint64 total_ticks_delete; // total de ticks de syscalls delete
};

struct memory_management_overhead
{
    int n_access; // numero de accesos a memoria
    int n_alloc;  // numero de allocs
    int n_free;   // numero de frees

    uint64 total_ticks_access; // total de ticks de accesos a memoria
    uint64 total_ticks_alloc;  // total de ticks nos allocs
    uint64 total_ticks_free;   // total de ticks para frees
};

struct proc_metrics
{
    int pid; // pid do processo

    uint64 start_ticks; // inicio do processo
    uint64 end_ticks;   // fim do processo
    uint64 ticks;

    struct io_latency_metrics io_metrics;          // metricas de IO
    struct fs_eficiency_metrics fs_metrics;        // metricas de eficiencia de FS
    struct memory_management_overhead mem_metrics; // metricas de overhead de gerenciamento de memoria
};

struct proc_metrics *get_my_proc_metrics();

struct proc_metrics *get_proc_metrics(int pid);
