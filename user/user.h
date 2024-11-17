struct stat;
struct proc_metrics;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int *);
int pipe(int *);
int write(int, const void *, int);
int read(int, void *, int);
int close(int);
int kill(int);
int exec(const char *, char **);
int open(const char *, int);
int mknod(const char *, short, short);
int unlink(const char *);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int mkdir(const char *);
int chdir(const char *);
int dup(int);
int getpid(void);
char *sbrk(int);
int sleep(int);
int uptime(void);

// syscalls que eu implementei:
int lseek(int fd, int offset, int whence);
int observeprocputs(void);
int getprocputs(uint8 *procputs, int size);
int waitandgetmetrics(int *status, struct proc_metrics *metrics);

// ulib.c
int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int strcmp(const char *, const char *);
void fprintf(int, const char *, ...) __attribute__((format(printf, 2, 3)));
void printf(const char *, ...) __attribute__((format(printf, 1, 2)));
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);
int atoi(const char *);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
char *strcat(char *dest, const char *src);

// void int_to_str(int num, char *str);
// void read_metrics(const char *file_path, struct proc_metrics *metrics);
// void save_metrics(char *save_path, struct proc_metrics *metrics);
// utils.c - minhas implementações
unsigned int rand_();
void srand_(unsigned long new_seed);

// umalloc.c
void *malloc(uint);
void free(void *);
