// yaffs2-recovery :
//   implementation of historical data recovery on YAFFS2 filesytem
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
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stack>
#include "./yaffs2_struct.h"

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

  off_t offset = ftello(fp_);
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
    offset = ftello(fp_);
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
        fseeko(fp_, *offv, SEEK_SET);
        fread(buf_, sizeof(uint8_t), SIZE_UNIT, fp_);
        file_size = convert(OFF_SIZE, LEN_SIZE);

        if (file_size > 0) {
          n_chunks = file_size / SIZE_CHUNK + (file_size % SIZE_CHUNK != 0);
          setFilename(name);
          data_chk = dataChunkMap_.find(obj_hdr->first);

          if (data_chk != dataChunkMap_.end()) {
            printf("%s\n%d chunks objid=%d\n", name, n_chunks, obj_hdr->first);
            if (data_chk->second.size() > 0) {
              recover(name, n_chunks, &(data_chk->second));
            }
          }
        }
      }
    }
  }
  return YAFFS2_OK;
}

int SuperBlock::recover(char *name, unsigned n_chunks, BlockMap *data_blk) {
  assert(n_chunks > 0);
  FILE *fout = fopen(name, "w");
  std::stack<off_t> data_stack;

  if (fout) {
    BlockMap::reverse_iterator block;
    OffsetVector::reverse_iterator offv;

    for (block = data_blk->rbegin();
        block != data_blk->rend() && n_chunks > 0;
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
    data_blk->erase(block.base(), data_blk->end());

    while (!(data_stack.empty())) {
        fseeko(fp_, data_stack.top(), SEEK_SET);
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

unsigned SuperBlock::convert(unsigned begin, unsigned nbytes) const {
  unsigned ret = (unsigned)(buf_[begin]);
  unsigned mul = 0;

  assert(nbytes > 0);
  while (--nbytes) {
    mul += 8u;
    ret |= ((unsigned)(buf_[++begin]) << mul);
  }
  return ret;
}

int SuperBlock::isUnlinked(void) const {
  return (!memcmp(UNLINKED, (buf_ + OFF_NAME), LEN_NAME * sizeof(uint8_t)));
}

int SuperBlock::isDeleted(void) const {
  return (!memcmp(DELETED, (buf_ + OFF_NAME), LEN_NAME * sizeof(uint8_t)));
}

void SuperBlock::setFilename(char *name) {
  memcpy(name, buf_ + OFF_NAME, LEN_NAME * sizeof(uint8_t));

  time_t the_time = (time_t)convert(OFF_MTIME, LEN_MTIME);
  struct tm tm_st;

  localtime_r(&the_time, &tm_st);

  snprintf(name, LEN_NAME + 30,
      "%s [%d-%d-%d-%02d-%02d-%02d]",
      name,
      tm_st.tm_year + 1900,
      tm_st.tm_mon + 1,
      tm_st.tm_mday,
      tm_st.tm_hour,
      tm_st.tm_min,
      tm_st.tm_sec);
}
