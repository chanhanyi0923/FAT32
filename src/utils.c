#include "utils.h"

/* these functions should be private */

extern struct fs_info fat_info;

u16 get_u16(u8 *ch) { return (*ch) + ((*(ch + 1)) << 8); }

u32 get_u32(u8 *ch) {
  return (*ch) + ((*(ch + 1)) << 8) + ((*(ch + 2)) << 16) + ((*(ch + 3)) << 24);
}

/* u16/u32 to char */
void set_u16(u8 *ch, u16 num) {
  *ch = (u8)(num & 0xFF);
  *(ch + 1) = (u8)((num >> 8) & 0xFF);
}

void set_u32(u8 *ch, u32 num) {
  *ch = (u8)(num & 0xFF);
  *(ch + 1) = (u8)((num >> 8) & 0xFF);
  *(ch + 2) = (u8)((num >> 16) & 0xFF);
  *(ch + 3) = (u8)((num >> 24) & 0xFF);
}

u32 get_start_cluster(const struct fat_file *file) {
  return (file->entry.attr.starthi << 16) + (file->entry.attr.startlow);
}

void set_start_cluster(struct fat_file *file, u32 clus) {
  file->entry.attr.starthi = (u16)((clus >> 16) & 0xffff);
  file->entry.attr.startlow = (u16)((clus & 0xffff));
}

u32 fs_wa(u32 num) {
  /* return the bits of `num` */
  u32 i;
  for (i = 0; num > 1; num >>= 1, i++);
  return i;
}

u32 fs_filename_cmp(const u8 *f1, const u8 *f2) {
  u32 i;
  for (i = 0; i < 11; i++) {
    if (f1[i] != f2[i]) return 1;
  }

  return 0;
}

/* data cluster num <==> sector num */
u32 fs_dataclus2sec(u32 clus) {
  return ((clus - 2) << fs_wa(fat_info.BPB.attr.BPB_SecPerClus)) +
         fat_info.first_data_sector;
}

u32 fs_datasec2clus(u32 sec) {
  return ((sec - fat_info.first_data_sector) >>
          fs_wa(fat_info.BPB.attr.BPB_SecPerClus)) + 2;
}

u32 fs_clus_to_sector_with_offset(u32 clus) {
  return fs_dataclus2sec(clus) + fat_info.base_addr;
}

u32 fs_sector_with_offset_to_clus(u32 sec) {
  return fs_datasec2clus(sec - fat_info.base_addr);
}
