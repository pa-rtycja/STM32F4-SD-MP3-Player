/**
 ******************************************************************************
 * @file      syscalls.c
 * @author    Auto-generated by STM32CubeIDE
 * @brief     STM32CubeIDE Minimal System calls file
 *
 *            For more information about which c-functions
 *            need which of these lowlevel functions
 *            please consult the Newlib libc-manual
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2020-2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes */
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <fcntl.h>
#include "fatfs.h"

/* Defines */
#define MAX_OPENED_FILES 2
#define FIRST_HANDLE 3

/* Variables */
extern int __io_putchar(int ch) __attribute__((weak));
extern int __io_getchar(void) __attribute__((weak));

char *__env[1] = { 0 };
char **environ = __env;

static FIL *file_handles[MAX_OPENED_FILES];

/* Functions */
static FIL *get_file_handle(int fd) {
  int index = fd - FIRST_HANDLE;

  if ((index < 0) || (index >= MAX_OPENED_FILES)) {
	  errno = EBADF;
	  return NULL;
  }

  FIL *file = file_handles[index];
  if (file == NULL) {
	  errno = EBADF;
  }
  return file;
}

static int find_free_handle(void) {
  int handle = -1;

  for (size_t i = 0; i < MAX_OPENED_FILES; ++i) {
	  if (file_handles[i] == NULL) {
		  handle = i;
		  break;
	  }
  }
  return handle;
}

void initialise_monitor_handles()
{
}

int _getpid(void)
{
  return 1;
}

int _kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

void _exit (int status)
{
  _kill(status, -1);
  while (1) {}    /* Make sure we hang here */
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
  if (file == 0) {
	  errno = EINVAL;
	  return -1;
  }

  FIL *handle = get_file_handle(file);
  if (handle == NULL) {
	  return -1;
  }

  UINT bytes_read;
  FRESULT ret = f_read(handle, ptr, len, &bytes_read);
  if (ret != FR_OK) {
	  errno = EIO;
	  return -1;
  }

  return bytes_read;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
  /* Redirect stdout and strerr to UART */
  if ((file == 1) || (file == 2)) {
	  for (size_t i = 0; i < len; ++i) {
		  __io_putchar(*ptr++);
	  }
	  return len;
  }

  FIL *handle = get_file_handle(file);
  if (handle == NULL) {
	  return -1;
  }

  UINT bytes_written;
  FRESULT ret = f_write(handle, ptr, len, &bytes_written);
  if (ret != FR_OK) {
  	  errno = EIO;
  	  return -1;
  }

  return bytes_written;
}

int _close(int file)
{
  FIL *handle = get_file_handle(file);
  if (handle == NULL) {
	  return -1;
  }

  FRESULT ret = f_close(handle);
  if (ret != FR_OK) {
	  errno = EIO;
	  ret = -1;
  }

  free(handle);
  file_handles[file - FIRST_HANDLE] = NULL;

  return ret;
}


int _fstat(int file, struct stat *st)
{
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  FRESULT ret;
  FSIZE_t pos, size;

  FIL *handle = get_file_handle(file);
  if (handle == NULL) {
	  errno = EBADF;
	  return -1;
  }

  switch (dir) {
  	  case SEEK_SET:
  		  ret = f_lseek(handle, ptr);
  		  break;
  	  case SEEK_CUR:
		  pos = f_tell(handle);
		  ret = f_lseek(handle, ptr + pos);
		  break;
  	  case SEEK_END:
  		  size = f_size(handle);
  		  ret = f_lseek(handle, size + ptr);
  		  break;
  	  default:
  		  ret = FR_INVALID_PARAMETER;
  		  break;
  }

  if (ret != FR_OK) {
	  errno = EIO;
	  return -1;
  }

  return f_tell(handle);
}

int _open(char *path, int flags, ...)
{
  int index = find_free_handle();

  if (index < 0) {
	  errno = ENFILE;
	  return -1;
  }

  FIL *file = calloc(1, sizeof(FIL));
  if (file == NULL) {
	  errno = ENOMEM;
	  return -1;
  }

  BYTE mode = 0;

  if ((flags & 0b111) == O_RDONLY) {
	  mode = FA_READ;
  }
  else if ((flags & 0b111) == O_WRONLY) {
	  mode = FA_WRITE;
  }
  else if ((flags & 0b111) == O_RDWR) {
	  mode = FA_READ | FA_WRITE;
  }

  if (flags & O_CREAT) {
	  mode |= FA_CREATE_ALWAYS;
  }
  else if (flags & O_APPEND) {
	  mode |= FA_OPEN_APPEND;
  }

  FRESULT ret = f_open(file, path, mode);
  if (ret != FR_OK) {
	  free(file);
	  errno = EIO;
	  return -1;
  }

  file_handles[index] = file;

  return index + FIRST_HANDLE;
}

int _wait(int *status)
{
  (void)status;
  errno = ECHILD;
  return -1;
}

int _unlink(char *name)
{
  (void)name;
  errno = ENOENT;
  return -1;
}

int _times(struct tms *buf)
{
  (void)buf;
  return -1;
}

int _stat(char *file, struct stat *st)
{
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _link(char *old, char *new)
{
  (void)old;
  (void)new;
  errno = EMLINK;
  return -1;
}

int _fork(void)
{
  errno = EAGAIN;
  return -1;
}

int _execve(char *name, char **argv, char **env)
{
  (void)name;
  (void)argv;
  (void)env;
  errno = ENOMEM;
  return -1;
}
