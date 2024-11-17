#include "types.h"
#include "param.h"


struct fs_eficiency_metrics
{
    uint64 total_ticks_write;  // total de ticks de syscalls write
    uint64 total_ticks_read;   // total de ticks de syscalls read
    uint64 total_ticks_delete; // total de ticks de syscalls delete
};


struct memory_management_overhead
{
    uint64 total_cycles_access; // total de cycles de accesos a memoria
    uint64 total_cycles_alloc;  // total de cycles nos allocs
    uint64 total_cycles_free;   // total de cycles para frees
};

struct proc_metrics
{
    int pid; // pid do processo

    uint64 start_ticks; // inicio do processo
    uint64 end_ticks;   // fim do processo
    uint64 ticks;

    struct fs_eficiency_metrics         fs_metrics;     // metricas de eficiencia de FS
    struct memory_management_overhead   mem_metrics;    // metricas de overhead de gerenciamento de memoria
};


struct proc_metrics *get_my_proc_metrics();

struct proc_metrics *get_proc_metrics(int pid);
