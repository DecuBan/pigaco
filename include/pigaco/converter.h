#ifndef PIGACO_CONVERTER_H
#define PIGACO_CONVERTER_H

#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#define STB_IMAGE_IMPLEMENTATION
#include "pigaco/stb_image.h"

#define ASCII_CHARS L" .,:;irsXA253hMHGS#9B&@"
#define ASCII_CHARS_LEN ((sizeof(ASCII_CHARS) / sizeof(wchar_t)) - 1)

struct Image {
  int width;
  int height;
  int channels;
  unsigned char *data;
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
  const unsigned char *image;
  const unsigned char *gray;
  wchar_t **out;
} ThreadData;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static void apply_contrast(unsigned char *gray, int width, int height,
                           float contrast) {
  int size = width * height;
  for (int i = 0; i < size; i++) {
    float val = gray[i];

    val = (val - 128.0f) * contrast + 128.0f;
    if (val < 0)
      val = 0;
    if (val > 255)
      val = 255;
    gray[i] = (unsigned char)val;
  }
}

static void floyd_steinberg_dither(unsigned char *gray, int width, int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = y * width + x;
      int old_pixel = gray[idx];

      int level = (old_pixel * (ASCII_CHARS_LEN - 1) + 127) / 255;
      int new_pixel = level * 255 / (ASCII_CHARS_LEN - 1);
      gray[idx] = (unsigned char)new_pixel;
      float error = old_pixel - new_pixel;

      if (x + 1 < width)
        gray[y * width + (x + 1)] = (unsigned char)fmin(
            255, fmax(0, gray[y * width + (x + 1)] + error * 7 / 16));
      if (y + 1 < height) {
        if (x > 0)
          gray[y + 1 * width + (x - 1)] = (unsigned char)fmin(
              255, fmax(0, gray[(y + 1) * width + (x - 1)] + error * 3 / 16));
        gray[(y + 1) * width + x] = (unsigned char)fmin(
            255, fmax(0, gray[(y + 1) * width + x] + error * 5 / 16));
        if (x + 1 < width)
          gray[(y + 1) * width + (x + 1)] = (unsigned char)fmin(
              255, fmax(0, gray[(y + 1) * width + (x + 1)] + error * 1 / 16));
      }
    }
  }
}

static void *process_rows(void *arg) {
  ThreadData *data = (ThreadData *)arg;

  for (int out_y = data->start_row; out_y < data->end_row; out_y++) {
    int y = out_y * data->vscale;
    if (y >= data->height)
      break;

    int buffer_size;
    if (data->use_color)
      buffer_size = data->out_cols * 24 + 1;
    else
      buffer_size = data->out_cols + 1;

    wchar_t *line = malloc(buffer_size * sizeof(wchar_t));
    if (!line) {
      fwprintf(stderr, L"Error allocate memory for line %d\n", out_y);
      continue;
    }

    line[0] = L'\0';
    int pos = 0;

    for (int x = 0; x < data->width; x += data->scale) {
      int index = y * data->width + x;
      unsigned char brightness = data->gray[index];
      int ascii_index = (brightness * (ASCII_CHARS_LEN - 1)) / 255;
      char c = ASCII_CHARS[ascii_index];

      if (data->use_color) {
        int idx_color = (y * data->width + x) * 3;
        unsigned char r = data->image[idx_color];
        unsigned char g = data->image[idx_color + 1];
        unsigned char b = data->image[idx_color + 2];

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

static void convert_image_to_ascii(const char *filename) {
  struct Image *image = (struct Image *)malloc(sizeof(struct Image) + 16);

  image->data =
      stbi_load(filename, &image->width, &image->height, &image->channels, 3);

  if (!image) {
    fwprintf(stderr, L"%s\n", stbi_failure_reason());

    return;
  }

  unsigned char *gray = malloc(image->width * image->height);
  if (!gray) {
    stbi_image_free(image->data);

    wprintf(L"Error allocate memory for gray.\n");

    return;
  }

  for (int y = 0; y < image->height; y++) {
    for (int x = 0; x < image->width; x++) {
      int idx = (y * image->width + x) * 3;

      unsigned char r = image->data[idx];
      unsigned char g = image->data[idx + 1];
      unsigned char b = image->data[idx + 2];

      float lum = 0.299f * r + 0.587f * g + 0.114f * b;

      gray[y * image->width + x] = (unsigned char)lum;
    }
  }

  float contrast_factor = 1.1f;
  apply_contrast(gray, image->width, image->height, contrast_factor);

  floyd_steinberg_dither(gray, image->width, image->height);

  int scale = 8;
  float aspect_ratio = 0.5f;
  int vscale = (int)(scale / aspect_ratio);

  if (vscale < 1)
    vscale = 1;

  int use_color = 1;

  int out_rows = (image->height + vscale - 1) / vscale;
  int out_cols = (image->width + scale - 1) / scale;

  int num_threads = 1;
#ifdef _SC_NPROCESSORS_ONLN
  long cpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (cpus > 1)
    num_threads = (int)cpus;
#endif
  wprintf(L"Using %d thread(s)\n", num_threads);

  wchar_t **output = malloc(out_rows * sizeof(wchar_t *));
  if (!output) {
    wprintf(L"Error allocate memory for output.\n");

    free(gray);

    stbi_image_free(image->data);

    free(image);

    return;
  }

  memset(output, 0, out_rows * sizeof(wchar_t *));

  pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
  ThreadData *thread_data = malloc(num_threads * sizeof(ThreadData));
  if (!threads || !thread_data) {
    fwprintf(stderr, L"Error allocate memory for threads.\n");

    free(output);
    free(gray);

    stbi_image_free(image->data);

    free(image);

    return;
  }

  int rows_per_thread = out_rows / num_threads;
  int extra_rows = out_rows % num_threads;
  int current_row = 0;

  for (int i = 0; i < num_threads; i++) {
    int start = current_row;
    int add = (i < extra_rows) ? 1 : 0;
    int end = start + rows_per_thread + add;

    thread_data[i].start_row = start;
    thread_data[i].end_row = end;
    thread_data[i].out_cols = out_cols;
    thread_data[i].width = image->width;
    thread_data[i].height = image->height;
    thread_data[i].scale = scale;
    thread_data[i].vscale = vscale;
    thread_data[i].use_color = use_color;
    thread_data[i].image = image->data;
    thread_data[i].gray = gray;
    thread_data[i].out = output;
    current_row = end;

    if (pthread_create(&threads[i], NULL, process_rows, &thread_data[i]) != 0)
      fwprintf(stderr, L"Error create thread %d\n", i);
  }

  for (int i = 0; i < num_threads; i++)
    pthread_join(threads[i], NULL);

  for (int i = 0; i < out_rows; i++) {
    if (output[i]) {
      wprintf(L"%ls\n", output[i]);

      free(output[i]);
    }
  }

  free(output);
  free(threads);
  free(thread_data);
  free(gray);

  stbi_image_free(image->data);

  free(image);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PIGACO_CONVERTER_H
