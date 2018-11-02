// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define __OE_NEED_TIME_CALLS
#define _GNU_SOURCE
#include "fs/syscall.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/syscall.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <time.h>
#include <time.h>
#include <unistd.h>

static oe_syscall_hook_t _hook;
static oe_spinlock_t _lock;

static const uint64_t _SEC_TO_MSEC = 1000UL;
static const uint64_t _MSEC_TO_USEC = 1000UL;
static const uint64_t _MSEC_TO_NSEC = 1000000UL;

static long
_syscall_open(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    const char* filename = (const char*)x1;
    int flags = (int)x2;
    int mode = (int)x3;
    fs_errno_t err;
    int ret;

    err = fs_syscall_open(filename, flags, mode, &ret);

    if (err != FS_ENOENT)
    {
        errno = err;
        return ret;
    }

    if (flags == O_WRONLY)
        return STDOUT_FILENO;

    return -1;
}

static long
_syscall_creat(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    const char* pathname = (const char*)x1;
    int mode = (int)x2;
    fs_errno_t err;
    int ret;

    err = fs_syscall_creat(pathname, mode, &ret);

    if (err != FS_ENOENT)
    {
        errno = err;
        return ret;
    }

    return -1;
}

static long _syscall_close(long n, long x1, ...)
{
    int fd = (int)x1;
    int ret;

    if (fd >= 3)
    {
        errno = fs_syscall_close(fd, &ret);
        return ret;
    }

    /* required by mbedtls */
    return 0;
}

static long _syscall_mmap(long n, ...)
{
    /* Always fail */
    return EPERM;
}

static long _syscall_readv(long num, long x1, long x2, long x3, ...)
{
    int fd = (int)x1;
    const struct iovec* iov = (const struct iovec*)x2;
    int iovcnt = (int)x3;
    ssize_t ret;

    if (fd >= 3)
    {
        errno = fs_syscall_readv(fd, (fs_iovec_t*)iov, iovcnt, &ret);
        return ret;
    }

    /* return zero-bytes read */
    return 0;
}

static long _syscall_stat(long num, long x1, long x2, long x3, ...)
{
    const char* pathname = (const char*)x1;
    struct stat* buf = (struct stat*)x2;
    int ret = -1;
    fs_stat_t stat;

    errno = fs_syscall_stat(pathname, &stat, &ret);

    if (errno == 0)
    {
        buf->st_dev = stat.st_dev;
        buf->st_ino = stat.st_ino;
        buf->st_mode = stat.st_mode;
        buf->st_nlink = stat.st_nlink;
        buf->st_uid = stat.st_uid;
        buf->st_gid = stat.st_gid;
        buf->st_rdev = stat.st_rdev;
        buf->st_size = stat.st_size;
        buf->st_blksize = stat.st_blksize;
        buf->st_blocks = stat.st_blocks;
        buf->st_atim.tv_sec = stat.st_atim.tv_sec;
        buf->st_atim.tv_nsec = stat.st_atim.tv_nsec;
        buf->st_mtim.tv_sec = stat.st_mtim.tv_sec;
        buf->st_mtim.tv_nsec = stat.st_mtim.tv_nsec;
        buf->st_ctim.tv_sec = stat.st_ctim.tv_sec;
        buf->st_ctim.tv_nsec = stat.st_ctim.tv_nsec;
    }

    return ret;
}

static long
_syscall_ioctl(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    /* Silently ignore ioctl's */
    return 0;
}

static long
_syscall_writev(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    int fd = (int)x1;
    const struct iovec* iov = (const struct iovec*)x2;
    unsigned long iovcnt = (unsigned long)x3;
    long ret = 0;
    int device;

    if (fd >= 3)
    {
        errno = fs_syscall_writev(fd, (fs_iovec_t*)iov, iovcnt, &ret);
        return ret;
    }

    /* Allow writing only to stdout and stderr */
    switch (fd)
    {
        case STDOUT_FILENO:
        {
            device = 0;
            break;
        }
        case STDERR_FILENO:
        {
            device = 1;
            break;
        }
        default:
        {
            abort();
        }
    }

    for (unsigned long i = 0; i < iovcnt; i++)
    {
        oe_host_write(device, iov[i].iov_base, iov[i].iov_len);
        ret += iov[i].iov_len;
    }

    return ret;
}

