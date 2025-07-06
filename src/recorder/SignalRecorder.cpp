#include "SignalRecorder.h"
#include "common/Logger.h"
#include <chrono>
#include <thread>
#include <sstream>

bool SignalRecorder::start(const std::string& fp) {
    file_.open(fp, std::ios::out|std::ios::trunc);
    if(!file_.is_open()){LOG_ERROR("Cannot open recording file");return false;}
    file_ << "timestamp_ns,message_id,signal_name,type,value\n"; file_.flush();
    subId_=router_.subscribeAll([this](const Signal&s){onSignal(s);});
    rec_.store(true); LOG_INFO("Recording started → " << fp); return true;
}
void SignalRecorder::stop() {
    if(!rec_.load())return; rec_.store(false); router_.unsubscribe(subId_);
    std::lock_guard<std::mutex> lk(fm_); if(file_.is_open()){file_.flush();file_.close();}
    LOG_INFO("Recording stopped. Total: " << cnt_.load());
}
void SignalRecorder::onSignal(const Signal& sig) {
    if(!rec_.load())return;
    std::ostringstream ss;
    ss << sig.timestamp_ns << ",0x" << std::hex << sig.messageId << std::dec << "," << sig.name << ",";
    if(std::holds_alternative<double>(sig.value))ss<<"double,"<<sig.asDouble();
    else if(std::holds_alternative<int>(sig.value))ss<<"int,"<<sig.asInt();
    else ss<<"bool,"<<(sig.asBool()?"true":"false");
    ss<<"\n";
    { std::lock_guard<std::mutex> lk(fm_); if(file_.is_open())file_<<ss.str(); }
    cnt_.fetch_add(1);
}

bool SignalReplayer::replay(const std::string& fp, double speed) {
    std::ifstream f(fp); if(!f.is_open()){LOG_ERROR("Cannot open replay file");return false;}
    LOG_INFO("Replaying from " << fp << " (speed=" << speed << "x)");
    pub_.start();
    std::string line; std::getline(f,line); int64_t prev=0; int ln=1;
    while(std::getline(f,line)) {
        if(line.empty())continue; ++ln;
        std::istringstream ss(line); std::string ts,mn,nm,ty,val;
        std::getline(ss,ts,','); std::getline(ss,mn,','); std::getline(ss,nm,','); std::getline(ss,ty,','); std::getline(ss,val);
        int64_t t; try{t=std::stoll(ts);}catch(...){continue;}
        if(prev>0){int64_t d=t-prev;int64_t w=d/1000.0/speed;if(w>0&&w<1000000)std::this_thread::sleep_for(std::chrono::microseconds(w));}
        prev=t;
        Signal sig; sig.name=nm; sig.timestamp_ns=Signal::now();
        if(mn.substr(0,2)=="0x"){try{sig.messageId=std::stoul(mn.substr(2),nullptr,16);}catch(...){}}
        try{if(ty=="double")sig.value=std::stod(val);else if(ty=="int")sig.value=std::stoi(val);else sig.value=(val=="true");}catch(...){continue;}
        router_.publish(sig); cnt_.fetch_add(1);
    }
    LOG_INFO("Replay complete: " << cnt_.load() << " signals"); pub_.stop(); return true;
}
