#include "fat.h"
#include "utils.h"

#ifdef PC_ENV
#include "pc_env.h"
#endif

struct fs_info fat_info;
struct fat_file pwd;

u32 init_fs() {
  if (0 != init_fat_info()) {
    goto init_fs_fail;
  }
  /* pwd */
  if (0 != fs_open(&pwd, "")) {
    goto init_fs_fail;
  }

  return 0;
init_fs_fail:
  return 1;
}

u32 init_fat_info() {
  u8 meta_buf[512];

  /* Init bufs */
  kernel_memset(meta_buf, 0, sizeof(meta_buf));
  kernel_memset(&fat_info, 0, sizeof(struct fs_info));

  /* Get MBR sector */
  if (read_block(meta_buf, 0, 1) == 1) {
    goto init_fat_info_err;
  }

  /* MBR partition 1 entry starts from +446, and LBA starts from +8 */
  fat_info.base_addr = get_u32(meta_buf + 446 + 8);

  /* Get FAT BPB */
  if (read_block(fat_info.BPB.data, fat_info.base_addr, 1) == 1) {
    goto init_fat_info_err;
  }

  if (read_block(fat_info.fat_fs_info, 1 + fat_info.base_addr, 1) == 1) {
    goto init_fat_info_err;
  }

  /* Sector size (MBR[11]) must be SECTOR_SIZE bytes */
  if (fat_info.BPB.attr.BPB_BytsPerSec != SECTOR_SIZE) {
    /* log(LOG_FAIL, "FAT32 Sector size must be %d bytes, but get %d bytes.", */
    /* SECTOR_SIZE, fat_info.BPB.attr.sector_size); */
    goto init_fat_info_err;
  }

  if (fat_info.BPB.attr.BPB_TotSec16 != 0) {
    /* FAT16 */
    fat_info.total_sectors = fat_info.BPB.attr.BPB_TotSec32;
  } else {
    /* FAT32 */
    fat_info.total_sectors = fat_info.BPB.attr.BPB_TotSec32;
  }

  if (fat_info.BPB.attr.BPB_FATSz16 != 0) {
    /* FAT16 */
    fat_info.sectors_per_fat = fat_info.BPB.attr.BPB_FATSz16;
  } else {
    /* FAT32 */
    fat_info.sectors_per_fat = fat_info.BPB.attr.BPB_FATSz32;
  }

  fat_info.first_data_sector =
      fat_info.BPB.attr.BPB_RsvdSecCnt +
      (fat_info.BPB.attr.BPB_NumFATs * fat_info.sectors_per_fat) +
      /* root dir sectors */
      ((fat_info.BPB.attr.BPB_RootEntCnt * 32) +
       (fat_info.BPB.attr.BPB_BytsPerSec - 1)) /
          fat_info.BPB.attr.BPB_BytsPerSec;

  fat_info.total_data_sectors =
      fat_info.total_sectors - fat_info.BPB.attr.BPB_RsvdSecCnt -
      fat_info.sectors_per_fat * fat_info.BPB.attr.BPB_NumFATs;

  fat_info.total_data_clusters =
      fat_info.total_data_sectors / fat_info.BPB.attr.BPB_SecPerClus;

  return 0;
init_fat_info_err:
  return 1;
}


u32 fs_get_fat_entry_value(u32 clus, u32 *next_clus) {
  u8 buf[512];

  u32 fat_offset = clus << 2;
  u32 this_fat_sec_num =
      fat_info.BPB.attr.BPB_RsvdSecCnt + fat_info.base_addr + (fat_offset >> 9);

  u32 this_fat_ent_offset = fat_offset & 511;

  if (read_block(buf, this_fat_sec_num, 1) == 1) {
    return 1;
  }

  *next_clus = 0x0fffffff & get_u32(buf + this_fat_ent_offset);
  return 0;
}

