#include <locale.h>

#define PG_CONVERTER_IMPLEMENTATION
#include "pigaco/converter.h"

int main(int argc, char **argv) {
  setlocale(LC_ALL, "en_US.UTF-8");

  if (argc < 2) {
    fwprintf(stderr, L"%s\n", "You need to enter the name of <image file>");
    return -1;
  }

  wprintf(L"Version of the converter %d\n", pg_version());

  convert_image_to_ascii(argv[1], 8, 0.5f);

  return 0;
}
