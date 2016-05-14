/*
 * =====================================================================================
 *
 *       Filename:  fscheck.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/07/2016 07:40:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YING FANG (), fang42@wisc.edu
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

//similar to vsfs
//superblock | inode table | bitmap | data blocks
//
//block 1 is super block
//inodes start at block 2

#define ROOTINO 1 //root inode
#define BSIZE 512 //block size

#define NDIRECT (12)
#define IPB (BSIZE / sizeof(struct dinode))

//fs super block
struct superblock {
    uint size;  //fs size (blocks)
    uint nblocks;  //number of blocks
    uint ninodes;  //number of inodes
};

struct dinode {
    short type; //file type
    short major; //major device number
    short minor; //minor device number
    short nlink; //number of links to inode
    uint size; //size of file(bytes)
    uint addrs[NDIRECT+1]; //data block addresses
};

struct dirent {
    ushort inum;
    char name[14];
};

int main(int argc, char *argv[]) {

    int fd = open(argv[1], O_RDONLY);
    // check bad file
    if(fd <= -1){
        fprintf(stderr, "image not found.\n");
        exit(1);
    }

    int rc;
    struct stat sbuf;
    rc = fstat(fd, &sbuf);
    assert (rc == 0);

    // use mmap()
    void *img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert (img_ptr != MAP_FAILED);

    struct superblock *sb;
    sb = (struct superblock *)(img_ptr + BSIZE);

    void *bitmap_ptr = (void *)(img_ptr + (sb->ninodes/IPB + 3)*BSIZE);

    //inodes
    int i;
    struct dinode *dip = (struct dinode *)(img_ptr + 2*BSIZE);
    // create an in_use map to compare with bitmap
    int in_use[sb->size];
    int usedblocks = sb->ninodes/IPB + 3 + (sb->size/(512*8)+1);
    // initialize the in_use array
    int k;
    for(k = 0; k < sb->size; k++ ) {
        in_use[k] = 0;
    }
    k = 0;
    while (k < usedblocks) {
        in_use[k] = 1;
        k++;
    }

    int nlinks[sb->ninodes];
    int inode_use[sb->ninodes];
    int inode_use_dir[sb->ninodes];

    for (i = 0; i < sb->ninodes; i++) {

        int indir = 0;
        if (dip->size > 512 * 12) {
            indir = 1;
        }

        // 1.check valid type
        if (dip->type > 3 || dip->type < 0) {
            fprintf(stderr, "ERROR: bad inode.\n");
            exit(1);
        }

        // 3.check root inode
        if (i == 1) {
            if (dip->type != 1) {
                fprintf(stderr, "ERROR: root directory does not exist.\n");
                exit(1); 
            }
            if (dip->addrs[0] == 0) {
                fprintf(stderr, "ERROR: root directory does not exist.\n");
                exit(1); 
            }
            struct dirent *direntPtr = (struct dirent *)(img_ptr + dip->addrs[0]*BSIZE);
            if (direntPtr->inum == 0) {
                fprintf(stderr, "ERROR: root directory does not exist.\n");
                exit(1); 
            }
        }
        int j;
        for (j = 0; j < 13; j++) {
            if (dip->addrs[j] == 0) {

                break;
            }
            // 2.check bad address within inode
            if(dip->addrs[j] > 1023 || dip->addrs[j] < 3){
                fprintf(stderr, "ERROR: bad address in inode.\n");
                exit(1);
            }
            // 6.check in-use block in bitmap
            int bitIdx = dip->addrs[j];
            char fetchedByte = *( (char*)bitmap_ptr + ( bitIdx >> 3 ) );
            int fetchedBit = ( fetchedByte >> (bitIdx & 7) ) & 1;
            if (fetchedBit == 0) {
                fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                exit(1);
            }
            if (in_use[bitIdx] == 1) {
                fprintf(stderr, "ERROR: address used more than once.\n");
                exit(1);
            } else {
                in_use[bitIdx] = 1;
            }

        }

        if (indir == 1) {
            uint *indirPtr = (uint *)(img_ptr + dip->addrs[NDIRECT]*BSIZE);
            for (j = 0; j < BSIZE/sizeof(uint); j++) {
                uint bitIdx = *indirPtr;
                if (bitIdx == 0) {
                    break;
                }
                if (bitIdx > 1023 || bitIdx < 0) {
                    fprintf(stderr, "ERROR: bad address in inode.\n");
                    exit(1);
                }
                char fetchedByte = *( (char*)bitmap_ptr + ( bitIdx >> 3 ) );
                int fetchedBit = ( fetchedByte >> (bitIdx & 7) ) & 1;
                if (fetchedBit == 0) {
                    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                    exit(1);
                }
                if (in_use[bitIdx] == 1) {
                    fprintf(stderr, "ERROR: address used more than once.\n");
                    exit(1);
                } else {
                    in_use[bitIdx] = 1;
                }
                indirPtr++;
            }
        }

        // 4.check directory format
        if (dip->type == 1) {
            void * blkAddr = img_ptr + dip->addrs[0] * BSIZE;
            struct dirent *direntPtr = (struct dirent*)blkAddr;
            if (strcmp(direntPtr->name, ".") != 0) {
                fprintf(stderr, "ERROR: directory not properly formatted.\n");
                exit(1);
            }
            direntPtr++;
            if (strcmp(direntPtr->name, "..") != 0) {
                fprintf(stderr, "ERROR: directory not properly formatted.\n");
                exit(1);
            }
            // 5.check parent directory
            if (i != 1 && direntPtr->inum == i) {
                fprintf(stderr, "ERROR: parent directory mismatch.\n");
                exit(1);
            }
            struct dinode *parentDip = (struct dinode *)(img_ptr + 2*BSIZE + direntPtr->inum*sizeof(struct dinode));
            if (parentDip->type != 1) {
                fprintf(stderr, "ERROR: parent directory mismatch.\n");
                exit(1);
            }
            int match = 0;
            int m;
            for (m = 0; m < 12; m++) {
                if (parentDip->addrs[m] == 0) {
                    break; }
                struct dirent *direntPtr = (struct dirent *)(img_ptr + parentDip->addrs[m]*BSIZE);
                int n;
                for (n = 0; n < BSIZE/sizeof(struct dirent); n++) {
                    if (direntPtr->inum == 0) {break;}
                    if (direntPtr->inum == i ) { match = 1;}
                    direntPtr++;
                }
            }
            if (match == 0) {
                if (i == 1) {
                    fprintf(stderr, "ERROR: root directory does not exist.\n");
                    exit(1); 
                }
                fprintf(stderr, "ERROR: parent directory mismatch.\n");
                exit(1);
            }

            int p;
            for (p = 0; p < 12; p++) {
                if (dip->addrs[p] == 0) {
                    break;
                }
                int q;
                // mark inode_use_dir map
                blkAddr = img_ptr + dip->addrs[p]*BSIZE;
                direntPtr = (struct dirent *)blkAddr;
                for (q = 0; q<BSIZE/sizeof(struct dirent *);p++) {
                    if (direntPtr->inum == 0) {break;}
                    inode_use_dir[direntPtr->inum] = 1;
                    //count link
                    if (strcmp(direntPtr->name,".") != 0 && strcmp(direntPtr->name,"..") != 0) {
                        nlinks[direntPtr->inum]++;
                    }
                    direntPtr++;
                }
            }

        }
        // printf("%u %u\n", dip->addrs[0],dip->addrs[1]);
        dip++;
    }

    nlinks[1]++;
    // 9. used inode in directory
    // 10. inode referred to in directory marked used
    dip = (struct dinode *)(img_ptr + 2*BSIZE);
    for (i = 0; i < sb->ninodes; i++) {
        if (dip->type != 0) {
            inode_use[i] = 1;
        }
        dip++;
    }
    for (i = 0; i < sb->ninodes; i++) {
        if (inode_use[i] == 1 && inode_use_dir[i] == 0) {
            fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
            exit(1);
        }
        if (inode_use_dir[i] == 1 && inode_use[i] == 0) {
            fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
            exit(1);
        }
    }
    dip = (struct dinode *)(img_ptr + 2*BSIZE);
    for (i = 0; i < sb->ninodes; i++) {
        if (dip->type == 1 && nlinks[i] != 1) {
            fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
            exit(1);
        }
        if (nlinks[i] != dip->nlink) {
            fprintf(stderr, "ERROR: bad reference count for file.\n");
            exit(1);
        }
        dip++;
    }

    //7.check marked bitmap
    int b;
    for(b = 0; b < sb->size; b++ ) {
        char fetchedByte = *( (char*)bitmap_ptr + ( b >> 3 ) );
        int fetchedBit = ( fetchedByte >> (b & 7) ) & 1;
        if (fetchedBit == 1) {
            if (in_use[b] == 0) {
                fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
                exit(1);
            }
        }
    }
    return 0;
}
