#include "usr.h"
#include "fat.h"
#include "utils.h"

#ifdef PC_ENV
#include "pc_env.h"
#endif

extern struct fat_file pwd;


u32 fs_cat(u8 *path) {
  u8 filename[12];
  struct fat_file file;

  /* Open */
  if (0 != fs_open(&file, path)) {
    kernel_printf("File %s open failed\n", path);
    return 1;
  }

  /* Read */
  u8 *buf = (u8 *)kmalloc(file.entry.attr.size + 1);
  if (fs_read(&file, buf, file.entry.attr.size) == 1) {
    kernel_printf("File %s read failed\n", path);
    return 1;
  } else {
    buf[file.entry.attr.size] = 0;
    kernel_printf("%s\n", buf);
  }

  /* fs_close(&cat_file); */
  kfree(buf);
  return 0;
}


u32 fs_mkdir(const u8 *path)
{
  struct fat_file file;

  file.entry.attr.attr = 16; /* 16: dir */
  file.entry.attr.size = 0;
  file.entry.attr.starthi = 0;
  file.entry.attr.startlow = 0;
  file.entry.attr.lcase = 24;

  if (fs_create(&file, path) == 1) {
    goto fs_mkdir_fail;
  }

  u32 clus;
  if (fs_alloc(&clus) == 1) {
    goto fs_mkdir_fail;
  }

  set_start_cluster(&file, clus);
  if (fs_write_meta(&file) == 1) {
    goto fs_mkdir_fail;
  }

  if (fs_make_empty_dir(&file) == 1) {
    goto fs_mkdir_fail;
  }

  return 0;
fs_mkdir_fail:
  return 1;
}

u32 fs_touch(const u8 *path)
{
  struct fat_file file;

  file.entry.attr.attr = 32; /* 32: file */
  file.entry.attr.size = 0;
  file.entry.attr.starthi = 0;
  file.entry.attr.startlow = 0;
  file.entry.attr.lcase = 24;

  if (fs_create(&file, path) == 1) {
    goto fs_touch_fail;
  }

  return 0;
fs_touch_fail:
  return 1;
}

u32 fs_rm(const u8 *path)
{
  u8 filename[12];
  struct fat_file file;

  /* Open */
  if (0 != fs_open(&file, path)) {
    kernel_printf("File %s open failed\n", path);
    goto fs_rm_fail;
  }

  if (fs_remove_file(&file) == 1) {
    kernel_printf("File %s remove failed\n", path);
    goto fs_rm_fail;
  }

  return 0;
fs_rm_fail:
  return 1;
}

u32 fs_mv(const u8 *src, const u8 *dst) {
  struct fat_file file_src, file_dst;
  u32 i;

  if (0 != fs_open(&file_src, src)) {
    kernel_printf("File %s open failed\n", src);
    goto fs_mv_fail;
  }

  for (i = 0; i < 32; i ++) {
    file_dst.entry.data[i] = file_src.entry.data[i];
  }
  
  if (fs_create(&file_dst, dst) == 1) {
    kernel_printf("File %s create failed\n", dst);
    goto fs_mv_fail;
  }

  if (fs_remove_entry(&file_src) == 1) {
    kernel_printf("File %s remove failed\n", src);
    goto fs_mv_fail;
  }

  return 0;
fs_mv_fail:
  return 1;
}


u32 fs_ls(const u8 *path_) {
  u8 path[256];
  u32 i;
  struct fat_file file;

  for (i = 0; i < 256 && path_[i]; i ++) {
    path[i] = path_[i];
  }
  path[i] = 0;
  if (path[0] == 0) {
    path[0] = '.';
    path[1] = 0;
  } else if (path[-- i] == '/') {
    path[i] = 0;
  }

  /* Open */
  if (0 != fs_open(&file, path)) {
    kernel_printf("File %s open failed\n", path);
    goto fs_ls_fail;
  }

  /* check attr is dir */
  if ((file.entry.attr.attr & 0x10) == 0) {
    kernel_printf("%s is not a directory\n", path);
    goto fs_ls_fail;
  }

  /* Read */
  if (fs_read_dir(&file) == 1) {
    kernel_printf("Directory %s read failed\n", path);
    goto fs_ls_fail;
  }
  kernel_printf("\n");

  return 0;
fs_ls_fail:
  return 1;
}


u32 fs_cd(const u8 *path_) {
  u8 path[256];
  u32 i;
  struct fat_file file;

  for (i = 0; i < 256 && path_[i]; i ++) {
    path[i] = path_[i];
  }
  path[i] = 0;
  if (path[-- i] == '/') {
    path[i] = 0;
  }

  /* Open */
  if (0 != fs_open(&file, path)) {
    kernel_printf("File %s open failed\n", path);
    goto fs_cd_fail;
  }

  /* check attr is dir */
  if ((file.entry.attr.attr & 0x10) == 0) {
    kernel_printf("%s is not a directory\n", path);
    goto fs_cd_fail;
  }

  /* clone opened dir to pwd */
  pwd.loc = 0;
  pwd.dir_entry_pos = file.dir_entry_pos;
  pwd.dir_entry_sector = file.dir_entry_sector;
  for (i = 0; i < 32; i ++) {
    pwd.entry.data[i] = file.entry.data[i];
  }
  for (i = 0; i < 256; i ++) {
    pwd.path[i] = file.path[i];
  }

  return 0;
fs_cd_fail:
  return 1;
}

