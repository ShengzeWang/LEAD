#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include "string"
#include "rs/builder.h"
namespace rss {
    long long scale_factor = std::numeric_limits<unsigned long long>::max()/200000000;
    rs::RadixSpline<uint64_t> load_rs(std::string data_set){
        std::vector<uint64_t> data;
        std::ifstream in(data_set, std::ios::binary);
        if (!in) {
            std::cerr << "Failed to open data file." << std::endl;
        }
        uint64_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
        data.resize(size);
        in.read(reinterpret_cast<char*>(data.data()), size * sizeof(uint64_t));
        in.close();
        std::cout << "Data loaded." << std::endl;
        // Build RadixSpline.
        uint64_t min = data.front();
        uint64_t max = data.back();
        rs::Builder<uint64_t> rsb(min, max);
        for (const auto& key : data) rsb.AddKey(key);
        rs::RadixSpline<uint64_t> rs = rsb.Finalize();
        std::cout << "Indexed." << std::endl;
        return rs;
    }

uint64_t lookup(uint64_t q, rs::RadixSpline<uint64_t> index) {
    rs::SearchBound bound =  index.GetSearchBound(q);
    return (bound.begin + bound.end)/2;
}
unsigned long long RMI_hash_id(long long or_key, rs::RadixSpline<uint64_t> index) {
    unsigned long long hash_id = rss::lookup(or_key, index) * scale_factor;
    return hash_id;
}
} // namespace
