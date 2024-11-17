//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      f->cache_size = 0;
      f->cache_page_size = 0;
      f->cache_page_count = 0;
      f->cached = 0;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}

int filecachedread(struct file *f, uint64 addr, int n)
{
  if (f->cached == 0 || f->cache_page_count == 0)
    return -1;

  int i = 0;
  while (i < n)
  {
    int target_block = f->off / PGSIZE;

    if (target_block >= f->cache_page_count)
      return -1;

    int remaining_bytes_on_block = (target_block + 1) * PGSIZE - f->off;
    int bytes_to_read = n - i;

    if (bytes_to_read > remaining_bytes_on_block)
      bytes_to_read = remaining_bytes_on_block;

    if (copyout(myproc()->pagetable, addr + i,
                f->cache[target_block] + (f->off % PGSIZE), bytes_to_read) < 0)
      return -1;

    f->off += bytes_to_read;
    i += bytes_to_read;
  }

  return i;
}

int filecachedwrite(struct file *f, uint64 addr, int n)
{
  if (f->cached == 0)
    return -1;

  if (f->cache_page_count == 0)
  {
    allocate_cache(f);
    allocate_cache_page(f);
  }

  int i = 0;
  while (i < n)
  {
    int target_block = f->off / PGSIZE;

    if (target_block >= f->cache_page_count)
      allocate_cache_page(f);

    int remaining_bytes_on_block = (target_block + 1) * PGSIZE - f->off;
    int bytes_to_write = n - i;

    if (bytes_to_write > remaining_bytes_on_block)
      bytes_to_write = remaining_bytes_on_block;

    if (copyin(myproc()->pagetable,
               f->cache[target_block] + (f->off % PGSIZE),
               addr + i, bytes_to_write) < 0)
      return -1;

    f->off += bytes_to_write;
    i += bytes_to_write;
  }

  if (f->off > f->cache_size)
    f->cache_size = f->off;

  return (i == n ? n : -1);
}

int filecachedflush(struct file *f)
{
  if (f->cached == 0)
    return -1;

  if (f->type != FD_INODE)
    panic("filecachedflush: invalid file type");

  int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;

  for (int i = 0; i < f->cache_page_count; i++)
  {
    int block_offset = i * PGSIZE;
    int bytes_remaining = PGSIZE;
    int bytes_written = 0;

    while (bytes_remaining > 0)
    {
      int n = bytes_remaining > max ? max : bytes_remaining;

      begin_op();
      ilock(f->ip);
      int r = writei(f->ip, 0, (uint64)f->cache[i] + bytes_written, block_offset + bytes_written, n);
      iunlock(f->ip);
      end_op();

      if (r <= 0)
      {
        printf("filecachedflush: error writing block %d\n", i);
        return -1;
      }

      bytes_remaining -= r;
      bytes_written += r;
    }
  }

  for (int i = 0; i < f->cache_page_count; i++)
    kfree((char *)f->cache[i]);
  kfree((char *)f->cache);
  f->cache = 0;
  f->cache_page_count = 0;
  f->cache_size = 0;

  return 0;
}

void allocate_cache(struct file *f)
{
  f->cache = (char **)kalloc();
  if (f->cache == 0)
    panic("allocate_cache: Could not allocate memory for cache");
}

void allocate_cache_page(struct file *f) {
  f->cache[f->cache_page_count] = (char *)kalloc();
  if (f->cache[f->cache_page_count++] == 0)
    panic("allocate_cache_page: Could not allocate memory for cache block");
}

