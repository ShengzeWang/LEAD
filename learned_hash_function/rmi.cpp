#include "rmi.h"
#include "rmi_data.h"
#include <math.h>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
namespace rmi {
bool load(char const* dataPath) {
  {
    std::ifstream infile(std::filesystem::path(dataPath) / "rmi_L1_PARAMETERS", std::ios::in | std::ios::binary);
    if (!infile.good()) return false;
    L1_PARAMETERS = (char*) malloc(402653184);
    if (L1_PARAMETERS == NULL) return false;
    infile.read((char*)L1_PARAMETERS, 402653184);
    if (!infile.good()) return false;
  }
  return true;
}
void cleanup() {
    free(L1_PARAMETERS);
}

inline double cubic(double a, double b, double c, double d, double x) {
    auto v1 = std::fma(a, x, b);
    auto v2 = std::fma(v1, x, c);
    auto v3 = std::fma(v2, x, d);
    return v3;
}

inline double linear(double alpha, double beta, double inp) {
    return std::fma(beta, inp, alpha);
}

inline size_t FCLAMP(double inp, double bound) {
  if (inp < 0.0) return 0;
  return (inp > bound ? bound : (size_t)inp);
}

uint64_t lookup(uint64_t key, size_t* err) {
  size_t modelIndex;
  double fpred;
  fpred = cubic(L0_PARAMETER0, L0_PARAMETER1, L0_PARAMETER2, L0_PARAMETER3, (double)key);
  modelIndex = (uint64_t) fpred;
  fpred = linear(*((double*) (L1_PARAMETERS + (modelIndex * 24) + 0)), *((double*) (L1_PARAMETERS + (modelIndex * 24) + 8)), (double)key);
  *err = *((uint64_t*) (L1_PARAMETERS + (modelIndex * 24) + 16));

  return FCLAMP(fpred, 200000000.0 - 1.0);
}
unsigned long long RMI_hash_id(long long or_key) {
    size_t err;
    unsigned long long hash_id = rmi::lookup(or_key, &err) * scale_factor;
    return hash_id;
}
} // namespace
