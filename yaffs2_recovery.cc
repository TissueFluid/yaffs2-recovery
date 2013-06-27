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
#include <stdio.h>
#include <unistd.h>

#include "./yaffs2_struct.h"

// Brief: do the recovering job
// Parameter:
//    img: path to the yaffs2 image
//    dst: where to put the generated files
//  Returns YAFFS2_OK on success
int yaffs2_recovery(char *img, char *dst);

// Brief: print usage
void usage(void);


int main(int argc, char * argv[]) {
  int opt;
  int status = 1; // whether the arguments meet my requests
  char *img_path = NULL;
  char *dst_path = NULL;

  if (argc == 1) {
    status = 0;
  }

  while ((opt = getopt(argc, argv, ":f:d:h")) != -1) {
    switch (opt) {
      case 'f':
        img_path = optarg;
        break;
      case 'd':
        dst_path = optarg;
        break;
      case 'h':
        status = 0;
        break;
      case ':':
        printf("Option `-%c\' needs a value.\n", optopt);
        status = 0;
        break;
      case '?':
        printf("Unknown option: %c\n", optopt);
        status = 0;
        break;
    }
  }
  if (status == 1
      && img_path != NULL
      && dst_path != NULL) {
    yaffs2_recovery(img_path, dst_path);
  } else {
    usage();
  }
  return 0;
}

int yaffs2_recovery(char *img, char *dst) {
  FILE *fp = fopen(img, "r");
  int ret;

  if (fp) {
    // backup previous working directory
    static char curr_name[FILENAME_MAX];
    getcwd(curr_name, FILENAME_MAX);

    if (chdir(dst) == -1) {
      perror(dst);
      ret = YAFFS2_FAILURE;

    } else {
      SuperBlock sb;

      if (sb.Build(fp) == YAFFS2_OK) {
        ret = (sb.Recover() == YAFFS2_OK ? YAFFS2_OK : YAFFS2_FAILURE);
        chdir(curr_name);
      } else {
        ret = YAFFS2_FAILURE;
      }
    }
    fclose(fp);

  } else {
    perror(img);
    ret = YAFFS2_FAILURE;
  }

  return ret;
}

void usage(void) {
  printf("yaffs2-recovery [-f <file>] [-d <dest>] [-h]\n"
      "  -f <file>   Recover from img <file>\n"
      "  -d <dest>   Write recovered files to <dest> directory\n"
      "  -h          Print help\n");
}