u32 fs_set_fat_entry_value(u32 clus, u32 next_clus) {
  u8 buf[512];

  u32 fat_offset = clus << 2;
  u32 this_fat_sec_num =
      fat_info.BPB.attr.BPB_RsvdSecCnt + fat_info.base_addr + (fat_offset >> 9);
  u32 this_fat_ent_offset = fat_offset & 511;

  if (read_block(buf, this_fat_sec_num, 1) == 1) {
    return 1;
  }
  u32 val = (0xf0000000 & get_u32(buf + this_fat_ent_offset)) |
            (0x0fffffff & next_clus);
  set_u32(buf + this_fat_ent_offset, val);
  if (write_block(buf, this_fat_sec_num, 1) == 1) {
    return 1;
  }
  return 0;
}

/* Find a free data cluster */
u32 fs_next_free(u32 start, u32 *next_free) {
  u32 clus;
  u32 next_clus;

  *next_free = 0xffffffff;

  for (clus = start; clus <= fat_info.total_data_clusters + 1; clus++) {
    if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
      goto fs_next_free_fail;
    }

    if (next_clus == 0) {
      *next_free = clus;
      break;
    }
  }

  return 0;
fs_next_free_fail:
  return 1;
}

u32 fs_alloc(u32 *new_alloc) {
  u32 clus, next_free;

  clus = get_u32(fat_info.fat_fs_info + 492) + 1;
  /*
   * If FSI_Nxt_Free is illegal (> FSI_Free_Count),
   * find a free data cluster from beginning
   */
  u32 free_cluster_count = get_u32(fat_info.fat_fs_info + 488) + 1;
  if (clus > free_cluster_count) {
    if (fs_next_free(2, &clus) == 1) {
      goto fs_alloc_fail;
    }
  }

  /* FAT allocated and update FSI_Nxt_Free */
  if (fs_set_fat_entry_value(clus, 0xFFFFFFFF) == 1) {
    goto fs_alloc_fail;
  }

  if (fs_next_free(clus, &next_free) == 1) {
    goto fs_alloc_fail;
  }

  /* no available free cluster */
  if (next_free > fat_info.total_data_clusters + 1) {
    goto fs_alloc_fail;
  }

  set_u32(fat_info.fat_fs_info + 492, next_free - 1);

  if (write_block(fat_info.fat_fs_info, 1 + fat_info.base_addr, 1) == 1) {
    goto fs_alloc_fail;
  }

  *new_alloc = clus;

  /* Erase new allocated cluster */
  /*
  if (write_block(new_alloc_empty, fs_dataclus2sec(clus),
                  fat_info.BPB.attr.sectors_per_cluster) == 1) {
    goto fs_alloc_fail;
  }
  */

  return 0;
fs_alloc_fail:
  return 1;
}

u32 fs_write_meta(struct fat_file *file) {
  u8 buf[512];
  u32 i;
  if (read_block(buf, file->dir_entry_sector, 1) == 1) {
    goto fs_write_meta_fail;
  }

  for (i = 0; i < 32; i++) {
    *(buf + file->dir_entry_pos + i) = file->entry.data[i];
  }
  if (write_block(buf, file->dir_entry_sector, 1) == 1) {
    goto fs_write_meta_fail;
  }

fs_write_meta_end:
  return 0;
fs_write_meta_fail:
  return 1;
}

