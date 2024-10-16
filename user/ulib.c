#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/proc_metrics.h"

// static unsigned long seed = 0;

//
// wrapper so that it's OK if main() does not call exit().
//
void
start()
{
  extern int main();
  main();
  exit(0);
}

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

char *
strcat(char *dest, const char *src)
{
  char *ptr = dest;
  while (*ptr != '\0')
  {
    ptr++;
  }

  while (*src != '\0')
  {
    *ptr = *src;
    ptr++;
    src++;
  }

  *ptr = '\0';

  return dest;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}



void*
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  if (src > dst) {
    while(n-- > 0)
      *dst++ = *src++;
  } else {
    dst += n;
    src += n;
    while(n-- > 0)
      *--dst = *--src;
  }
  return vdst;
}

int
memcmp(const void *s1, const void *s2, uint n)
{
  const char *p1 = s1, *p2 = s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

void *
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}


// void srand(unsigned long new_seed)
// {
//   seed = new_seed;
// }

// unsigned long get_seed()
// {
//   return seed;
// }


// int rand()
// {
//   long hi, lo, x;

//   /* Transform to [1, 0x7ffffffe] range. */
//   x = (seed % 0x7ffffffe) + 1;
//   hi = x / 127773;
//   lo = x % 127773;
//   x = 16807 * lo - 2836 * hi;
//   if (x < 0)
//     x += 0x7fffffff;
//   /* Transform to [0, 0x7ffffffd] range. */
//   x--;
//   seed = x;
//   return (x);
// }

// void save_metrics(char *save_path, struct proc_metrics *metrics)
// {
//   int file;

//   file = open(save_path, O_RDWR | O_CREATE);

//   if (file <= 0)
//   {
//     printf("Could not create the file %s\n", save_path);
//     exit(1);
//   }

//   if (write(file, metrics, sizeof(struct proc_metrics)) < sizeof(struct proc_metrics))
//   {
//     printf("Error on write\n");
//     close(file);
//     exit(1);
//   }

//   close(file);
// }

// void read_metrics(const char *file_path, struct proc_metrics *metrics)
// {
//   int file = open(file_path, O_RDONLY);
//   if (file < 0)
//   {
//     printf("Could not open the file");
//     exit(1);
//   }

//   if (read(file, metrics, sizeof(struct proc_metrics)) < 0)
//   {
//     printf("Error reading from file");
//     close(file);
//     exit(1);
//   }

//   close(file);
// }

// void int_to_str(int num, char *str)
// {
//   int i = 0, rem, len = 0, n;
//   n = num;

//   while (n != 0)
//   {
//     len++;
//     n /= 10;
//   }

//   for (i = 0; i < len; i++)
//   {
//     rem = num % 10;
//     num = num / 10;
//     str[len - (i + 1)] = rem + '0';
//   }

//   str[len] = '\0';
// }
