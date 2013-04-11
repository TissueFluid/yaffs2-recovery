#include <stdio.h>
#include <unistd.h>

#include "yaffs2_struct.h"

int yaffs2_recovery(char *img, char *dst) {
  FILE *fp = fopen(img, "r");
  int yaffs2_status;

  if (fp) {
    static char curr_name[FILENAME_MAX];
    getcwd(curr_name, FILENAME_MAX);

    if (chdir(dst) == -1) {
      perror(dst);
      yaffs2_status = YAFFS2_FAILURE;

    } else {
      SuperBlock sb;

      if (sb.Build(fp) == YAFFS2_OK ) {
        yaffs2_status = (sb.Recover() == YAFFS2_OK ? YAFFS2_OK : YAFFS2_FAILURE);
        chdir(curr_name);
      } else {
        yaffs2_status = YAFFS2_FAILURE;
      }
    }
    fclose(fp);

  } else {
    perror(img);
    yaffs2_status = YAFFS2_FAILURE;
  }

  return yaffs2_status;
}

void usage(void)
{
  printf("yaffs2-recovery [-f <file>] [-d <dest>] [-h]\n"
      "  -f <file>   Recover from img <file>\n"
      "  -d <dest>   Write recovered files to <dest> directory\n"
      "  -h          Print help\n");
}


int main(int argc, char * argv[])
{
  int opt;
  int status = 1;
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
        usage();
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
  if (status == 1) {
    yaffs2_recovery(img_path, dst_path);
  } else {
    usage();
  }
  return 0;
}