/* Write to file */
u32 fs_write(struct fat_file *file, const u8 *buf, u32 size) {
  u8 block_buf[512];

  u32 begin_byte = file->loc & 4095;
  u32 end_byte = (file->loc + size - 1) & 4095;
  u32 begin_clus_count = file->loc >> 12;
  u32 end_clus_count = (file->loc + size - 1) >> 12;
  u32 clus = get_start_cluster(file), next_clus;
  u32 sector, next_sector;
  u32 i, offset, count = 0;

  if (size == 0) {
    goto fs_write_end;
  }

  /* empty file */
  if (clus == 0) {
    if (fs_alloc(&clus) == 1) {
      goto fs_write_fail;
    }
    set_start_cluster(file, clus);
  }

  /* get first sector */
  sector = fs_clus_to_sector_with_offset(clus);

  u32 clus_count;
  for (clus_count = 0; clus_count <= end_clus_count; clus_count++) {
    if (clus_count == begin_clus_count && clus_count == end_clus_count) {
      for (i = (begin_byte >> 9); i < (end_byte >> 9) + 1; i++) {
        u32 begin_offset = i == (begin_byte >> 9) ? (begin_byte & 511) : 0;
        u32 end_offset = i == (end_byte >> 9) ? ((end_byte & 511) + 1) : 512;
        for (offset = begin_offset; offset < end_offset; offset++) {
          block_buf[offset] = buf[count++];
        }
        if (write_block(block_buf, sector + i, 1) == 1) {
          goto fs_write_fail;
        }
      }
    } else if (clus_count == begin_clus_count) {
      for (i = (begin_byte >> 9); i < 8; i++) {
        u32 begin_offset = i == (begin_byte >> 9) ? (begin_byte & 511) : 0;
        for (offset = begin_offset; offset < 512; offset++) {
          block_buf[offset] = buf[count++];
        }
        if (write_block(block_buf, sector + i, 1) == 1) {
          goto fs_write_fail;
        }
      }
    } else if (clus_count == end_clus_count) {
      for (i = 0; i < (end_byte >> 9) + 1; i++) {
        u32 end_offset = i == (end_byte >> 9) ? ((end_byte & 511) + 1) : 512;
        for (offset = 0; offset < end_offset; offset++) {
          block_buf[offset] = buf[count++];
        }
        if (write_block(block_buf, sector + i, 1) == 1) {
          goto fs_write_fail;
        }
      }
    } else if (clus_count > begin_clus_count) {
      for (i = 0; i < 8; i++) {
        for (offset = 0; offset < 512; offset++) {
          block_buf[offset] = buf[count++];
        }
        if (write_block(block_buf, sector + i, 1) == 1) {
          goto fs_write_fail;
        }
      }
    }

    /* get next cluster */
    u32 clus = fs_sector_with_offset_to_clus(sector);
    u32 next_clus;
    if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
      goto fs_write_fail;
    }

    /* need to alloc a new cluster */
    if (clus_count < end_clus_count &&
        next_clus > fat_info.total_data_clusters + 1) {
      if (fs_alloc(&next_clus) == 1) {
        goto fs_write_fail;
      }
      if (fs_set_fat_entry_value(clus, next_clus) == 1) {
        goto fs_write_fail;
      }
    }

    /* next_sector */
    sector = fs_clus_to_sector_with_offset(next_clus);
  }

  /* update file size */
  if (file->loc + count > file->entry.attr.size) {
    file->entry.attr.size = file->loc + count;
  }
  file->loc += count;

  /* write back to meta data */
  if (fs_write_meta(file) == 1) {
    goto fs_write_fail;
  }

fs_write_end:
  return 0;

fs_write_fail:
  return 1;
}

u32 fs_find_in_dir(const u32 begin_sector, const u8 *name,
                   struct fat_file *file) {
  u8 buf[512];

  u32 sector, next_sector;
  for (sector = begin_sector;; sector = next_sector) {
    u32 offset;
    for (offset = 0; offset < 4096; offset += 32) {
      if ((offset & 511) == 0) {
        if (read_block(buf, sector + (offset >> 9), 1) == 1) {
          goto fs_find_in_dir_fail;
        }
      }
      u32 i;
      for (i = 0; i < 32; i++) {
        file->entry.data[i] = *(buf + (offset & 511) + i);
      }

      switch (file->entry.attr.attr) {
        case 16:
        case 32:
          if (fs_filename_cmp(file->entry.data, name) == 0 &&
              (file->entry.attr.attr & 0x08) == 0) {
            file->dir_entry_pos = (offset & 511);
            file->dir_entry_sector = sector + (offset >> 9);
            goto fs_find_in_dir_suc;
          }
          break;
        case 15:
        default:
          break;
      }
    }

    u32 clus = fs_sector_with_offset_to_clus(sector);
    u32 next_clus;
    if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
      goto fs_find_in_dir_fail;
    }

    if (next_clus >= 0x0ffffff8) {
      break;
    }
    next_sector = fs_clus_to_sector_with_offset(next_clus);
  }