static long _syscall_clock_gettime(long n, long x1, long x2)
{
    clockid_t clk_id = (clockid_t)x1;
    struct timespec* tp = (struct timespec*)x2;
    int ret = -1;
    uint64_t msec;

    if (!tp)
        goto done;

    if (clk_id != CLOCK_REALTIME)
    {
        /* Only supporting CLOCK_REALTIME */
        oe_assert("clock_gettime(): panic" == NULL);
        goto done;
    }

    if ((msec = oe_get_time()) == (uint64_t)-1)
        goto done;

    tp->tv_sec = msec / _SEC_TO_MSEC;
    tp->tv_nsec = (msec % _SEC_TO_MSEC) * _MSEC_TO_NSEC;

    ret = 0;

done:

    return ret;
}

static long _syscall_gettimeofday(long n, long x1, long x2)
{
    struct timeval* tv = (struct timeval*)x1;
    void* tz = (void*)x2;
    int ret = -1;
    uint64_t msec;

    if (tv)
        oe_memset(tv, 0, sizeof(struct timeval));

    if (tz)
        oe_memset(tz, 0, sizeof(struct timezone));

    if (!tv)
        goto done;

    if ((msec = oe_get_time()) == (uint64_t)-1)
        goto done;

    tv->tv_sec = msec / _SEC_TO_MSEC;
    tv->tv_usec = msec % _MSEC_TO_USEC;

    ret = 0;

done:
    return ret;
}

static long _syscall_nanosleep(long n, long x1, long x2)
{
    const struct timespec* req = (struct timespec*)x1;
    struct timespec* rem = (struct timespec*)x2;
    size_t ret = -1;
    uint64_t milliseconds = 0;

    if (rem)
        oe_memset(rem, 0, sizeof(*rem));

    if (!req)
        goto done;

    /* Convert timespec to milliseconds */
    milliseconds += req->tv_sec * 1000UL;
    milliseconds += req->tv_nsec / 1000000UL;

    /* Perform OCALL */
    ret = oe_sleep(milliseconds);

done:

    return ret;
}

static ssize_t _syscall_lseek(int fd, ssize_t off, int whence)
{
    ssize_t ret;

    errno = fs_syscall_lseek(fd, off, whence, &ret);

    return ret;
}

static int _syscall_link(const char* oldpath, const char* newpath)
{
    int ret;

    errno = fs_syscall_link(oldpath, newpath, &ret);

    return ret;
}

static int _syscall_unlink(const char* pathname)
{
    int ret;

    errno = fs_syscall_unlink(pathname, &ret);

    return ret;
}

static int _syscall_rename(const char* oldpath, const char* newpath)
{
    int ret;

    errno = fs_syscall_rename(oldpath, newpath, &ret);

    return ret;
}

static int _syscall_truncate(const char* path, ssize_t length)
{
    int ret;

    errno = fs_syscall_truncate(path, length, &ret);

    return ret;
}

static int _syscall_mkdir(const char* pathname, uint32_t mode)
{
    int ret;

    errno = fs_syscall_mkdir(pathname, mode, &ret);

    return ret;
}

static int _syscall_rmdir(const char* pathname)
{
    int ret;

    errno = fs_syscall_rmdir(pathname, &ret);

    return ret;
}

int _syscall_getdents(unsigned int fd, struct dirent* dirp, unsigned int count)
{
    int ret;

    errno = fs_syscall_getdents(fd, dirp, count, &ret);

    return ret;
}

static int _syscall_fcntl(int fd, int cmd, ...)
{
    switch (cmd)
    {
        case F_SETFD:
        {
            va_list ap;
            va_start(ap, cmd);

            int flags = va_arg(ap, int);

            /* Ignore O_CLOEXEC in enclaves since exec is unsupported. */
            if (flags == O_CLOEXEC)
                return 0;

            va_end(ap);
            return -1;
        }
        default:
        {
            return -1;
        }
    }
}

static int _syscall_access(const char* pathname, int mode)
{
    int ret;

    errno = fs_syscall_access(pathname, mode, &ret);

    return ret;
}

