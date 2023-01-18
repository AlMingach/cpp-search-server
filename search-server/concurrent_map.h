#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <string>
#include <vector>
#include <mutex>

#include "log_duration.h"


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) {
        bucket_count_ = bucket_count;
        buckets.reserve(bucket_count);
        guards.reserve(bucket_count);
        for (size_t i = 0; i < bucket_count; ++i) {
            buckets.push_back(new std::map<Key, Value>);
            guards.push_back(new std::mutex);
        }
    }

    Access operator[](const Key& key) {
        auto index = static_cast<uint64_t>(key) % bucket_count_;
        return { std::lock_guard(*guards[index]), (*buckets[index])[key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (size_t i = 0; i < bucket_count_; ++i) {
            std::lock_guard guard(*guards[i]);
            result.insert((*buckets[i]).begin(), (*buckets[i]).end());
        }
        return result;
    }

    void Erase(const Key& key) {
        auto index = static_cast<uint64_t>(key) % bucket_count_;
        {
            std::lock_guard guard(*guards[index]);
            (*buckets[index]).erase(key);
        }
    }

private:
    std::vector<std::map<Key, Value>*> buckets;
    std::vector<std::mutex*> guards;
    size_t bucket_count_;
};