fs_find_in_dir_fail:
  return 1;
fs_find_in_dir_suc:
  return 0;
}

/* path convertion */
u32 fs_next_slash(const u8 *f, u8 *filename11) {
  u32 i, j, k;
  u8 chr11[13];
  for (i = 0; (*(f + i) != 0) && (*(f + i) != '/'); i++);

  if (*f == '.' && *(f + 1) == '.' && (*(f + 2) == 0 || *(f + 2) == '/')) {
    const u8 name[12] = "..         ";
    for (j = 0; j < 12; j ++) {
      filename11[j] = name[j];
    }
  } else if (*f == '.' && (*(f + 1) == 0 || *(f + 1) == '/')) {
    const u8 name[12] = ".          ";
    for (j = 0; j < 12; j ++) {
      filename11[j] = name[j];
    }
  } else {
    for (j = 0; j < 12; j++) {
      chr11[j] = 0;
      filename11[j] = 0x20;
    }
    for (j = 0; j < 12 && j < i; j++) {
      chr11[j] = *(f + j);
      if (chr11[j] >= 'a' && chr11[j] <= 'z')
        chr11[j] = (u8)(chr11[j] - 'a' + 'A');
    }
    chr11[12] = 0;

    for (j = 0; (chr11[j] != 0) && (j < 12); j++) {
      if (chr11[j] == '.') break;

      filename11[j] = chr11[j];
    }

    if (chr11[j] == '.') {
      j++;
      for (k = 8; (chr11[j] != 0) && (j < 12) && (k < 11); j++, k++) {
        filename11[k] = chr11[j];
      }
    }

    filename11[11] = 0;
  }

  return i;
}

u32 fs_find(struct fat_file *file) {
  u8 *path;
  u32 sector, begin_sector, next_sector;

  /* root */
  if (file->path[0] == 0) {
    file->dir_entry_pos = 0;
    file->dir_entry_sector = 0;
    file->entry.attr.attr = 16;
    set_start_cluster(file, fat_info.BPB.attr.BPB_RootClus);
    goto fs_find_suc;
  }

  if (file->path[0] == '/') {
    path = file->path + 1;
    begin_sector = fs_clus_to_sector_with_offset(fat_info.BPB.attr.BPB_RootClus);
  } else {
    path = file->path;
    begin_sector = fs_clus_to_sector_with_offset(get_start_cluster(&pwd));
  }

  for (sector = begin_sector; ; sector = next_sector) {
    u8 filename11[12];
    path += fs_next_slash(path, filename11);

    if (fs_find_in_dir(sector, filename11, file) == 1) {
      goto fs_find_fail;
    }

    if (*path == 0) {
      break;
    } else if ((file->entry.attr.attr & 0x10) == 0) {
      /* should be sub-dir*/
      goto fs_find_fail;
    }
    path++;
    next_sector = fs_clus_to_sector_with_offset(get_start_cluster(file));
  }

fs_find_suc:
  return 0;

fs_find_fail:
  return 1;
}

