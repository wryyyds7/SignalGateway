#include "CanReceiver.h"
#include "common/Logger.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CanReceiver::CanReceiver() { loadBuiltinDbc(); }
CanReceiver::~CanReceiver() { stop(); }

void CanReceiver::loadBuiltinDbc() {
    dbc_[0x100] = {0x100, "Engine", {{"EngineRPM",0,16,true,false,0.25,0,'d'},{"CoolantTemp",16,8,true,false,1.0,-40,'d'},{"FuelLevel",24,8,true,false,0.4,0,'d'}}};
    dbc_[0x200] = {0x200, "Transmission", {{"Gear",0,8,true,true,1.0,0,'i'},{"VehicleSpeed",8,16,true,false,0.01,0,'d'}}};
    dbc_[0x400] = {0x400, "Body", {{"LeftTurnSignal",0,1,true,false,1.0,0,'b'},{"RightTurnSignal",1,1,true,false,1.0,0,'b'},{"HighBeam",2,1,true,false,1.0,0,'b'},{"Handbrake",3,1,true,false,1.0,0,'b'}}};
    LOG_INFO("DBC loaded: " << dbc_.size() << " messages");
}

bool CanReceiver::start(const std::string& channel, bool autoSim) {
    running_.store(true);
    rxThread_ = std::thread(&CanReceiver::rxThreadFunc, this);
    if (autoSim) simThread_ = std::thread(&CanReceiver::simThreadFunc, this);
    LOG_INFO("CAN receiver started on '" << channel << "'");
    return true;
}

void CanReceiver::stop() {
    running_.store(false); qCv_.notify_all();
    if (rxThread_.joinable()) rxThread_.join();
    if (simThread_.joinable()) simThread_.join();
}

void CanReceiver::injectFrame(const CanFrame& f) {
    { std::lock_guard<std::mutex> lk(qMutex_); if (queue_.size()>=MAX_QUEUE) queue_.pop(); queue_.push(f); }
    qCv_.notify_one();
}

void CanReceiver::rxThreadFunc() {
    while (running_.load()) {
        CanFrame f;
        { std::unique_lock<std::mutex> lk(qMutex_);
          qCv_.wait_for(lk, std::chrono::milliseconds(100), [this]{return !queue_.empty()||!running_.load();});
          if (!running_.load()) return; if (queue_.empty()) continue; f = queue_.front(); queue_.pop(); }
        f.timestamp_ns = Signal::now(); parseFrame(f);
    }
}

void CanReceiver::simThreadFunc() {
    while (running_.load()) {
        ++simTick_;
        if (manualState_ && !manualState_->autoMode.load()) {
            simSpeed_=manualState_->speed.load(); simRpm_=manualState_->rpm.load();
            simGear_=manualState_->gear.load(); simLeftTurn_=manualState_->leftTurn.load();
            simRightTurn_=manualState_->rightTurn.load(); simHighBeam_=manualState_->highBeam.load();
            simHandbrake_=manualState_->handbrake.load();
            injectFrame(generateEngineFrame()); injectFrame(generateTransmissionFrame()); injectFrame(generateBodyFrame());
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); continue;
        }
        int c = simTick_ % 600;
        if (c<200) { double t=(double)c/200.0; simSpeed_=200.0*(1.0-std::cos(t*M_PI))*0.5; }
        else if (c<400) simSpeed_=200.0+std::sin(c*0.1)*2.0;
        else { double t=(double)(c-400)/200.0; simSpeed_=200.0*(1.0+std::cos(t*M_PI))*0.5; }
        simGear_=std::min(8,(int)(simSpeed_/30.0)); if(simSpeed_<1) simGear_=0;
        double gr=(simGear_>0)?30.0*simGear_:10.0; simRpm_=std::min(7500.0,(simSpeed_/gr)*6000.0);
        if(simRpm_<800) simRpm_=800+std::sin(simTick_*0.3)*100;
        simCoolant_=std::min(105.0,40.0+simTick_*0.02);
        if(simTick_%200==0) simFuel_-=1.0; if(simFuel_<0) simFuel_=100.0;
        simLeftTurn_=(c>80&&c<120); simRightTurn_=(c>350&&c<390); simHighBeam_=(c>450&&c<490);
        simHandbrake_=(simSpeed_<5.0); simOdometer_+=(int)(simSpeed_*0.014);
        injectFrame(generateEngineFrame()); injectFrame(generateTransmissionFrame()); injectFrame(generateBodyFrame());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

double CanReceiver::extractSignal(const std::vector<uint8_t>& d, const DbcSignalDef& s) const {
    if (d.empty()||s.bitLength==0||s.bitLength>64) return std::numeric_limits<double>::quiet_NaN();
    uint64_t raw=0;
    for (uint8_t i=0;i<s.bitLength;++i) {
        uint16_t bp=s.startBit+i; uint8_t bi=bp/8,br=bp%8;
        if(bi>=d.size()) break; if((d[bi]>>br)&1) raw|=(1ULL<<i);
    }
    if(s.isSigned) { uint64_t sb=1ULL<<(s.bitLength-1); if(raw&sb){raw=~raw&((1ULL<<s.bitLength)-1);return -(double)(raw+1)*s.factor+s.offset;} }
    return (double)raw*s.factor+s.offset;
}

void CanReceiver::parseFrame(const CanFrame& f) {
    auto it=dbc_.find(f.id); if(it==dbc_.end()) return;
    for (const auto& sd : it->second.signals) {
        double rv=extractSignal(f.data,sd); if(std::isnan(rv)) continue;
        Signal sig; sig.name=sd.name; sig.messageId=f.id; sig.timestamp_ns=f.timestamp_ns;
        switch(sd.type){case 'd':sig.value=rv;break;case 'i':sig.value=(int)rv;break;case 'b':sig.value=(rv>0.5);break;}
        std::lock_guard<std::mutex> lk(cbMutex_); if(callback_) callback_(sig);
    }
}

CanFrame CanReceiver::generateEngineFrame() {
    CanFrame f; f.id=0x100; f.dlc=8; f.data.resize(8,0);
    uint16_t r=(uint16_t)(simRpm_*4.0); f.data[0]=r&0xFF; f.data[1]=(r>>8)&0xFF;
    f.data[2]=(uint8_t)(simCoolant_+40); f.data[3]=(uint8_t)simFuel_; return f;
}
CanFrame CanReceiver::generateTransmissionFrame() {
    CanFrame f; f.id=0x200; f.dlc=8; f.data.resize(8,0); f.data[0]=(uint8_t)simGear_;
    uint16_t s=(uint16_t)(simSpeed_*100.0); f.data[1]=s&0xFF; f.data[2]=(s>>8)&0xFF; return f;
}
CanFrame CanReceiver::generateBodyFrame() {
    CanFrame f; f.id=0x400; f.dlc=8; f.data.resize(8,0); uint8_t st=0;
    if(simLeftTurn_)st|=1; if(simRightTurn_)st|=2; if(simHighBeam_)st|=4; if(simHandbrake_)st|=8; f.data[0]=st; return f;
}
