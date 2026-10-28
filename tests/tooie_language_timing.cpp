#include "tooie_language_bundle.h"
#include "tooie_language_import.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
static std::vector<uint8_t> read(const char* p) { std::ifstream f(p, std::ios::binary); return {std::istreambuf_iterator<char>(f), {}}; }
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    auto usa = read(argv[1]); auto bundle_bytes = read(argv[2]); auto europe = read(argv[3]);
    std::string error; tooie_language_bundle::Bundle bundle; tooie_language_pack::LanguagePack pack; std::vector<uint8_t> image;
    auto start = std::chrono::steady_clock::now();
    if (!tooie_language_bundle::parse(bundle_bytes, bundle, error) || !tooie_language_bundle::build(bundle, usa, pack, error) || !tooie_language_pack::rewrite(usa, pack, image, error)) { std::cerr << error << '\n'; return 1; }
    auto end = std::chrono::steady_clock::now(); std::cout << "pt-BR " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms\n";
    start = std::chrono::steady_clock::now();
    if (!tooie_language_import::build(europe, usa, "es", pack, error)) { std::cerr << error << '\n'; return 1; }
    end = std::chrono::steady_clock::now(); std::cout << "es " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms\n";
}