u32 fs_read(struct fat_file *file, u8 *buf, u32 size) {
  u32 count = 0;
  u32 begin_byte = file->loc & 4095;
  u32 end_byte = (file->loc + size - 1) & 4095;
  u32 begin_clus_count = file->loc >> 12;
  u32 end_clus_count = (file->loc + size - 1) >> 12;
  u32 start_clus = get_start_cluster(file);
  u32 sector = fs_clus_to_sector_with_offset(start_clus), next_sector;
  u32 i, offset;
  u8 block_buf[512];

  if (start_clus == 0) {
    goto fs_read_suc;
  }

  if (file->loc + size > file->entry.attr.size) {
    size = file->entry.attr.size - file->loc;
  }

  if (size == 0) {
    goto fs_read_suc;
  }

  if (begin_clus_count == end_clus_count) {
    for (i = (begin_byte >> 9); i < (end_byte >> 9) + 1; i++) {
      if (read_block(block_buf, sector + i, 1) == 1) {
        return 1;
      }

      u32 begin_offset = i == (begin_byte >> 9) ? (begin_byte & 511) : 0;
      u32 end_offset = i == (end_byte >> 9) ? ((end_byte & 511) + 1) : 512;
      for (offset = begin_offset; offset < end_offset; offset++) {
        buf[count++] = block_buf[offset];
      }
    }
  } else {
    u32 clus_count;
    for (clus_count = 0; clus_count <= end_clus_count; clus_count++) {
      if (clus_count == begin_clus_count) {
        for (i = (begin_byte >> 9); i < 8; i++) {
          if (read_block(block_buf, sector + i, 1) == 1) {
            return 1;
          }
          u32 begin_offset = i == (begin_byte >> 9) ? (begin_byte & 511) : 0;
          for (offset = begin_offset; offset < 512; offset++) {
            buf[count++] = block_buf[offset];
          }
        }
      } else if (clus_count == end_clus_count) {
        for (i = 0; i < (end_byte >> 9) + 1; i++) {
          if (read_block(block_buf, sector + i, 1) == 1) {
            return 1;
          }
          u32 end_offset = i == (end_byte >> 9) ? ((end_byte & 511) + 1) : 512;
          for (offset = 0; offset < end_offset; offset++) {
            buf[count++] = block_buf[offset];
          }
        }
      } else if (clus_count > begin_clus_count) {
        for (i = 0; i < 8; i++) {
          if (read_block(block_buf, sector + i, 1) == 1) {
            return 1;
          }
          for (offset = 0; offset < 512; offset++) {
            buf[count++] = block_buf[offset];
          }
        }
      }

      /* get next cluster */
      u32 clus = fs_sector_with_offset_to_clus(sector);
      u32 next_clus;
      if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
        return 1;
      }

      if (clus_count < end_clus_count && next_clus >= 0x0ffffff8) {
        goto fs_read_fail;
      }
      /* next_sector */
      sector = fs_clus_to_sector_with_offset(next_clus);
    }
  }

  file->loc += count;

fs_read_suc:
  return 0;

fs_read_fail:
  return 1;
}

u32 fs_open(struct fat_file *file, const u8 *path) {
  u32 i;
  for (i = 0; i < 256; i++) file->path[i] = 0;
  for (i = 0; i < 256 && path[i] != 0; i++) file->path[i] = path[i];

  file->loc = 0;

  if (fs_find(file) == 1) goto fs_open_err;

  /* If file not exists */
  if (file->dir_entry_pos == 0xFFFFFFFF) goto fs_open_err;

  return 0;
fs_open_err:
  return 1;
}


u32 fs_find_empty_dir_entry(const u32 begin_sector, struct fat_file *file) {
  u8 buf[512];

  u32 sector, next_sector;
  for (sector = begin_sector;; sector = next_sector) {
    u32 offset;
    for (offset = 0; offset < 4096; offset += 32) {
      if ((offset & 511) == 0) {
        if (read_block(buf, sector + (offset >> 9), 1) == 1) {
          goto fs_find_empty_dir_entry_fail;
        }
      }

      u8 first_byte = *(buf + (offset & 511));
      /* 0: available, 0xE5: erased */
      if (first_byte == 0 || first_byte == 0xE5) {
          file->dir_entry_pos = (offset & 511);
          file->dir_entry_sector = sector + (offset >> 9);
          goto fs_find_empty_dir_entry_suc;
      }
    }

    /* get next cluster */
    u32 clus = fs_sector_with_offset_to_clus(sector);
    u32 next_clus;
    if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
      goto fs_find_empty_dir_entry_fail;
    }

    /* need to alloc a new cluster */
    if (next_clus >= 0x0ffffff8) {
      if (fs_alloc(&next_clus) == 1) {
        goto fs_find_empty_dir_entry_fail;
      }
      if (fs_set_fat_entry_value(clus, next_clus) == 1) {
        goto fs_find_empty_dir_entry_fail;
      }
    }
    next_sector = fs_clus_to_sector_with_offset(next_clus);
  }

fs_find_empty_dir_entry_fail:
  return 1;
