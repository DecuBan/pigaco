#include <locale.h>

#include "pigaco/converter.h"

int main(int argc, char **argv) {
  setlocale(LC_ALL, "en_US.UTF-8");

  if (argc < 2) {
    fwprintf(stderr, L"%s\n", "You need to enter the name <image file>");
    return -1;
  }

  convert_image_to_ascii(argv[1]);

  return 0;
}
