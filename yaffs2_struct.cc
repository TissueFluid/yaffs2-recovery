#include <assert.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <stack>
#include "yaffs2_struct.h"

SuperBlock::SuperBlock() {
  fp_ = NULL;
}

SuperBlock::~SuperBlock() {
}

int SuperBlock::Clear(void) {
  objHeaderMap_.clear();
  dataChunkMap_.clear();
  fp_ = NULL;
  return YAFFS2_OK;
}

int SuperBlock::Build(FILE *fp) {
  fp_ = fp;
  rewind(fp_);

  long offset = ftell(fp_);
  unsigned block_id = 0;
  unsigned obj_id = 0;

  while (fread(buf_, sizeof(uint8_t), SIZE_UNIT, fp) == SIZE_UNIT) {
    block_id = convert(OFF_BLK_ID, LEN_BLK_ID);

    if (block_id != 0 && block_id != 0xffffffff) {
      obj_id = convert(OFF_OBJ_ID, LEN_OBJ_ID);

      switch (buf_[OFF_OBJ_TYPE]) {
        case TYPE_HEADER:
          if (!isUnlinked() && !isDeleted()) {
            objHeaderMap_[obj_id][block_id].push_back(offset);
          }
          break;

        case TYPE_DATA:
          dataChunkMap_[obj_id][block_id].push_back(offset);
          break;
      }
    }
    offset = ftell(fp_);
  }
  return YAFFS2_OK;
}

int SuperBlock::Recover(void) {
  ObjHeaderMap::const_reverse_iterator obj_hdr;
  DataChunkMap::iterator data_chk;
  BlockMap::const_reverse_iterator block;
  OffsetVector::const_reverse_iterator offv;
  unsigned file_size;
  unsigned n_chunks;
  static char name[LEN_NAME + 30];

  for (obj_hdr = objHeaderMap_.rbegin();
      obj_hdr != objHeaderMap_.rend();
      obj_hdr++) {
    for (block = obj_hdr->second.rbegin();
        block != obj_hdr->second.rend() && !(dataChunkMap_.empty());
        block++) {
      for (offv = block->second.rbegin();
          offv != block->second.rend() && !(dataChunkMap_.empty());
          offv++) {
        fseek(fp_, *offv, SEEK_SET);
        fread(buf_, sizeof(uint8_t), SIZE_UNIT, fp_);
        file_size = convert(OFF_SIZE, LEN_SIZE);

        if (file_size > 0) {
          n_chunks = file_size / SIZE_CHUNK + (file_size % SIZE_CHUNK != 0);
          setFilename(name);

          if ((data_chk = dataChunkMap_.find(obj_hdr->first)) != dataChunkMap_.end()) {
            printf("%s\n%d chunks objid=%d\n", name, n_chunks, obj_hdr->first);
            if (data_chk->second.size() > 0) {
              recover(name, n_chunks, data_chk->second);
            }
          }
        }
      }
    }
  }
  return YAFFS2_OK;
}

int SuperBlock::recover(char *name, unsigned n_chunks, BlockMap &data_blk) {
  assert(n_chunks > 0);
  FILE *fout = fopen(name, "w");
  std::stack<long> data_stack;

  if (fout) {
    BlockMap::reverse_iterator block;
    OffsetVector::reverse_iterator offv;

    for (block = data_blk.rbegin();
        block != data_blk.rend() && n_chunks > 0;
        block++) {
      for (offv = block->second.rbegin();
          offv != block->second.rend() && n_chunks > 0;
          offv++, n_chunks--) {
        data_stack.push(*offv);
      }
      block->second.erase(offv.base(), block->second.end());
    }

    if (block->second.empty()) {
      block++;
    }
    data_blk.erase(block.base(), data_blk.end());

    while (!(data_stack.empty())) {
        fseek(fp_, data_stack.top(), SEEK_SET);
        fread(buf_, sizeof(uint8_t), SIZE_UNIT, fp_);
        fwrite(buf_, sizeof(uint8_t), convert(OFF_SIZE, LEN_SIZE), fout);
        data_stack.pop();
    }

    fclose(fout);
    return YAFFS2_OK;

  } else {
    perror("");
    return YAFFS2_FAILURE;
  }
}

unsigned SuperBlock::convert(long begin, unsigned nbytes) const {
  unsigned ret = (unsigned)(buf_[begin]);
  unsigned mul = 0;

  assert(nbytes > 0);
  while (--nbytes) {
    mul += 8u;
    ret |= ((unsigned)(buf_[++begin]) << mul);
  }
  return ret;
}

bool SuperBlock::isUnlinked(void) const {
  return (!strncmp(UNLINKED, (char*)(buf_ + OFF_NAME), LEN_NAME));
}

bool SuperBlock::isDeleted(void) const {
  return (!strncmp(DELETED, (char*)(buf_ + OFF_NAME), LEN_NAME));
}

void SuperBlock::setFilename(char *name) {
  strncpy(name, (char*)buf_ + OFF_NAME, LEN_NAME);

  time_t the_time = (time_t)convert(OFF_MTIME, LEN_MTIME);
  struct tm* tm_ptr = localtime(&the_time);

  sprintf(name, 
      "%s [%d-%d-%d-%02d-%02d-%02d]",
      name,
      tm_ptr->tm_year + 1900,
      tm_ptr->tm_mon + 1,
      tm_ptr->tm_mday,
      tm_ptr->tm_hour,
      tm_ptr->tm_min,
      tm_ptr->tm_sec);
  return ;
}
