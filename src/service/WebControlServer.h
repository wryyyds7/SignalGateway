#pragma once
#include "common/Signal.h"
#include "common/Logger.h"
#include <atomic>
#include <thread>
#include <string>

class SignalRouter;

class WebControlServer {
public:
    struct ManualState {
        std::atomic<double> speed{0}, rpm{800};
        std::atomic<int> gear{0};
        std::atomic<bool> leftTurn{false}, rightTurn{false}, highBeam{false}, handbrake{true}, autoMode{true};
    };
    WebControlServer(SignalRouter& r, ManualState& s) : router_(r), state_(s) {}
    ~WebControlServer() { stop(); }
    bool start(int port = 8080);
    void stop();
    size_t clientCount() const { return 0; }
private:
    void serverThreadFunc();
    void handleRequest(int fd);
    std::string getHtmlPage() const;
    SignalRouter& router_; ManualState& state_;
    std::atomic<bool> running_{false}; std::thread thread_; int listenFd_ = -1; int port_ = 8080;
};
