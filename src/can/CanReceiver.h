#pragma once
#include "CanFrame.h"
#include "common/Signal.h"
#include "service/WebControlServer.h"
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <string>

struct DbcSignalDef {
    std::string name; uint8_t startBit=0, bitLength=0; bool isLittleEndian=true, isSigned=false;
    double factor=1.0, offset=0.0; char type='d';
};
struct DbcMessageDef { uint32_t id=0; std::string name; std::vector<DbcSignalDef> signals; };

class CanReceiver {
public:
    using SignalCallback = std::function<void(const Signal&)>;
    CanReceiver(); ~CanReceiver();
    void setManualState(WebControlServer::ManualState* s) { manualState_ = s; }
    void setSignalCallback(SignalCallback cb) { std::lock_guard<std::mutex> lk(cbMutex_); callback_ = std::move(cb); }
    void loadBuiltinDbc();
    bool start(const std::string& channel = "virtual", bool autoSim = true);
    void stop();
    void injectFrame(const CanFrame& frame);
    bool isRunning() const { return running_.load(); }
private:
    void rxThreadFunc(); void simThreadFunc(); void parseFrame(const CanFrame& frame);
    double extractSignal(const std::vector<uint8_t>& data, const DbcSignalDef& sig) const;
    SignalCallback callback_; std::mutex cbMutex_;
    std::atomic<bool> running_{false}; std::thread rxThread_, simThread_;
    std::mutex qMutex_; std::condition_variable qCv_; std::queue<CanFrame> queue_;
    std::unordered_map<uint32_t, DbcMessageDef> dbc_;
    WebControlServer::ManualState* manualState_ = nullptr;
    int simTick_=0; double simSpeed_=0, simRpm_=0, simCoolant_=40, simFuel_=75;
    int simGear_=0; bool simLeftTurn_=false, simRightTurn_=false, simHighBeam_=false, simHandbrake_=true; int simOdometer_=0;
    CanFrame generateEngineFrame(); CanFrame generateTransmissionFrame(); CanFrame generateBodyFrame();
    static constexpr size_t MAX_QUEUE = 1024;
};