fs_find_empty_dir_entry_suc:
  return 0;
}


u32 fs_create(struct fat_file *file, const u8 *path) {
  u32 sector, i;
  u8 filename11[12];
  struct fat_file *dir = file;
  u8 entry_data[32];
  for (i = 0; i < 32; i ++) {
    entry_data[i] = file->entry.data[i];
  }

  for (i = 0; i < 256; i++) {
    dir->path[i] = 0;
  }
  for (i = 0; i < 256 && path[i] != 0; i++) {
    dir->path[i] = path[i];
  }

  /* find last slash */
  for (; i > 0 && dir->path[i] != '/'; i --);
  if (dir->path[i] == '/') {
    fs_next_slash(dir->path + i + 1, filename11);
    for (; i < 256 && dir->path[i] != 0; i ++) {
      dir->path[i] = 0;
    }
  } else {
    fs_next_slash(dir->path, filename11);
    dir->path[0] = '.';
    dir->path[1] = 0;
  }

  dir->loc = 0;

  /* find dir */
  if (fs_find(dir) == 1) {
    goto fs_create_fail;
  }

  /* If dir not exists */
  if (dir->dir_entry_pos == 0xFFFFFFFF) {
    goto fs_create_fail;
  }

  sector = fs_clus_to_sector_with_offset(get_start_cluster(dir));

  /* if file already exists */
  if (fs_find_in_dir(sector, filename11, file) == 0) {
    goto fs_create_fail;
  }

  if (fs_find_empty_dir_entry(sector, file) == 1) {
    goto fs_create_fail;
  }

  /* set attributes for file */
  for (i = 11; i < 32; i ++) {
    file->entry.data[i] = entry_data[i];
  }
  for (i = 0; i < 11; i ++) {
    file->entry.data[i] = filename11[i];
  }

  /* write back to meta data */
  if (fs_write_meta(file) == 1) {
    goto fs_create_fail;
  }

  return 0;
fs_create_fail:
  return 1;
}


u32 fs_make_empty_dir(struct fat_file *file)
{
  u8 buf[512];
  u32 i, sector;
  u32 dot_clus = get_start_cluster(file);
  u32 double_dot_clus = fs_sector_with_offset_to_clus(file->dir_entry_sector);
  sector = fs_clus_to_sector_with_offset(dot_clus);
  for (i = 0; i < 512; i ++) {
    buf[i] = 0;
  }
  for (i = 1; i < fat_info.BPB.attr.BPB_SecPerClus; i ++) {
    if (write_block(buf, sector + i, 1) == 1) {
      goto fs_make_empty_dir_fail;
    }
  }

  u8 dc0 = (u8)(0xff & (dot_clus >> 16));
  u8 dc1 = (u8)(0xff & (dot_clus >> 24));
  u8 dc2 = (u8)(0xff & (dot_clus));
  u8 dc3 = (u8)(0xff & (dot_clus >> 8));

  u8 ddc0 = (u8)(0xff & (double_dot_clus >> 16));
  u8 ddc1 = (u8)(0xff & (double_dot_clus >> 24));
  u8 ddc2 = (u8)(0xff & (double_dot_clus));
  u8 ddc3 = (u8)(0xff & (double_dot_clus >> 8));

  const u8 dot[32] = {
    0x2E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,  dc0,  dc1, 0x00, 0x00,
    0x00, 0x00,  dc2,  dc3, 0x00, 0x00, 0x00, 0x00
  };

  const u8 double_dot[32] = {
    0x2E, 0x2E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, ddc0, ddc1, 0x00, 0x00,
    0x00, 0x00, ddc2, ddc3, 0x00, 0x00, 0x00, 0x00
  };

  for (i = 0; i < 32; i ++) {
    buf[i] = dot[i];
    buf[i + 32] = double_dot[i];
  }

  if (write_block(buf, sector, 1) == 1) {
    goto fs_make_empty_dir_fail;
  }

  return 0;
fs_make_empty_dir_fail:
  return 1;
}


