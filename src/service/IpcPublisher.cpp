#include "IpcPublisher.h"
#include "common/Logger.h"
#include <cstring>
#include <sstream>
namespace { std::string esc(const std::string& s){std::string o;for(char c:s)switch(c){case'"':o+="\\\"";break;case'\\':o+="\\\\";break;default:o+=c;}return o;} }

bool IpcPublisher::start(const std::string& path) {
    listenFd_=socket(AF_UNIX,SOCK_STREAM,0); if(listenFd_<0){LOG_ERROR("IPC socket() failed");return false;}
    struct sockaddr_un a; memset(&a,0,sizeof(a)); a.sun_family=AF_UNIX; strncpy(a.sun_path,path.c_str(),sizeof(a.sun_path)-1);
    unlink(path.c_str());
    if(bind(listenFd_,(struct sockaddr*)&a,sizeof(a))<0){LOG_ERROR("IPC bind() failed");close(listenFd_);return false;}
    if(listen(listenFd_,5)<0){LOG_ERROR("IPC listen() failed");close(listenFd_);return false;}
    run_.store(true); accThread_=std::thread(&IpcPublisher::acceptThreadFunc,this);
    subId_=router_.subscribeAll([this](const Signal&s){onSignal(s);});
    LOG_INFO("IPC publisher listening on " << path); return true;
}
void IpcPublisher::stop() {
    if(!run_.load())return; run_.store(false); router_.unsubscribe(subId_);
    if(listenFd_>=0){close(listenFd_);listenFd_=-1;} if(accThread_.joinable())accThread_.join();
    std::lock_guard<std::mutex> lk(mtx_); for(int fd:clients_)close(fd); clients_.clear();
}
void IpcPublisher::acceptThreadFunc() {
    while(run_.load()) {
        struct timeval tv{0,100000}; fd_set f; FD_ZERO(&f); FD_SET(listenFd_,&f);
        if(select(listenFd_+1,&f,nullptr,nullptr,&tv)<=0)continue;
        int c=accept(listenFd_,nullptr,nullptr); if(c<0)continue;
        std::lock_guard<std::mutex> lk(mtx_); clients_.push_back(c);
        LOG_INFO("IPC client connected (fd=" << c << ")");
    }
}
std::string IpcPublisher::serializeSignal(const Signal& sig) const {
    std::ostringstream ss; ss << "{\"name\":\"" << esc(sig.name) << "\",\"value\":";
    if(std::holds_alternative<double>(sig.value))ss<<sig.asDouble();
    else if(std::holds_alternative<int>(sig.value))ss<<sig.asInt();
    else ss<<(sig.asBool()?"true":"false");
    ss << ",\"ts\":" << sig.timestamp_ns << "}"; return ss.str();
}
void IpcPublisher::onSignal(const Signal& sig) {
    std::string j=serializeSignal(sig)+"\n"; std::lock_guard<std::mutex> lk(mtx_);
    std::vector<int> alive; for(int fd:clients_){if(write(fd,j.data(),j.size())>0)alive.push_back(fd);else close(fd);} clients_=std::move(alive);
}
