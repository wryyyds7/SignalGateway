#include "SomeipPublisher.h"
#include <cstring>
SomeipPublisher::SomeipPublisher(SignalRouter& r) : router_(r) {
    uint16_t s=0x1001;
    svcMap_["VehicleSpeed"]={s++,1,1,"VehicleService"}; svcMap_["EngineRPM"]={s++,1,2,"VehicleService"};
    svcMap_["Gear"]={s++,1,3,"VehicleService"}; svcMap_["CoolantTemp"]={s++,1,4,"VehicleService"};
    svcMap_["FuelLevel"]={s++,1,5,"VehicleService"};
    svcMap_["LeftTurnSignal"]={s++,2,1,"BodyService"}; svcMap_["RightTurnSignal"]={s++,2,2,"BodyService"};
    svcMap_["HighBeam"]={s++,2,3,"BodyService"}; svcMap_["Handbrake"]={s++,2,4,"BodyService"};
}
SomeipPublisher::~SomeipPublisher() { stop(); }
bool SomeipPublisher::start() { if(run_.load())return true; subId_=router_.subscribeAll([this](const Signal&s){onSignal(s);}); run_.store(true); LOG_INFO("SOME/IP publisher started"); return true; }
void SomeipPublisher::stop() { if(!run_.load())return; router_.unsubscribe(subId_); run_.store(false); }
void SomeipPublisher::onSignal(const Signal& sig) {
    auto it=svcMap_.find(sig.name); if(it==svcMap_.end())return;
    SomeipMessage m; m.serviceId=it->second.serviceId; m.methodId=it->second.methodId;
    m.timestamp_ns=sig.timestamp_ns; m.payload=serializeSignal(sig); m.payloadSize=m.payload.size();
    pub_++; { std::lock_guard<std::mutex> lk(mtx_); msgs_.push_back(std::move(m)); if(msgs_.size()>MAX_RECENT) msgs_.pop_front(); }
}
std::vector<uint8_t> SomeipPublisher::serializeSignal(const Signal& sig) const {
    uint16_t nl=sig.name.size(); std::vector<uint8_t> p(2+nl+1+8);
    std::memcpy(p.data(),&nl,2); std::memcpy(p.data()+2,sig.name.data(),nl);
    if(std::holds_alternative<double>(sig.value)){p[2+nl]=1;double v=sig.asDouble();std::memcpy(&p[3+nl],&v,8);}
    else if(std::holds_alternative<int>(sig.value)){p[2+nl]=2;int64_t v=sig.asInt();std::memcpy(&p[3+nl],&v,8);}
    else{p[2+nl]=3;int64_t v=sig.asBool()?1:0;std::memcpy(&p[3+nl],&v,8);} return p;
}
