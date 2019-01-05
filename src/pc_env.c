#ifndef _PC_ENV_H
#define _PC_ENV_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define SECTOR_SIZE 512

void * kernel_memset ( void * ptr, int value, size_t num )
{
    return memset(ptr, value, num);
}

void* kmalloc (size_t size)
{
    return malloc(size);
}


int kernel_printf( const char * format, ... )
{
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

void kfree( void* ptr )
{
    return free(ptr);
}

int read_block(unsigned char *buf, unsigned int addr, unsigned int count) {
  FILE *fat32_fp;
  fat32_fp = fopen("../fat32.vhd", "rb");
  fseek(fat32_fp, addr << 9, SEEK_SET);
  fread(buf, 512, count, fat32_fp);
  return 0;
}

int write_block(unsigned char *buf, unsigned int addr, unsigned int count) {
  FILE *fat32_fp;
  fat32_fp = fopen("../fat32.vhd", "rb+");
  fseek(fat32_fp, addr << 9, SEEK_SET);
  fwrite(buf, 512, count, fat32_fp);
  fflush(fat32_fp);
  return 0;
}

#endif

