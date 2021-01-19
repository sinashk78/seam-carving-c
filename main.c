#define _GNU_SOURCE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <float.h>
#include <stdarg.h>
#include "lib/gifenc/gifenc.h"
#include "includes/seam_carve.h"
#include "lib/stb-image/stb_image.h"
#include "lib/stb-image/stb_image_write.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NANO 1000000000

char *create_str(const char *format, ...)
{
  va_list args;
  char *buffer = calloc(255, sizeof(char));
  va_start(args, format);
  vsnprintf(buffer, 255, format, args);
  va_end(args);

  return buffer;
}

int main(int argc, char const *argv[])
{
  if (argc != 6)
  {
    printf("too few arguments!");
    return 1;
  }
  const char *filename = argv[1];
  int target_width = atoi(argv[2]);
  int target_height = atoi(argv[3]);
  MODE_T mode = atoi(argv[4]) == 0 ? BACKWARD : FORWARD;
  bool save_gif = atoi(argv[5]) == 1;

  if (target_height < 0 || target_width < 0)
  {
    printf("invalid pixels.\n");
    return 1;
  }

  int w, h, ch;
  pixel3_t *img = (pixel3_t *)stbi_load(filename, &w, &h, &ch, CHANNEL);

  if (img == NULL)
  {
    printf("image doesn't exist.\n");
    stbi_image_free(img);
    return 1;
  }
  if (h - target_height < 3 || w - target_width < 3)
  {
    printf("target size is too close(or bigger) to the actual size. why?! :(\n");
    stbi_image_free(img);
    return 1;
  }
  if (target_height == 0 && target_width == 0)
  {
    printf("target size is 0. No point in running the program!\n");
    stbi_image_free(img);
    return 1;
  }

  struct stat st = {0};

  if (stat("output/", &st) == -1)
  {
    mkdir("output/", 0700);
  }

  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);

  printf("calculation in progress...");
  fflush(stdout);
  seam_carve_t *sc = seam_carve_init(img, w, h, target_width, target_height, mode, save_gif);
  while (has_next(sc))
  {
    printf("i:%d , j: %d\n", sc->current_h, sc->current_w);
    calculate_energy(sc);
    if (has_vseam(sc))
    {
      find_vseam(sc);
      draw_vseam(sc);
      remove_vseam(sc);
      // find_vseam_forward(&vseam, w - j, h - i, c, &min_table, &energy_img);
      // draw_vseam(energy_img, vseam, w - j, h - i, i < target_height ? NULL : gif);
      // remove_vseam(&img, vseam, w - (j + 1), h - i);
    }
    if (has_hseam(sc))
    {
      find_hseam(sc);
      draw_hseam(sc);
      remove_hseam(sc);
      // find_hseam(&hseam, w - j, h - i, e, &min_table);
      // draw_hseam(energy_img, hseam, w - j, h - i, gif);
      // remove_hseam(&img, hseam,
      //              j < target_width ? w - (j + 1) : w - target_width,
      //              h - (i + 1));
    }

    next_seam(sc);
  }

  clock_gettime(CLOCK_REALTIME, &end);
  printf("\nrunnig time(seconds): %lf\n",
         end.tv_sec - start.tv_sec +
             (double)(end.tv_nsec - start.tv_nsec) / NANO);

  stbi_write_png("output/result.png",
                 w - target_width,
                 h - target_height,
                 CHANNEL,
                 sc->img,
                 (w - target_width) * CHANNEL);

  // free resources
  seam_carve_free(sc);
  return 0;
}
