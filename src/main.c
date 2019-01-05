
#include "fat.h"

extern struct fat_file pwd;

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

int test_write_2(const char *path)
{
  struct fat_file file;

  /* Open */
  if (0 != fs_open(&file, path)) {
    printf("File %s open failed", path);
    return 1;
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
return 1;
  }
return 0;
}

int main() {
  init_fs();

  char buf[512];
  char arg0[128];
  char arg1[128];
  char arg2[128];

  while (1) {
    printf("%s $\n", pwd.path);
    gets(buf);
    sscanf(buf, "%s%s%s", arg0, arg1, arg2);
    
    if (strcmp(arg0, "cat") == 0) {
      if (fs_cat(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "write") == 0) {
      if (test_write_2(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "mkdir") == 0) {
      if (fs_mkdir(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "touch") == 0) {
      if (fs_touch(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "rm") == 0) {
      printf("%s %s\n", arg0, arg1);
      if (fs_rm(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "ls") == 0) {
      if (fs_ls(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "cd") == 0) {
      if (fs_cd(arg1) != 0) {
        puts("fail");
      }
    } else if (strcmp(arg0, "mv") == 0) {
      if (fs_mv(arg1, arg2) != 0) {
        puts("fail");
      }
    }
    puts("");
  }

  /* test other file in same dir */
  /*
  fs_rm("/b/a.txt");
  */
/*
  fs_cd("/test");

printf("%s $\n", pwd.path);
fs_ls("/b/");
printf("%s $\n", pwd.path);
fs_ls("/");
printf("%s $\n", pwd.path);
fs_ls("/b");

printf("%s $\n", pwd.path);
fs_ls("c");
printf("%s $\n", pwd.path);
fs_ls("d");

fs_touch("t1.txt");
test_write_2("t1.txt");


fs_cd("d");
printf("%s $\n", pwd.path);

fs_touch("t2.txt");
test_write_2("t2.txt");


fs_ls("");
printf("%s $\n", pwd.path);
fs_ls(".");
printf("%s $\n", pwd.path);
fs_ls("b");
printf("%s $\n", pwd.path);
fs_cd("../c");
printf("%s $\n", pwd.path);

fs_touch("t3.txt");
test_write_2("t3.txt");


fs_ls("b");
printf("%s $\n", pwd.path);
fs_ls(".");
*/


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
