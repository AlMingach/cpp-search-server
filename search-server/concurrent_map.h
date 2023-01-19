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

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        auto index = static_cast<uint64_t>(key) % buckets_.size();
        return { std::lock_guard(buckets_[index].mutex), buckets_[index].bucket[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (size_t index = 0; index < buckets_.size(); ++index) {
            std::lock_guard guard(buckets_[index].mutex);
            result.insert(buckets_[index].bucket.begin(), buckets_[index].bucket.end());
        }
        return result;
    }

    void Erase(const Key& key) {
        auto index = static_cast<uint64_t>(key) % buckets_.size();
        {
            std::lock_guard guard(buckets_[index].mutex);
            buckets_[index].bucket.erase(key);
        }
    }

private:
    struct Bucket {
        std::map<Key, Value> bucket;
        std::mutex mutex;
    };

    std::vector<Bucket> buckets_;
};