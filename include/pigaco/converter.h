/* * * * * * * * * * * * * * * * * * *
 *  start of header file
 */

#ifndef PIGACO_CONVERTER_H
#define PIGACO_CONVERTER_H

#include <wchar.h>

#ifndef PGDEF
#ifdef PG_CONVERTER_STATIC
#define PGDEF static
#else
#define PGDEF extern
#endif
#endif

#ifdef __cplusplus
#define pg_inline inline
#else
#define pg_inline
#endif

#define PG_VERSION 1

#define ASCII_CHARS L" .,:;irsXA253hMHGS#9B&@"
#define ASCII_CHARS_LEN ((sizeof(ASCII_CHARS) / sizeof(wchar_t)) - 1)

#ifdef PG_CONVERTER_TYPES
typedef unsigned char pgu8;
typedef unsigned int pgu32;
#else
#include <stdint.h>

typedef uint8_t pgu8;
typedef uint32_t pgu32;
#endif

struct Image {
  int width;
  int height;
  int channels;
  pgu8 *data;
};

typedef struct {
  int start_row;
  int end_row;
  int out_cols;
  int width;
  int height;
  int scale;
  int vscale;
  int use_color;
  volatile const pgu8 *image;
  volatile const pgu8 *gray;
  wchar_t **out;
} ThreadData;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef __cplusplus
namespace pg {
#endif // __cplusplus, namespace begin

PGDEF pg_inline void apply__contrast(pgu8 *gray, int width, int height,
                                     float contrast);

PGDEF pg_inline void floyd__steinberg_dither(pgu8 *gray, int width, int height);

PGDEF void *process__rows(void *arg);

PGDEF void convert_image_to_ascii(const char *filename, int scale,
                                  float aspect_ratio);

PGDEF pg_inline const pgu32 pg_version();

#ifdef __cplusplus
}
#endif // __cplusplus, namespace end

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PIGACO_CONVERTER_H

/*
 * end of header file
 * * * * * * * * * * * * * * * * * * */

#ifdef PG_CONVERTER_IMPLEMENTATION

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PG_CONVERTER_TYPES

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define PG_MALLOC(size) malloc(size)
#define PG_FREE(ptr) free(ptr)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef __cplusplus
using namespace pg;
#endif // __cplusplus

PGDEF void convert_image_to_ascii(const char *filename, int scale,
                                  float aspect_ratio) {
  struct Image *image = (struct Image *)PG_MALLOC(sizeof(struct Image));

  image->data =
      stbi_load(filename, &image->width, &image->height, &image->channels, 3);

  if (!image) {
    fwprintf(stderr, L"%s\n", stbi_failure_reason());

    PG_FREE(image);

    return;
  }

  pgu8 *gray = (pgu8 *)PG_MALLOC(image->width * image->height);
  if (!gray) {
    wprintf(L"Error allocate memory for gray.\n");

    stbi_image_free(image->data);

    PG_FREE(image);

    return;
  }

  for (int i = 0; i < image->width * image->height * 3; i += 3)
    gray[i / 3] = (pgu8)(0.299f * image->data[i] + 0.587f * image->data[i + 1] +
                         0.114f * image->data[i + 2]);

  apply__contrast(gray, image->width, image->height, 1.1f);

  floyd__steinberg_dither(gray, image->width, image->height);

  int vscale = (int)(scale / aspect_ratio);
  vscale = vscale < 1 ? 1 : vscale;

  int use_color = 1;

  int out_rows = (image->height + vscale - 1) / vscale;
  int out_cols = (image->width + scale - 1) / scale;

  wchar_t **output = (wchar_t **)PG_MALLOC(out_rows * sizeof(wchar_t *));
  memset(output, 0, out_rows * sizeof(wchar_t *));

  int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  pthread_t *threads = (pthread_t *)PG_MALLOC(num_threads * sizeof(pthread_t));
  ThreadData *thread_data =
      (ThreadData *)PG_MALLOC(num_threads * sizeof(ThreadData));

  wprintf(L"Using %d thread(s)\n", num_threads);

  int rows_per_thread = out_rows / num_threads;
  int extra_rows = out_rows % num_threads;
  int current_row = 0;

  for (int i = 0; i < num_threads; i++) {
    thread_data[i].start_row = current_row;
    thread_data[i].end_row = current_row + rows_per_thread + (i < extra_rows);
    thread_data[i].out_cols = out_cols;
    thread_data[i].width = image->width;
    thread_data[i].height = image->height;
    thread_data[i].scale = scale;
    thread_data[i].vscale = vscale;
    thread_data[i].use_color = use_color;
    thread_data[i].image = image->data;
    thread_data[i].gray = gray;
    thread_data[i].out = output;
    current_row = thread_data[i].end_row;

    if (pthread_create(&threads[i], NULL, process__rows, &thread_data[i]) != 0)
      fwprintf(stderr, L"Error create thread %d\n", i);
  }

  for (int i = 0; i < num_threads; i++)
    pthread_join(threads[i], NULL);

  for (int i = 0; i < out_rows; i++) {
    if (output[i]) {
      wprintf(L"%ls\n", output[i]);

      PG_FREE(output[i]);
    }
  }

  PG_FREE(output);
  PG_FREE(threads);
  PG_FREE(thread_data);
  PG_FREE(gray);

  stbi_image_free(image->data);

  PG_FREE(image);
}

