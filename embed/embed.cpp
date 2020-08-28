#include <iostream>
#include <fstream>
#include <string>

static const char *template1 = R"(#include "injectable.hpp"

namespace Injectable {

const char *data = // NOLINTNEXTLINE(bugprone-string-literal-with-embedded-nul)
")";

static const char *template2 = "\";\n\nconst size_t size = ";

static const char *template3 = ";\n}\n";

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << argv[0] << " file.dll source.cpp" << std::endl;
    return 1;
  }
  std::cout << "Generating " << argv[1] << " -> " << argv[2];

  static char h[] = "0123456789ABCDEF";

  std::ifstream in;
  in.open(argv[1], std::ios::binary);

  std::ofstream out;
  out.open(argv[2], std::ios::binary | std::ios::trunc);

  out << template1;

  unsigned count = 0, data;
  while (data = in.get(), in.good()) {
    out << '\\' << 'x' << h[data >> 4u & 0x0F] << h[data >> 0u & 0x0F];
    count++;
    if (count % 100 == 0) {
      out << "\"\n\"";
    }
  }

  std::cout << " [" << count << "Bytes]" << std::endl;

  out << template2;

  out << count;

  out << template3;

  out.close();
  in.close();

  return 0;
}