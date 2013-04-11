#ifndef YAFFS2_STRUCT_H_
#define YAFFS2_STRUCT_H_

#include <inttypes.h>
#include <vector>
#include <map>
#include <set>

typedef std::vector<long> OffsetVector;
typedef std::map<unsigned, OffsetVector> BlockMap;
typedef std::map<unsigned, BlockMap> ObjHeaderMap;
typedef std::map<unsigned, BlockMap> DataChunkMap;
typedef std::set<unsigned> UnlinkedSet;


const int YAFFS2_OK = 0;
const int YAFFS2_FAILURE = -1;

// I metion chunk + oob as a unit
const unsigned SIZE_CHUNK = 0x800u;
const unsigned SIZE_OOB   = 0x40u;
const unsigned SIZE_UNIT  = SIZE_CHUNK + SIZE_OOB;

// offset of some useful values in a unit
const unsigned OFF_OBJ_TYPE = SIZE_CHUNK + 0x07u;
const unsigned OFF_OBJ_ID   = SIZE_CHUNK + 0x04u;
const unsigned OFF_NAME     = 0x0Au;
const unsigned OFF_SIZE     = SIZE_CHUNK + 0x0Cu;
const unsigned OFF_BLK_ID   = SIZE_CHUNK;
const unsigned OFF_CHK_ID   = SIZE_CHUNK + 0x08u;
const unsigned OFF_MTIME    = 0x120u;

// nbytes of the useful values in a unit
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

const char UNLINKED[] = "unlinked";
const char DELETED[] = "deleted";


class SuperBlock {
  public:
    SuperBlock();
    ~SuperBlock();
    int Clear(void);
    int Build(FILE *fp);
    int Recover(void);

  protected:
    ObjHeaderMap objHeaderMap_;
    DataChunkMap dataChunkMap_;

    unsigned convert(long begin, unsigned nbytes) const;
    bool isUnlinked(void) const;
    bool isDeleted(void) const;
    void setFilename(char *name);
    int recover(char *name, unsigned n_chunks, BlockMap & data_blk);

  private:
    uint8_t buf_[SIZE_UNIT];
    FILE *fp_;
};

#endif