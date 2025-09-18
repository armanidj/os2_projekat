//
// Created by os on 12/11/24.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "raid.h"
#include "fs.h"
#include "param.h"

#define DISK_NUM (DISKS)

typedef struct raidmeta {
	uint64 raidType;     // which RAID is it?
	uint64 diskNum;      // which disk in the RAID is it?
	uint64 isActive;     // is the disk up and running? (probably not needed but might be useful for integrity checks)
	uchar padding[1000]; // blank space to fill out the whole block
}RaidMeta;               // to write to block zero of each disk

typedef struct raidinfo {
    uint64 valid;       // is RAID structure valid?
    uint64 raidType;    // which RAID is it?
	uint64 numOfDisks;  // how many disks do we have in the RAID? (bit position - 1 == disk number)
    uint64 onlineDisks; // which disks are currently up and running?
}RaidInfo;              // to have info stored in RAM

RaidInfo currentRaid;
RaidMeta diskmeta;


int init_raid (enum RAID_TYPE raid) {

	if ((raid == 0 || raid == 2) && DISK_NUM % 2) panic("Attempted to make a RAID 0 or 0_1 array with an odd amount of disks.\n       Please check again.");
    // set
	currentRaid.onlineDisks = 0;
	currentRaid.valid = 1;
	currentRaid.raidType = raid;

    diskmeta.raidType = raid;
    diskmeta.isActive = 1;


    for (int i = 1; i <= DISK_NUM; i++) {

    	diskmeta.diskNum = i;
		write_block(i, 0, ((unsigned char*)&diskmeta));

    	currentRaid.onlineDisks += (1 << (i-1));
    }

    currentRaid.numOfDisks = currentRaid.onlineDisks;

    return 0;

}

int write_raid (int blkNum, unsigned char *data) {

	if (!currentRaid.valid) { return -1;}

    switch (currentRaid.raidType) {
	case RAID0:

		if (currentRaid.numOfDisks != currentRaid.onlineDisks) { return -1;}
		write_block((blkNum % DISK_NUM) + 1, (blkNum / DISK_NUM) + 1, data);

		break;

        case RAID1:

            for (int i = 1; i <= DISK_NUM; i++) {
                if ((currentRaid.onlineDisks >> (i-1)) & 1)
			write_block(i, blkNum, data);

            }
	        break;

	case RAID0_1:
		if (currentRaid.numOfDisks != currentRaid.onlineDisks) { return -1;}

    	int disk = blkNum % (DISK_NUM/2) + 1;
		int diskPair = ((disk + (DISK_NUM / 2)) % DISK_NUM ? (disk + (DISK_NUM / 2)) % DISK_NUM : DISK_NUM);

		write_block(disk, (blkNum / (DISK_NUM/2)) + 1, data);
		write_block(diskPair, (blkNum / (DISK_NUM/2)) + 1, data);
		break;

	default:
		panic("Unsupported RAID type");
    }

    return 0;

}

int read_raid (int blkNum, unsigned char *data) {

	if (!currentRaid.valid) { return -1;}

	switch (currentRaid.raidType) {
	   case RAID0:
	       if (currentRaid.numOfDisks != currentRaid.onlineDisks) { return -1;}
	       read_block((blkNum % DISK_NUM) + 1, (blkNum / DISK_NUM) + 1, data);
	       break;

	   case RAID1:
	        if (currentRaid.onlineDisks == 0) { return -1;}
	        for (int i = 1; i <= DISK_NUM; i++) {
	            if ((currentRaid.onlineDisks >> (i-1)) & 1)
					read_block(i, blkNum, data);

	        }
	        break;

	   case RAID0_1:
	        if (currentRaid.numOfDisks != currentRaid.onlineDisks) { return -1;}

	        read_block((blkNum % (DISK_NUM/2)) + 1, (blkNum / (DISK_NUM/2)) + 1, data);
	        //read_block((blkNum % (DISK_NUM/2)) + 1 + (DISK_NUM / 2), (blkNum / (DISK_NUM/2)) + 1, data);
	        break;

	   default:
		panic("Unsupported RAID type");
	   }

    return 0;

}


int disk_fail_raid (int diskn) {
    currentRaid.onlineDisks -= (1 << (diskn - 1));

    diskmeta.raidType = currentRaid.raidType;
    diskmeta.diskNum = diskn;
    diskmeta.isActive = 0;

    write_block(diskn, 0, ((unsigned char*)&diskmeta));

    return 0;
}


int disk_repaired_raid (int diskn) {

	uchar buf[BSIZE] = {0};

    diskmeta.raidType = currentRaid.raidType;
    diskmeta.diskNum = diskn;
    diskmeta.isActive = 1;

    write_block(diskn, 0, ((unsigned char*)&diskmeta));

	switch(currentRaid.raidType){
		case RAID0:
			destroy_raid();
			break;
		case RAID1:
			for (int i = 1; i <= DISK_NUM; i++) {
				if ((currentRaid.onlineDisks >> (i-1)) & 1) {
					for (int blkNum = 0; blkNum < FSSIZE; blkNum++) {
						read_block(i, blkNum, buf);
						write_block(diskn, blkNum, buf);
					}
					i = DISK_NUM;
				}
			}
			break;

		case RAID0_1:
			int diskPair = ((diskn + (DISKS / 2)) % DISKS ? (diskn + (DISKS / 2)) % DISKS : DISKS);
			if ((currentRaid.onlineDisks >> (diskPair - 1)) & 1) {
				for (int blkNum = 1; blkNum < FSSIZE; blkNum++){
					read_block(diskPair, blkNum, buf);
					write_block(diskn, blkNum, buf);
				}
			}
			break;
	}

	if ((currentRaid.onlineDisks >> (diskn - 1)) ^ 1) currentRaid.onlineDisks += (1 << (diskn - 1));
	return 0;
}


int info_raid (uint *blkn, uint *blks, uint* diskn) {

    *blkn = FSSIZE;
    *blks = BSIZE;
    *diskn = DISK_NUM;

    return 0;
}

int destroy_raid () {
    // zeroes out all the disks

	uchar buf[BSIZE] = {0};

	for (int i = 1; i <= DISK_NUM; i++){
		for (int j = 0; j < FSSIZE; j++) {
			write_block(i, j, buf);
		}
	}

    return 0;
}


