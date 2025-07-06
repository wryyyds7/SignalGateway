#pragma once
#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <mutex>
class LatencyTracker {
public:
    void record(int64_t latency_ns) { std::lock_guard<std::mutex> lk(mutex_); samples_.push_back(latency_ns); }
    void report(const std::string& label) const {
        std::lock_guard<std::mutex> lk(mutex_);
        if (samples_.empty()) { std::cout << "[" << label << "] No samples\n"; return; }
        auto sorted = samples_; std::sort(sorted.begin(), sorted.end());
        int64_t sum = 0; for (auto s : sorted) sum += s;
        std::cout << "[" << label << "] (n=" << samples_.size() << ") avg=" << (double)sum/sorted.size()/1000.0
                  << "us p50=" << sorted[sorted.size()/2]/1000.0 << "us p99=" << sorted[(size_t)(sorted.size()*0.99)]/1000.0
                  << "us max=" << sorted.back()/1000.0 << "us\n";
    }
    void clear() { std::lock_guard<std::mutex> lk(mutex_); samples_.clear(); }
    size_t count() const { std::lock_guard<std::mutex> lk(mutex_); return samples_.size(); }
private:
    std::vector<int64_t> samples_;
    mutable std::mutex mutex_;
};
