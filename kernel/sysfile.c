//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "proc_metrics.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  argint(n, &fd);
  if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
    return -1;
  if (pfd)
    *pfd = fd;
  if (pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for (fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd] == 0)
    {
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

uint64
sys_dup(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;

  struct file *f;
  int fd;

  if (argfd(0, 0, &f) < 0) {
    ret = -1;
    goto return_block;
    // return -1;
  }
  if ((fd = fdalloc(f)) < 0) {
    ret = -1;
    goto return_block;
    // return -1;
  }
  filedup(f);
  ret = fd;
  // return fd;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

uint64
sys_read(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_read++;

  struct file *f;
  int n;
  uint64 q;

  argaddr(1, &q);
  argint(2, &n);
  if (argfd(0, 0, &f) < 0){
    ret = -1;
    goto return_block;
    // return -1;
  }
  ret = fileread(f, q, n);

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_read += xticks;

  return ret;
}

// uint64
// sys_read(void)
// {
//   uint64 ret = 0;
//   struct metrics *metrics;
//   struct proc *p = myproc();
//   metrics = get_metrics(p->pid);

//   uint xticks;
//   acquire(&tickslock);
//   xticks = ticks;
//   release(&tickslock);

//   metrics->io_metrics.num_io_calls++;
//   metrics->fs_metrics.n_read++;

//   struct file *f;
//   int n;
//   uint64 q;

//   argaddr(1, &q);
//   argint(2, &n);
//   if (argfd(0, 0, &f) < 0)
//   {
//     ret = -1;
//   }
//   if (ret != -1)
//     ret = fileread(f, q, n);

//   acquire(&tickslock);
//   xticks = ticks - xticks;
//   release(&tickslock);
//   xticks = p->ticks - xticks;

//   metrics->io_metrics.total_ticks += xticks;
//   metrics->fs_metrics.total_ticks_read += xticks;
//   return ret;
// }

uint64
sys_write(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;

  struct file *f;
  int n;
  uint64 q;

  argaddr(1, &q);
  argint(2, &n);
  if (argfd(0, 0, &f) < 0){
    ret = -1;
    goto return_block;
    // return -1;
  }

  ret = filewrite(f, q, n);

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

uint64
sys_close(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_delete++;

  int fd;
  struct file *f;

  if (argfd(0, &fd, &f) < 0){
    ret = -1;
    goto return_block;
    // return -1;
  }
  myproc()->ofile[fd] = 0;
  fileclose(f);
  ret = 0;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_delete += xticks;

  return ret;
}

uint64
sys_fstat(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_read++;

  struct file *f;
  uint64 st; // user pointer to struct stat

  argaddr(1, &st);
  if (argfd(0, 0, &f) < 0){
    ret = -1;
    goto return_block;
    // return -1;
  }
  ret = filestat(f, st);


return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_read += xticks;

  return ret;
}

// Create the path new as a link to the same inode as old.
uint64
sys_link(void)
{
  struct proc_metrics *proc_metrics;
  uint64 ret;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;

  char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
  struct inode *dp, *ip;

  if (argstr(0, old, MAXPATH) < 0 || argstr(1, new, MAXPATH) < 0){
    // return -1;
    ret = -1;
    goto return_block;
  }

  begin_op();
  if ((ip = namei(old)) == 0)
  {
    end_op();
    // return -1;
  }

  ilock(ip);
  if (ip->type == T_DIR)
  {
    iunlockput(ip);
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if ((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0)
  {
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  ret = 0;
  goto return_block;
  // return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  // return -1;
  ret = -1;
  goto return_block;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
  {
    if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if (de.inum != 0)
      return 0;
  }
  return 1;
}

uint64
sys_unlink(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;
  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_delete++;

  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], path[MAXPATH];
  uint off;

  if (argstr(0, path, MAXPATH) < 0){
    ret = -1;
    goto return_block;
    // return -1;
  }

  begin_op();
  if ((dp = nameiparent(path, name)) == 0)
  {
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if ((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if (ip->nlink < 1)
    panic("unlink: nlink < 1");
  if (ip->type == T_DIR && !isdirempty(ip))
  {
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if (ip->type == T_DIR)
  {
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  ret = 0;
  goto return_block;

bad:
  iunlockput(dp);
  end_op();
  ret = -1;
  goto return_block;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_delete += xticks;

  return ret;
}

static struct inode *
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if ((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if ((ip = dirlookup(dp, name, 0)) != 0)
  {
    iunlockput(dp);
    ilock(ip);
    if (type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }

  if ((ip = ialloc(dp->dev, type)) == 0)
  {
    iunlockput(dp);
    return 0;
  }

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if (type == T_DIR)
  { // Create . and .. entries.
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      goto fail;
  }

  if (dirlink(dp, name, ip->inum) < 0)
    goto fail;

  if (type == T_DIR)
  {
    // now that success is guaranteed:
    dp->nlink++; // for ".."
    iupdate(dp);
  }

  iunlockput(dp);

  return ip;

fail:
  // something went wrong. de-allocate ip.
  ip->nlink = 0;
  iupdate(ip);
  iunlockput(ip);
  iunlockput(dp);
  return 0;
}

uint64
sys_open(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;

  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  argint(1, &omode);
  if ((n = argstr(0, path, MAXPATH)) < 0){
    ret = -1;
    // return -1;
  }

  begin_op();

  if (omode & O_CREATE)
  {
    ip = create(path, T_FILE, 0, 0);
    if (ip == 0)
    {
      end_op();
      ret = -1;
      goto return_block;
      // return -1;
    }
  }
  else
  {
    if ((ip = namei(path)) == 0)
    {
      end_op();
      ret = -1;
      goto return_block;
      // return -1;
    }
    ilock(ip);
    if (ip->type == T_DIR && omode != O_RDONLY)
    {
      iunlockput(ip);
      end_op();
      ret = -1;
      goto return_block;
      // return -1;
    }
  }

  if (ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV))
  {
    iunlockput(ip);
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }

  if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0)
  {
    if (f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }

  if (ip->type == T_DEVICE)
  {
    f->type = FD_DEVICE;
    f->major = ip->major;
  }
  else
  {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if ((omode & O_TRUNC) && ip->type == T_FILE)
  {
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  ret = fd;
  goto return_block;
  // return fd;
return_block:
  xticks = p->ticks - xticks;

  if ((omode & (O_RDONLY | O_RDWR)) != 0) {
    proc_metrics->fs_metrics.n_read++;
    proc_metrics->fs_metrics.total_ticks_read += xticks;
  }

  if ((omode & (O_WRONLY | O_RDWR | O_CREATE | O_TRUNC)) != 0)  
  {
    proc_metrics->fs_metrics.n_write++;
    proc_metrics->fs_metrics.total_ticks_write += xticks;
  }

  proc_metrics->io_metrics.total_ticks += xticks;
  return ret;
}

uint64
sys_mkdir(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;

  char path[MAXPATH];
  struct inode *ip;

  begin_op();
  if (argstr(0, path, MAXPATH) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0)
  {
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }
  iunlockput(ip);
  end_op();
  ret = 0;
  // return 0;
return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

uint64
sys_mknod(void)
{
  struct inode *ip;
  char path[MAXPATH];
  int major, minor;

  begin_op();
  argint(1, &major);
  argint(2, &minor);
  if ((argstr(0, path, MAXPATH)) < 0 ||
      (ip = create(path, T_DEVICE, major, minor)) == 0)
  {
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_chdir(void)
{
  char path[MAXPATH];
  struct inode *ip;
  uint64 ret;
  struct proc *p = myproc();
  struct proc_metrics *proc_metrics;
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;
  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;


  begin_op();
  if (argstr(0, path, MAXPATH) < 0 || (ip = namei(path)) == 0)
  {
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }
  ilock(ip);
  if (ip->type != T_DIR)
  {
    iunlockput(ip);
    end_op();
    ret = -1;
    goto return_block;
    // return -1;
  }
  iunlock(ip);
  iput(p->cwd);
  end_op();
  p->cwd = ip;
  ret = 0;
  goto return_block;
  // return 0;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

uint64
sys_exec(void)
{
  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;

  char path[MAXPATH], *argv[MAXARG];
  int i;
  uint64 uargv, uarg;

  argaddr(1, &uargv);
  if (argstr(0, path, MAXPATH) < 0)
  {
    ret = -1;
    goto return_block;
    // return -1;
  }
  memset(argv, 0, sizeof(argv));
  for (i = 0;; i++)
  {
    if (i >= NELEM(argv))
    {
      goto bad;
    }
    if (fetchaddr(uargv + sizeof(uint64) * i, (uint64 *)&uarg) < 0)
    {
      goto bad;
    }
    if (uarg == 0)
    {
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if (argv[i] == 0)
      goto bad;
    if (fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  ret = exec(path, argv);

  for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  goto return_block;

bad:
  for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  ret = -1;
  goto return_block;
  // return -1;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

uint64
sys_pipe(void)
{
  uint64 fdarray; // user pointer to array of two integers
  struct file *rf, *wf;
  int fd0, fd1;
  struct proc *p = myproc();

  uint64 ret;
  struct proc_metrics *proc_metrics;
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_write++;

  argaddr(0, &fdarray);
  if (pipealloc(&rf, &wf) < 0){
    ret = -1;
    goto return_block;
    // return -1;
  }
  fd0 = -1;
  if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0)
  {
    if (fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    ret = -1;
    goto return_block;
    // return -1;
  }
  if (copyout(p->pagetable, fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
      copyout(p->pagetable, fdarray + sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0)
  {
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    ret = -1;
    goto return_block;
    // return -1;
  }
  ret = 0;
  goto return_block;
  // return 0;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_write += xticks;

  return ret;
}

uint64
sys_lseek(void)
{
  struct file *f;
  int fd;
  uint64 offset;
  int whence;

  uint64 ret;
  struct proc_metrics *proc_metrics;
  struct proc *p = myproc();
  proc_metrics = get_proc_metrics(p->pid);

  uint xticks = p->ticks;

  proc_metrics->io_metrics.num_io_calls++;
  proc_metrics->fs_metrics.n_read++;

  argint(1, (int *)&offset);
  argint(2, &whence);

  if (argfd(0, &fd, &f) < 0)
  {
    printf("lseek: argfd failed\n");
    exit(1);
  }

  if (whence < 0)
  {
    printf("lseek: invalid whence\n");
    exit(1);
  }

  if (offset < 0)
  {
    printf("lseek: invalid offset\n");
    exit(1);
  }

  switch (whence)
  {
  case SEEK_SET:
    f->off = offset;
    break;

  case SEEK_CUR:
    if (f->off + offset < 0){
      ret = -1;
      goto return_block;
      // return -1;
    }
    f->off += offset;
    break;

  case SEEK_END:
    f->off = f->ip->size + offset;
    break;

  default:
    printf("lseek: invalid whence\n");
  }

  ret = f->off;
  // return f->off;

return_block:
  xticks = p->ticks - xticks;

  proc_metrics->io_metrics.total_ticks += xticks;
  proc_metrics->fs_metrics.total_ticks_read += xticks;

  return ret;
}