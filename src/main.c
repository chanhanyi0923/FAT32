
#include "fat.h"

void test_write_1(const char *path)
{
  struct fat_file file;

  /* Open */
  if (0 != fs_open(&file, path)) {
    printf("File %s open failed", path);
  }

  /* Read */
  u32 size = 1024;
  u8 *buf = (u8 *)kmalloc(size);
  u32 i;
  for (i = 0; i < size; i ++) {
    buf[i] = '-';
  }
  buf[size - 1] = '\n';

  if (fs_write(&file, buf, size) == 1) {
  }

}

void test_write_2(const char *path)
{
  struct fat_file file;

  /* Open */
  if (0 != fs_open(&file, path)) {
    printf("File %s open failed", path);
  }

  file.loc = 0;
  u32 size = 10000;
  u8 *buf = (u8 *)kmalloc(size);
  u32 i;
  for (i = 0; i < size; i ++) {
    buf[i] = (i % 26) + '0';
  }
  buf[size - 1] = '\n';

  if (fs_write(&file, buf, size) == 1) {
    puts("fail");
  }

}

int main() {
  init_fat_info();

  /* test other file in same dir */
  /*
  fs_rm("/b/a.txt");
  */

  fs_ls("/b/");
  fs_ls("/");
  fs_ls("/b");

  /*test ".."*/
/*
  fs_cat("/b/d/../abcd/test2.txt");

  fs_cat("/b/d/new/../../abcd/test2.txt");
*/


/* mv test */
/* 需要測試目錄裏面有其他文件的情況 */
/*
if (fs_mv("/b/a.txt", "/ttt.txt") == 1) {
  printf("mv fail\n");
}
*/


/*

if (fs_touch("/test.txt") == 1) {
  printf("/test.txt fail\n");
}

if (fs_touch("/b/test.txt") == 1) {
  printf("/b/test.txt fail\n");
}
*/

/*
if (fs_touch("/a/v/test.txt") == 1) {
  printf("/a/v/test.txt fail\n");
}
if (fs_touch("/a/c/test.txt") == 1) {
  printf("/a/c/test.txt fail\n");
}

if (fs_mkdir("/b/d/new") == 1) {
  printf("/a/v/new fail\n");
}
if (fs_mkdir("/a/c/new") == 1) {
  printf("/a/c/new fail\n");
}

if (fs_touch("/b/d/new/test.txt") == 1) {
  printf("/b/d/new/test.txt fail\n");
}

test_write_2("/b/d/new/test.txt");
*/


/*
test_write_2("/a/v/test.txt");
*/

/*
  fs_cat("/t4.txt");
*/
/*
test_write_2("/t4.txt");
*/
/*
const char path[] = "/a/c/t4.txt";
struct fat_file file;
if (0 != fs_open(&file, path)) {
  puts("a");
}
fs_write_meta(&file);
*/

  
  return 0;
}
