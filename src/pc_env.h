#ifndef _PC_ENV_H
#define _PC_ENV_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define SECTOR_SIZE 512

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

void * kernel_memset ( void * ptr, int value, size_t num );
void* kmalloc (size_t size);
int kernel_printf( const char * format, ... );
void kfree( void* ptr );
int read_block(unsigned char *buf, unsigned int addr, unsigned int count);
int write_block(unsigned char *buf, unsigned int addr, unsigned int count);

#endif

