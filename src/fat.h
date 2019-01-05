#ifndef _FAT_H
#define _FAT_H

#ifdef PC_ENV
#include "pc_env.h"
#endif

struct __attribute__((__packed__)) dir_entry_attr {
    u8 name[8];                   /* Name */
    u8 ext[3];                    /* Extension */
    u8 attr;                      /* attribute bits */
    u8 lcase;                     /* Case for base and extension */
    u8 ctime_cs;                  /* Creation time, centiseconds (0-199) */
    u16 ctime;                    /* Creation time */
    u16 cdate;                    /* Creation date */
    u16 adate;                    /* Last access date */
    u16 starthi;                  /* Start cluster (Hight 16 bits) */
    u16 time;                     /* Last modify time */
    u16 date;                     /* Last modify date */
    u16 startlow;                 /* Start cluster (Low 16 bits) */
    u32 size;                     /* file size (in bytes) */
};

union dir_entry {
    u8 data[32];
    struct dir_entry_attr attr;
};


/* DIR_FstClusHI/LO to clus */

struct fat_file {
    u8 path[256];
    /* Current file pointer */
    u32 loc;
    /* Current directory entry position */
    u32 dir_entry_pos;
    u32 dir_entry_sector;
    /* current directory entry */
    union dir_entry entry;
};

struct __attribute__((__packed__)) BPB_attr {
  /*  0x00 ~ 0x0f */
  u8 BS_jmpBoot[3];    /*  jump_code[3] */
  u8 BS_OEMName[8];    /*  oem_name[8] */
  u16 BPB_BytsPerSec;  /*  sector_size */
  u8 BPB_SecPerClus;   /*  sectors_per_cluster */
  u16 BPB_RsvdSecCnt;  /*  reserved_sectors */
  /*  0x10 ~ 0x1f */
  u8 BPB_NumFATs;      /*  number_of_copies_of_fat */
  u16 BPB_RootEntCnt;  /*  max_root_dir_entries */
  u16 BPB_TotSec16;    /*  num_of_small_sectors */
  u8 BPB_Media;        /*  media_descriptor */
  u16 BPB_FATSz16;     /*  sectors_per_fat */
  u16 BPB_SecPerTrk;   /*  sectors_per_track */
  u16 BPB_NumHeads;    /*  num_of_heads */
  u32 BPB_HiddSec;     /*  num_of_hidden_sectors */
  u32 BPB_TotSec32;    /*  num_of_sectors */

  /*  fat32 */
  u32 BPB_FATSz32;   /*  num_of_sectors_per_fat */
  u16 BPB_ExtFlags;  /*  flags */
  u16 BPB_FSVer;     /*  version */
  u32 BPB_RootClus;  /*  cluster_number_of_root_dir */
  /*  0x30 ~ 0x3f */
  u16 BPB_FSInfo;       /*  sector_number_of_fs_info */
  u16 BPB_BkBootSec;    /*  sector_number_of_backup_boot */
  u8 BPB_Reserved[12];  /*  reserved_data[12] */
  /*  0x40 ~ 0x51 */
  u8 BS_DrvNum;      /*  logical_drive_number */
  u8 BS_Reserved1;   /*  unused */
  u8 BS_BootSig;     /*  extended_signature */
  u32 BS_VolID;      /*  serial_number */
  u8 BS_VolLab[11];  /*  volume_name[11] */
  /*  0x52 ~ 0x1fe */
  u8 BS_FilSysType[8];  /*  fat_name[8] */

  u8 exec_code[420];     /*  exec_code[420] */
  u8 Signature_word[2];  /*  boot_record_signature[2] */
};


union BPB_info {
  u8 data[512];
  struct BPB_attr attr;
};

struct fs_info {
  u32 base_addr;
  u32 sectors_per_fat;
  u32 total_sectors;
  u32 total_data_clusters;
  u32 total_data_sectors;
  u32 first_data_sector;
  union BPB_info BPB;
  u8 fat_fs_info[SECTOR_SIZE];
};

#endif


