#include "fat.h"

#ifdef PC_ENV
#include "pc_env.h"
#endif

/* these functions should be private */


u16 get_u16(u8 *ch);
u32 get_u32(u8 *ch);

/* u16/u32 to char */
void set_u16(u8 *ch, u16 num);
void set_u32(u8 *ch, u32 num);

u32 get_start_cluster(const struct fat_file *file);
void set_start_cluster(struct fat_file *file, u32 clus);
u32 fs_wa(u32 num);
u32 fs_filename_cmp(const u8 *f1, const u8 *f2);

/* data cluster num <==> sector num */
u32 fs_dataclus2sec(u32 clus);
u32 fs_datasec2clus(u32 sec);
u32 fs_clus_to_sector_with_offset(u32 clus);
u32 fs_sector_with_offset_to_clus(u32 sec);

