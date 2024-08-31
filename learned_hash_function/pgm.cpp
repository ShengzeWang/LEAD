#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include "string"
#include "pgm/pgm_index.hpp"
namespace pgmm {
    const int epsilon = 17858;
    long long scale_factor = std::numeric_limits<unsigned long long>::max()/200000000;
    pgm::PGMIndex<uint64_t, epsilon> load_pgm(std::string data_set){
        std::vector<uint64_t> data;
        std::ifstream in("data_set", std::ios::binary);
        if (!in) {
            std::cerr << "Failed to open data file." << std::endl;
        }
        uint64_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
        data.resize(size);
        in.read(reinterpret_cast<char*>(data.data()), size * sizeof(uint64_t));
        in.close();
        std::cout << "Data loaded." << std::endl;
        // Construct the PGM-index
        pgm::PGMIndex<uint64_t, epsilon> index(data);
        std::cout << "Indexed." << std::endl;
        return index;
    }

uint64_t lookup(uint64_t q, pgm::PGMIndex<uint64_t, epsilon> index) {
    auto range = index.search(q);
    return range.pos;
}
unsigned long long RMI_hash_id(long long or_key, pgm::PGMIndex<uint64_t, epsilon> index) {
    unsigned long long hash_id = pgmm::lookup(or_key, index) * scale_factor;
    return hash_id;
}
} // namespace
