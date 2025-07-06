#pragma once
#include "common/Signal.h"
#include "common/Logger.h"
#include "common/Timing.h"
#include <functional>
#include <mutex>
#include <vector>
#include <string>
#include <atomic>
#include <algorithm>
using SubscriberId = uint64_t;
class SignalRouter {
public:
    using SignalHandler = std::function<void(const Signal&)>;
    SignalRouter() {} ~SignalRouter() { std::lock_guard<std::mutex> lk(mtx_); subs_.clear(); }
    SubscriberId subscribe(const std::string& name, SignalHandler h) { std::lock_guard<std::mutex> lk(mtx_); auto id=nid_++; subs_.push_back({id,name,std::move(h)}); return id; }
    SubscriberId subscribeAll(SignalHandler h) { std::lock_guard<std::mutex> lk(mtx_); auto id=nid_++; subs_.push_back({id,"",std::move(h)}); return id; }
    void unsubscribe(SubscriberId id) { std::lock_guard<std::mutex> lk(mtx_); subs_.erase(std::remove_if(subs_.begin(),subs_.end(),[id](const Sub&s){return s.id==id;}),subs_.end()); }
    void publish(const Signal& sig) { pub_++; std::vector<SignalHandler> call; { std::lock_guard<std::mutex> lk(mtx_); for(auto&s:subs_) if(s.name.empty()||s.name==sig.name) call.push_back(s.h); } for(auto&h:call) h(sig); }
    size_t subscriberCount() const { std::lock_guard<std::mutex> lk(mtx_); return subs_.size(); }
    size_t publishedCount() const { return pub_.load(); }
    LatencyTracker& latency() { return lat_; }
private:
    struct Sub { SubscriberId id; std::string name; SignalHandler h; };
    mutable std::mutex mtx_; std::vector<Sub> subs_; std::atomic<SubscriberId> nid_{1}; std::atomic<size_t> pub_{0}; LatencyTracker lat_;
};
