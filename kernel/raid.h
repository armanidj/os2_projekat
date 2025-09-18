//
// Created by os on 9/14/25.
//

#ifndef XV6_RISCV_OS2_RSICV_RAID1_RAID_H
#define XV6_RISCV_OS2_RSICV_RAID1_RAID_H



int init_raid(enum RAID_TYPE raid);
int read_raid(int blkn, uchar* data);
int write_raid(int blkn, uchar* data);
int disk_fail_raid(int diskn);
int disk_repaired_raid(int diskn);
int info_raid(uint *blkn, uint *blks, uint *diskn);
int destroy_raid();

#endif // XV6_RISCV_OS2_RSICV_RAID1_RAID_H
