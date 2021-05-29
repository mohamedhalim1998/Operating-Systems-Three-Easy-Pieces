#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#define T_UNLOCATED 0  // Unlocated
#define T_DIR 1        // Directory
#define T_FILE 2       // File
#define T_DEV 3        // Device

struct superblock {
   uint size;        // Size of file system image (blocks)
   uint nblocks;     // Number of data blocks
   uint ninodes;     // Number of inodes.
   uint nlog;        // Number of log blocks
   uint logstart;    // Block number of first log block
   uint inodestart;  // Block number of first inode block
   uint bmapstart;   // Block number of first free map block
};
struct dinode {
   short type;      // File type
   short major;     // Major device number (T_DEV only)
   short minor;     // Minor device number (T_DEV only)
   short nlink;     // Number of links to inode in file system
   uint size;       // Size of file (bytes)
   uint addrs[13];  // Data block addresses
};
struct dirent {
   ushort inum;
   char name[14];
};
struct superblock sb;
char *image;
struct dinode *inodes;
int *inuse_block;
int *inuse_inode;
int get_int_from_mem(char *ptr) {
   int res = (int)ptr[0] & 0xff;
   res = res | (((int)ptr[1] & 0xff) << 8);
   res = res | (((int)ptr[2] & 0xff) << 8);
   res = res | (((int)ptr[3] & 0xff) << 8);
   // printf("%02X", ptr[3] & 0xff);
   // printf("%02X", ptr[2] & 0xff);
   // printf("%02X", ptr[1] & 0xff);
   // printf("%02X\n", ptr[0] & 0xff);
   return res;
}
short get_short_from_mem(char *ptr) {
   short res = (int)ptr[0] & 0xff;
   res = res | (((int)ptr[1] & 0xff) << 8);
   // printf("%02X", ptr[1] & 0xff);
   // printf("%02X\n", ptr[0] & 0xff);
   return res;
}
void construct_super_block(char *ptr) {
   sb.size = get_int_from_mem(ptr);
   sb.nblocks = get_int_from_mem(ptr + 4);
   sb.ninodes = get_int_from_mem(ptr + 8);
   sb.inodestart = get_int_from_mem(ptr + 20);
   sb.bmapstart = get_int_from_mem(ptr + 24);
   printf(
       "super block {size = %d, data_blocks = %d, idones = %d , inode_start = "
       "%d, bitmap_start = %d}\n",
       sb.size, sb.nblocks, sb.ninodes, sb.inodestart, sb.bmapstart);
}
void construct_inodes() {
   char *inode_start = image + sb.inodestart * 512;
   inodes = malloc(sb.ninodes * sizeof(struct dinode));
   inuse_inode = malloc(sb.ninodes * sizeof(int));
   for (int i = 0; i < sb.ninodes; i++) {
      // printf("%03d-inode: %lX\n", i, (intptr_t)(inode_start - image));
      inodes[i].type = get_short_from_mem(inode_start);
      inodes[i].major = get_short_from_mem(inode_start + 2);
      inodes[i].minor = get_short_from_mem(inode_start + 4);
      inodes[i].nlink = get_short_from_mem(inode_start + 6);
      inodes[i].size = get_int_from_mem(inode_start + 8);

      if (inodes[i].type != T_UNLOCATED) inuse_inode[i] = 1;

      for (int j = 0; j < 13; j++) {
         int x = get_int_from_mem(inode_start + 12 + j * 4);
         inodes[i].addrs[j] = x;
      }
      inode_start += sizeof(struct dinode);
   }
}
void check_inode_type(struct dinode inode) {
   if (inode.type != T_DEV && inode.type != T_FILE && inode.type != T_DIR &&
       inode.type != T_UNLOCATED) {
      fprintf(stderr, "ERROR: bad inode.");
      exit(1);
   }
}
void check_inode_data_blocks(struct dinode inode) {
   for (int i = 0; i < 12; i++) {
      if (inode.addrs[i] > sb.size) {
         fprintf(stderr, "ERROR: bad direct address in inode");
         exit(1);
      }
   }
   if (inode.addrs[12] > sb.size) {
      fprintf(stderr, "ERROR: bad indirect address in inode");
      exit(1);
   }
}
void check_dir(struct dinode inode, int node_num) {
   if (node_num != 1) inuse_inode[node_num]--;
   for (int i = 0; i < 12; i++) {
      if (inode.addrs[i] == 0) {
         return;
      }
      char *dir_ptr = image + (inode.addrs[i] * 512);
      for (int p = 0; p < 512;) {
         short inum = get_short_from_mem(dir_ptr);
         char name[14];
         strncpy(name, dir_ptr + 2, 14);
         // printf("file name: %s, inum: %d\n", name, inum);
         if (inum == 0) {
            return;
         } else {
            inuse_inode[inum]++;
         }
         if (strcmp(name, ".") == 0) {
            if (node_num != inum) {
               fprintf(stderr, "ERROR: directory not properly formatted.");
               exit(1);
            }
            inuse_inode[inum]--;
         } else if (strcmp(name, "..") == 0) {
            if (node_num == 1 && node_num != inum) {
               fprintf(stderr, "ERROR: root directory does not exist.");
               exit(1);
            }
            inuse_inode[inum]--;
         }
         inum = get_short_from_mem(dir_ptr);
         strncpy(name, dir_ptr + 2, 14);
         p += sizeof(struct dirent);
         dir_ptr += sizeof(struct dirent);
      }
   }
}
void check_inodes() {
   for (int i = 0; i < sb.ninodes; i++) {
      check_inode_type(inodes[i]);
      check_inode_data_blocks(inodes[i]);
      switch (inodes[i].type) {
         case T_UNLOCATED:
            continue;
            break;
         case T_DIR:
            check_dir(inodes[i], i);
            break;
         default:
            inuse_inode[i]--;
            break;
      }
   }
   for (int i = 0; i < sb.ninodes; i++) {
      // printf("%03d: inode ref: %d TYPE: %d\n", i, inuse_inode[i],
      //  inodes[i].type);
      switch (inodes[i].type) {
         case T_UNLOCATED:
            if (inuse_inode[i] > 0) {
               fprintf(
                   stderr,
                   "ERROR: inode referred to in directory but marked free.");
               exit(1);
            }
            break;
         case T_DIR:
            if (inuse_inode[i] != 1) {
               fprintf(
                   stderr,
                   "ERROR: directory appears more than once in file system.");
               exit(1);
            }
            break;
         case T_FILE:
            if (inuse_inode[i] != 1) {
               fprintf(stderr, "ERROR: bad reference count for file.");
               exit(1);
            }
            break;
         default:
            if (inuse_inode[i] != 1) {
               fprintf(stderr,
                       "ERROR: inode marked use but not found in a directory.");
               exit(1);
            }
      }
   }
}
void mark_eserved_blocks() {
   for (int i = 0; i <= sb.bmapstart; i++) {
      inuse_block[i] = 1;
   }
}
void mark_indirect_add(int x) {
   // printf("indirect\n");
   char *ptr = image + 512 * x;

   for (int k = 0; k < 128; k++) {
      x = get_int_from_mem(ptr + k * 4);
      if (x != 0) {
         // printf("datanode %03d is in use\n", x - 58);
         inuse_block[x] += 1;
         if (inuse_block[x] > 1) {
            fprintf(stderr, "ERROR: indirect address used more than once.");
            exit(1);
         }
      }
   }
}
void mark_add() {
   for (int i = 0; i < sb.ninodes; i++) {
      for (int j = 0; j < 13; j++) {
         int x = inodes[i].addrs[j];
         if (x != 0) {
            // printf("datanode %03d is in use\n", x - 58);
            inuse_block[x] += 1;
            if (inuse_block[x] > 1) {
               fprintf(stderr, "ERROR: direct address used more than once.");
               exit(1);
            }
            if (j == 12) {
               mark_indirect_add(x);
            }
         }
      }
   }
}
void mark_used_blocks() {
   inuse_block = malloc(sb.size * sizeof(int));
   mark_eserved_blocks();
   mark_add();
}
void check_bitmap() {
   mark_used_blocks();
   // for (int i = 0; i < 1000; i++) {
   //    printf("%d", inuse_block[i]);
   //    if ((i + 1) % 8 == 0) {
   //       printf(" ");
   //    }
   // }
   // printf("\n");
   int ptr = sb.bmapstart * 512;
   for (int i = 0; i < 1000; i += 8) {
      char val = image[ptr];
      // printf("%d:", val);
      ptr++;
      for (int j = 0; j < 8; j++) {
         int mask = val & (1 << j);
         // if (mask > 0)
         //    printf("%d", 1);
         // else
         //    printf("%d", 0);
         if (mask == 0 && inuse_block[i + j] == 1) {
            fprintf(stderr,
                    "ERROR: address used by inode but marked free in bitmap.");
            exit(1);

         } else if (mask > 0 && inuse_block[i + j] == 0) {
            fprintf(stderr,
                    "ERROR: bitmap marks block in use but it is not in use.");
            exit(1);
         }
      }
      // printf(" ");
   }
}
int check(size_t len) {
   char *super_block = image + 512;
   construct_super_block(super_block);
   construct_inodes();
   check_inodes();
   check_bitmap();
   return 0;
}

int main(int argc, char const *argv[]) {
   if (argc < 2) {
      fprintf(stderr, "Usage: xcheck <file_system_image>");
      exit(1);
   }
   int fd = open(argv[1], O_RDONLY);
   struct stat statbuf;
   fstat(fd, &statbuf);
   image = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
   if (image == MAP_FAILED) {
      printf("Mapping Failed\n");
      return 1;
   }
   close(fd);
   size_t len = statbuf.st_size;
   // int i = 0;
   // for (int i = 512; i < len; i++) {
   //    printf("%x", image[i] & 0xff);
   // }
   // printf("i = %d\n", i);
   check(len);
   return 0;
}
