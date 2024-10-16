#include "kernel/proc_metrics.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define LINE_SIZE 101
#define MAX_LINES 100

char buffer[LINE_SIZE];

static unsigned long seed = 0;

unsigned int
rand()
{
    const unsigned int a = 1103515245;
    const unsigned int c = 12345;
    const unsigned int m = 0x80000000;

    seed = (a * seed + c) % m;

    return seed;
}

void srand(unsigned long new_seed)
{
    seed = new_seed;
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

    read_metrics(save_path, metrics);
}

void generate_random_line()
{
    for (int i = 0; i < LINE_SIZE - 2; i++)
    {
        char c = 32 + rand() % 95;
        buffer[i] = c;
    }
    buffer[LINE_SIZE - 2] = '\n'; // quebra de linha
    buffer[LINE_SIZE - 1] = '\0'; // fim da string
}

int create_random_file(char *save_dir)
{
    int file;

    file = open(save_dir, O_RDWR | O_CREATE);

    if (file <= 0)
    {
        printf("Could not create the file %s\n", save_dir);
        exit(1);
    }

    for (int i = 0; i < MAX_LINES; i++)
    {
        generate_random_line();
        write(file, buffer, LINE_SIZE);
    }
    return file;
}

void swap_lines(int file)
{
    int offset1, offset2, temp;
    char buffer1[LINE_SIZE], buffer2[LINE_SIZE];

    offset1 = rand() % MAX_LINES;

    do
        offset2 = rand() % MAX_LINES;
    while (offset1 == offset2);

    offset1 *= (LINE_SIZE);
    offset2 *= (LINE_SIZE);

    if ((temp = lseek(file, offset1, SEEK_SET)) != offset1)
    {
        printf("Error on lseek. Expected: %d, Returned: %d\n", offset1, temp);
        exit(1);
    }

    if ((temp = read(file, buffer1, LINE_SIZE)) != LINE_SIZE)
    {
        printf("Error on read\n. Expected: %d, Returned: %d\n", LINE_SIZE, temp);
        exit(1);
    }

    if ((temp = lseek(file, offset2, SEEK_SET)) != offset2)
    {
        printf("Error on lseek\n. Expected: %d, Returned: %d\n", offset2, temp);
        exit(1);
    }

    if ((temp = read(file, buffer2, LINE_SIZE)) != LINE_SIZE)
    {
        printf("Error on read\n. Expected: %d, Returned: %d\n", LINE_SIZE, temp);
        exit(1);
    }

    if ((temp = lseek(file, offset1, SEEK_SET)) != offset1)
    {
        printf("Error on lseek\n. Expected: %d, Returned: %d\n", offset1, temp);
        exit(1);
    }

    if ((temp = write(file, buffer2, LINE_SIZE)) != LINE_SIZE)
    {
        printf("Error on write\n. Expected: %d, Returned: %d\n", LINE_SIZE, temp);
        exit(1);
    }

    if ((temp = lseek(file, offset2, SEEK_SET)) != offset2)
    {
        printf("Error on lseek\n. Expected: %d, Returned: %d\n", offset2, temp);
        exit(1);
    }

    if ((temp = write(file, buffer1, LINE_SIZE)) != LINE_SIZE)
    {
        printf("Error on write\n. Expected: %d, Returned: %d\n", LINE_SIZE, temp);
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Invalid params\n");
        exit(1);
    }

    int file, seed;
    int pipe_fd = atoi(argv[3]);

    seed = atoi(argv[1]);
    srand(seed);

    file = create_random_file(argv[2]);

    for (int i = 0; i < 50; i++)
        swap_lines(file);

    close(file);

    unlink(argv[2]);

    struct proc_metrics proc_metrics;
    if (getprocmetrics(&proc_metrics) < 0)
    {
        printf("Error on getprocmetrics\n");
        exit(1);
    }

    // proc_metrics.end_ticks = uptime();
    // save_metrics(strcat(argv[2], ".metrics"), &proc_metrics);
    if (pipe_fd != -1)
        if (write(pipe_fd, &proc_metrics, sizeof(struct proc_metrics)) != sizeof(struct proc_metrics))
        {
            printf("Error writing to pipe\n");
            close(pipe_fd);
            exit(1);
        }
    close(pipe_fd);
    exit(0);
}