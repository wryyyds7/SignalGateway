#pragma once
#include "common/Signal.h"
#include "routing/SignalRouter.h"
#include "service/SomeipPublisher.h"
#include <fstream>
#include <string>
#include <mutex>
#include <atomic>
class SignalRecorder {
public:
    SignalRecorder(SignalRouter& r) : router_(r) {}
    ~SignalRecorder() { stop(); }
    bool start(const std::string& fp);
    void stop();
    bool isRecording() const { return rec_.load(); }
    size_t recordedCount() const { return cnt_.load(); }
private:
    void onSignal(const Signal& sig);
    SignalRouter& router_; SubscriberId subId_;
    std::atomic<bool> rec_{false}; std::atomic<size_t> cnt_{0};
    std::ofstream file_; std::mutex fm_;
};
class SignalReplayer {
public:
    SignalReplayer(SignalRouter& r, SomeipPublisher& p) : router_(r), pub_(p) {}
    bool replay(const std::string& fp, double speed=1.0);
    size_t replayedCount() const { return cnt_.load(); }
private:
    SignalRouter& router_; SomeipPublisher& pub_; std::atomic<size_t> cnt_{0};
};
