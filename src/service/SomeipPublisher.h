#pragma once
#include "common/Signal.h"
#include "routing/SignalRouter.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <mutex>
#include <atomic>
struct SomeipServiceInfo { uint16_t serviceId, instanceId, methodId; std::string serviceName; };
struct SomeipMessage { uint16_t serviceId, methodId; uint32_t payloadSize; std::vector<uint8_t> payload; int64_t timestamp_ns; };
class SomeipPublisher {
public:
    SomeipPublisher(SignalRouter& r); ~SomeipPublisher();
    bool start(); void stop();
    size_t publishedCount() const { return pub_.load(); }
    std::vector<SomeipMessage> recentMessages() const { std::lock_guard<std::mutex> lk(mtx_); return std::vector<SomeipMessage>(msgs_.begin(), msgs_.end()); }
private:
    void onSignal(const Signal& sig);
    std::vector<uint8_t> serializeSignal(const Signal& sig) const;
    SignalRouter& router_; SubscriberId subId_; std::atomic<bool> run_{false};
    mutable std::mutex mtx_; std::deque<SomeipMessage> msgs_; std::atomic<size_t> pub_{0};
    std::unordered_map<std::string, SomeipServiceInfo> svcMap_;
    static constexpr size_t MAX_RECENT = 256;
};
