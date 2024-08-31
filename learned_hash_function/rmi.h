#include <cstddef>
#include <cstdint>
#include <limits>
namespace rmi {
bool load(char const* dataPath);
void cleanup();
const size_t RMI_SIZE = 402653216;
const uint64_t BUILD_TIME_NS = 38280922293;
const char NAME[] = "rmi";
uint64_t lookup(uint64_t key, size_t* err);
unsigned long long RMI_hash_id(long long key);
}