PGDEF void apply__contrast(pgu8 *gray, int width, int height, float contrast) {
  int size = width * height;
  for (int i = 0; i < size; i++) {
    float val = gray[i];

    val = (val - 128.0f) * contrast + 128.0f;
    if (val < 0)
      val = 0;
    if (val > 255)
      val = 255;
    gray[i] = (pgu8)val;
  }
}

PGDEF void floyd__steinberg_dither(pgu8 *gray, int width, int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = y * width + x;
      int old_pixel = gray[idx];

      int level = (old_pixel * (ASCII_CHARS_LEN - 1) + 127) / 255;
      int new_pixel = level * 255 / (ASCII_CHARS_LEN - 1);
      gray[idx] = (pgu8)new_pixel;
      float error = old_pixel - new_pixel;

      if (x + 1 < width)
        gray[y * width + (x + 1)] = (pgu8)fmin(
            255, fmax(0, gray[y * width + (x + 1)] + error * 7 / 16));
      if (y + 1 < height) {
        if (x > 0)
          gray[y + 1 * width + (x - 1)] = (pgu8)fmin(
              255, fmax(0, gray[(y + 1) * width + (x - 1)] + error * 3 / 16));
        gray[(y + 1) * width + x] = (pgu8)fmin(
            255, fmax(0, gray[(y + 1) * width + x] + error * 5 / 16));
        if (x + 1 < width)
          gray[(y + 1) * width + (x + 1)] = (pgu8)fmin(
              255, fmax(0, gray[(y + 1) * width + (x + 1)] + error * 1 / 16));
      }
    }
  }
}

PGDEF void *process__rows(void *arg) {
  ThreadData *data = (ThreadData *)arg;

  for (int out_y = data->start_row; out_y < data->end_row; out_y++) {
    int y = out_y * data->vscale;
    if (y >= data->height)
      break;

    int buffer_size =
        data->use_color < 1 ? data->out_cols + 1 : data->out_cols * 24 + 1;

    wchar_t *line = (wchar_t *)PG_MALLOC(buffer_size * sizeof(wchar_t));
    if (!line) {
      fwprintf(stderr, L"Error allocate memory for line %d\n", out_y);
      continue;
    }

    line[0] = L'\0';
    int pos = 0;

    for (int x = 0; x < data->width; x += data->scale) {
      int index = y * data->width + x;
      pgu8 brightness = data->gray[index];
      int ascii_index = (brightness * (ASCII_CHARS_LEN - 1)) / 255;
      char c = ASCII_CHARS[ascii_index];

      if (data->use_color) {
        int idx_color = (y * data->width + x) * 3;
        pgu8 r = data->image[idx_color];
        pgu8 g = data->image[idx_color + 1];
        pgu8 b = data->image[idx_color + 2];

        int n = swprintf(line + pos, buffer_size - pos,
                         L"\033[38;2;%d;%d;%dm%c\033[0m", r, g, b, c);

        pos += n;
      } else {
        line[pos++] = c;
      }
    }

    line[pos] = L'\0';
    data->out[out_y] = line;
  }

  return NULL;
}

PGDEF const pgu32 pg_version() { return PG_VERSION; }

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PG_CONVERTER_IMPLEMENTATION