u32 fs_remove_entry(struct fat_file *file) {
  u32 i;
  for (i = 1; i < 32; i ++) {
    file->entry.data[i] = 0;
  }
  file->entry.data[0] = 0xE5;

  if (fs_write_meta(file) == 1) {
    return 1;
  }
  return 0;
}

u32 fs_remove_file(struct fat_file *file) {
  u32 clus_count, end_clus_count;
  u32 clus, next_clus;

  if (file->entry.attr.size > 0) {
    end_clus_count = (file->entry.attr.size - 1) >> 12;
    clus = get_start_cluster(file);

    for (clus_count = 0; clus_count <= end_clus_count; clus_count ++) {
      if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
        goto fs_remove_fail;
      }
      /* clear fat */
      if (fs_set_fat_entry_value(clus, 0) == 1) {
        goto fs_remove_fail;
      }
      if (clus_count < end_clus_count && next_clus >= 0x0ffffff8) {
        goto fs_remove_fail;
      }
      clus = next_clus;
    }
  }

  if (fs_remove_entry(file) == 1) {
    goto fs_remove_fail;
  }

fs_remove_end:
  return 0;
fs_remove_fail:
  return 1;
}


void fs_get_filename(const u8 *entry, u8 *buf) {
  u32 i;
  u32 l1 = 0, l2 = 8;

  for (i = 0; i < 11; i++) {
    buf[i] = entry[i];
  }

  if (buf[0] == '.') {
    if (buf[1] == '.') {
      buf[2] = 0;
    } else {
      buf[1] = 0;
    }
  } else {
    for (i = 0; i < 8; i++) {
      if (buf[i] == 0x20) {
        buf[i] = '.';
        l1 = i;
        break;
      }
    }

    if (i == 8) {
      for (i = 11; i > 8; i--) {
        buf[i] = buf[i - 1];
      }

      buf[8] = '.';
      l1 = 8;
      l2 = 9;
    }

    for (i = l1 + 1; i < l1 + 4; i++) {
      if (buf[l2 + i - l1 - 1] != 0x20) {
        buf[i] = buf[l2 + i - l1 - 1];
      } else {
        break;
      }
    }

    buf[i] = 0;

    if (buf[i - 1] == '.') {
      buf[i - 1] = 0;
    }
  }
}


u32 fs_read_dir(const struct fat_file *dir) {
  struct fat_file file;
  u8 buf[512], name[12];
  u32 sector, next_sector, begin_sector;
  u8 is_first = 1;

  begin_sector = fs_clus_to_sector_with_offset(get_start_cluster(dir));

  for (sector = begin_sector;; sector = next_sector) {
    u32 offset;
    for (offset = 0; offset < 4096; offset += 32) {
      if ((offset & 511) == 0) {
        if (read_block(buf, sector + (offset >> 9), 1) == 1) {
          goto fs_read_dir_fail;
        }
      }

      u8 first_byte = *(buf + (offset & 511));
      if (first_byte == 0) {
        goto fs_read_dir_end;
      }

      u32 i;
      for (i = 0; i < 32; i++) {
        file.entry.data[i] = *(buf + (offset & 511) + i);
      }

      switch (file.entry.attr.attr) {
        case 16:
          if (!is_first) {
            kernel_printf("  ");
          }
          is_first = 0;
          fs_get_filename(file.entry.data, name);
          kernel_printf("%s/", name);
          break;
        case 32:
          if (!is_first) {
            kernel_printf("  ");
          }
          is_first = 0;
          fs_get_filename(file.entry.data, name);
          kernel_printf("%s", name);
          break;
        case 15:
        default:
          break;
      }
    }

    u32 clus = fs_sector_with_offset_to_clus(sector);
    u32 next_clus;
    if (fs_get_fat_entry_value(clus, &next_clus) == 1) {
      goto fs_read_dir_fail;
    }

    if (next_clus >= 0x0ffffff8) {
      break;
    }
    next_sector = fs_clus_to_sector_with_offset(next_clus);
  }

fs_read_dir_end:
  return 0;
fs_read_dir_fail:
  return 1;
}


#include "new.c"
