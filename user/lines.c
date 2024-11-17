#include "kernel/proc_metrics.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define LINE_SIZE 101
#define MAX_LINES 100

char buffer[LINE_SIZE];

void generate_random_line()
{
    for (int i = 0; i < LINE_SIZE - 1; i++)
    {
        char c = 32 + rand_() % 95;
        buffer[i] = c;
    }
    buffer[LINE_SIZE - 1] = '\n';
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

    offset1 = rand_() % MAX_LINES;

    do
        offset2 = rand_() % MAX_LINES;
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
    if (argc != 3)
    {
        printf("Invalid params\n");
        exit(1);
    }

    int file, seed;

    seed = atoi(argv[1]);
    srand_(seed);

    file = create_random_file(argv[2]);

    for (int i = 0; i < 50; i++)
        swap_lines(file);

    close(file);
    unlink(argv[2]);

    exit(0);
}