static int _syscall_getcwd(char* buf, unsigned long size)
{
    int ret;

    errno = fs_syscall_getcwd(buf, size, &ret);

    return ret;
}

static int _syscall_chdir(const char* path)
{
    int ret;

    errno = fs_syscall_chdir(path, &ret);

    return ret;
}

/* Intercept __syscalls() from MUSL */
long __syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    oe_spin_lock(&_lock);
    oe_syscall_hook_t hook = _hook;
    oe_spin_unlock(&_lock);

    /* Invoke the syscall hook if any */
    if (hook)
    {
        long ret = -1;

        if (hook(n, x1, x2, x3, x4, x5, x6, &ret) == OE_OK)
        {
            /* The hook handled the syscall */
            return ret;
        }

        /* The hook ignored the syscall so fall through */
    }

    switch (n)
    {
        case SYS_nanosleep:
            return _syscall_nanosleep(n, x1, x2);
        case SYS_gettimeofday:
            return _syscall_gettimeofday(n, x1, x2);
        case SYS_clock_gettime:
            return _syscall_clock_gettime(n, x1, x2);
        case SYS_writev:
            return _syscall_writev(n, x1, x2, x3, x4, x5, x6);
        case SYS_ioctl:
            return _syscall_ioctl(n, x1, x2, x3, x4, x5, x6);
        case SYS_open:
            return _syscall_open(n, x1, x2, x3, x4, x5, x6);
        case SYS_creat:
            return _syscall_creat(n, x1, x2, x3, x4, x5, x6);
        case SYS_close:
            return _syscall_close(n, x1, x2, x3, x4, x5, x6);
        case SYS_mmap:
            return _syscall_mmap(n, x1, x2, x3, x4, x5, x6);
        case SYS_readv:
            return _syscall_readv(n, x1, x2, x3, x4, x5, x6);
        case SYS_stat:
            return _syscall_stat(n, x1, x2, x3, x4, x5, x6);
        case SYS_lseek:
            return _syscall_lseek(x1, x2, x3);
        case SYS_link:
            return _syscall_link((const char*)x1, (const char*)x2);
        case SYS_unlink:
            return _syscall_unlink((const char*)x1);
        case SYS_rename:
            return _syscall_rename((const char*)x1, (const char*)x2);
        case SYS_truncate:
            return _syscall_truncate((const char*)x1, (ssize_t)x2);
        case SYS_mkdir:
            return _syscall_mkdir((const char*)x1, (uint32_t)x2);
        case SYS_rmdir:
            return _syscall_rmdir((const char*)x1);
        case SYS_fcntl:
            return _syscall_fcntl((int)x1, (int)x2);
        case SYS_access:
            return _syscall_access((const char*)x1, (int)x2);
        case SYS_getcwd:
            return _syscall_getcwd((char*)x1, (unsigned long)x2);
        case SYS_chdir:
            return _syscall_chdir((char*)x1);
        case SYS_getdents:
        case SYS_getdents64:
        {
            return _syscall_getdents(
                (unsigned int)x1, (struct dirent*)x2, (unsigned int)x3);
        }
        default:
        {
            /* All other MUSL-initiated syscalls are aborted. */
            fprintf(stderr, "error: __syscall(): n=%lu\n", n);
            abort();
            return 0;
        }
    }

    return 0;
}

/* Intercept __syscalls_cp() from MUSL */
long __syscall_cp(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    return __syscall(n, x1, x2, x3, x4, x5, x6);
}

long syscall(long number, ...)
{
    va_list ap;

    va_start(ap, number);
    long x1 = va_arg(ap, long);
    long x2 = va_arg(ap, long);
    long x3 = va_arg(ap, long);
    long x4 = va_arg(ap, long);
    long x5 = va_arg(ap, long);
    long x6 = va_arg(ap, long);
    long ret = __syscall(number, x1, x2, x3, x4, x5, x6);
    va_end(ap);

    return ret;
}

void oe_register_syscall_hook(oe_syscall_hook_t hook)
{
    oe_spin_lock(&_lock);
    _hook = hook;
    oe_spin_unlock(&_lock);
}
