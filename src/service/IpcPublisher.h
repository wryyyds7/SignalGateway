#pragma once
#include "common/Signal.h"
#include "routing/SignalRouter.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
class IpcPublisher {
public:
    IpcPublisher(SignalRouter& r) : router_(r) {}
    ~IpcPublisher() { stop(); }
    bool start(const std::string& socketPath);
    void stop();
    size_t clientCount() const { std::lock_guard<std::mutex> lk(mtx_); return clients_.size(); }
private:
    void acceptThreadFunc();
    void onSignal(const Signal& sig);
    std::string serializeSignal(const Signal& sig) const;
    SignalRouter& router_; SubscriberId subId_; std::atomic<bool> run_{false};
    int listenFd_=-1; std::thread accThread_; mutable std::mutex mtx_; std::vector<int> clients_;
};
