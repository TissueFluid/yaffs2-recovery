// yaffs2-recovery :
//   implementation of historical data recovery on YAFFS2
//
// Copyright (C) Zu Zhiyue <zuzhiyue@gmail.com>
//
// This file is part of yaffs2-recovery.
//
//    yaffs2-recovery is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    yaffs2-recovery is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with yaffs2-recovery.  If not, see <http://www.gnu.org/licenses/>.
#ifndef YAFFS2_STRUCT_H_
#define YAFFS2_STRUCT_H_

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include <vector>
#include <map>
#include <set>

// It's hard to explain the following things :(
// Offsetvector is a vector which stores some chunks' offset
// BlockMap maps block id to Offsetvector
// ObjheaderMap maps object id to object header chunks' BlockMap
// DataChunkMap maps object id to data chunks' BlockMap
// Unlinkedset stores the object ids of the unlinked files
typedef std::vector<off_t> OffsetVector;
typedef std::map<unsigned, OffsetVector> BlockMap;
typedef std::map<unsigned, BlockMap> ObjHeaderMap;
typedef std::map<unsigned, BlockMap> DataChunkMap;
typedef std::set<unsigned> UnlinkedSet;

// valid return values of some functions
const int YAFFS2_OK = 0;
const int YAFFS2_FAILURE = -1;

// I metion chunk + oob as a unit
const unsigned SIZE_CHUNK = 0x800u;
const unsigned SIZE_OOB   = 0x40u;
const unsigned SIZE_UNIT  = SIZE_CHUNK + SIZE_OOB;

// offset of some specific values in a unit
const unsigned OFF_OBJ_TYPE = SIZE_CHUNK + 0x07u;
const unsigned OFF_OBJ_ID   = SIZE_CHUNK + 0x04u;
const unsigned OFF_NAME     = 0x0Au;
const unsigned OFF_SIZE     = SIZE_CHUNK + 0x0Cu;
const unsigned OFF_BLK_ID   = SIZE_CHUNK;
const unsigned OFF_CHK_ID   = SIZE_CHUNK + 0x08u;
const unsigned OFF_MTIME    = 0x120u;

// size of the corresponding values above
const unsigned LEN_OBJ_TYPE = 1u;
const unsigned LEN_OBJ_ID   = 2u;
const unsigned LEN_NAME     = 255u;
const unsigned LEN_SIZE     = 4u;
const unsigned LEN_BLK_ID   = 4u;
const unsigned LEN_CHK_ID   = 4u;
const unsigned LEN_MTIME    = 4u;

// There exists six types of files in yaffs2.
// Each has an 8-bit value.
const uint8_t TYPE_DATA     = 0x0;
const uint8_t TYPE_HEADER   = 0x10;
const uint8_t TYPE_SIMLINK  = 0x20;
const uint8_t TYPE_DIR      = 0x30;
const uint8_t TYPE_HARDLINK = 0x40;
const uint8_t TYPE_SPECIAL  = 0x50;

// Flag of unlinked file
const uint8_t UNLINKED[] = {
  'u', 'n', 'l', 'i', 'n', 'k', 'e', 'd'
};

// Flag of deleted file
const uint8_t DELETED[] = {
  'd', 'e', 'l', 'e', 't', 'e', 'd'
};

// By reading a yaffs2 image, this structure would reorganize the whole
// image's data in order that each file in the image is mapped to its
// corresponding chunks, and finally recover all of the files as well as
// their historical versions as much as possible (depends on whether the
// corresponding chunk is erased by yaffs2).
// Sample usage:
//    FILE *fp = fopen(path_to_img, "r");
//    Superblock sb;
//    if (sb.Build(fp) == YAFFS2_OK) {
//      if (sb.Recover() == YAFFS2_OK) {
//        printf("Success!\n");
//      } else {
//        printf("Recover error!\n");
//      }
//    } else {
//      printf("Build error!\n");
//    }
class SuperBlock {
  public:
    SuperBlock();
    ~SuperBlock();

    // Brief: reset the values of member variables to 
    // Returns YAFFS2_OK on success
    int Clear(void);

    // Brief: To prepare for the subsequent recovery work, this function
    //        reorganize the image file's data and build a structure of
    //        mappings, which map each file to its corresponding chunks.
    // Parameter: 
    //    fp: the image's file descriptor
    // Returns YAFFS2_OK on success
    int Build(FILE *fp);

    // Brief: After successfully calling Build() above, the member variables
    //        have been assigned correctly. This function will try its best to
    //        recover all files of the file image as well as their historical
    //        versions(if they exists).
    // Returns YAFFS2_OK on success
    int Recover(void);

  protected:
    // maps a file's object id to its object header chunks
    ObjHeaderMap objHeaderMap_;
    // maps a file's object id to its data chunks
    //
    DataChunkMap dataChunkMap_;

    // Brief: This function converts little endian to an unsigned value
    // Parameter:
    //    begin: the offset of the first byte of a specific value
    //    nbytes: the count of byte of the specific value
    // Returns the result after the conversion
    unsigned convert(unsigned begin, unsigned nbytes) const;

    // Returns non-zero if the current object header chunk pointed by fp is an
    // unlinked/deleted chunk
    int isUnlinked(void) const;
    int isDeleted(void) const;

    // Brief: This function sets the name of the file to be recovered. The
    //        filename consists two parts: orignal name and mtime. 
    // Parameter:
    //    name: the recovered filename will be copied to this parameter. 
    void setFilename(char *name);

    // Breif: This function creates a new file named $name and this new file's
    //        content is the last $n_chunks chunks of $data_blk.
    //        (word begins with $ is a parameter)
    // Parameter:
    //    name: the name of the recovered file
    //    n_chunks: the capacity of the recovered file
    //    data_blk: a structure that stores info on data chunks
    // Returns YAFFS2_OK on success
    int recover(char *name, unsigned n_chunks, BlockMap *data_blk);

  private:
    // A buffer which read a chunk's data every time
    uint8_t buf_[SIZE_UNIT];

    // This points to the image file.
    FILE *fp_;
};

#endif  // YAFFS2_STRUCT_H_
