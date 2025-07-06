#include "WebControlServer.h"
#include "routing/SignalRouter.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <cstdlib>

bool WebControlServer::start(int port) {
    port_=port; listenFd_=socket(AF_INET,SOCK_STREAM,0); if(listenFd_<0)return false;
    int o=1; setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,&o,sizeof(o));
    struct sockaddr_in a; memset(&a,0,sizeof(a)); a.sin_family=AF_INET; a.sin_addr.s_addr=INADDR_ANY; a.sin_port=htons(port);
    if(bind(listenFd_,(struct sockaddr*)&a,sizeof(a))<0){close(listenFd_);return false;}
    if(listen(listenFd_,5)<0){close(listenFd_);return false;}
    running_.store(true); thread_=std::thread(&WebControlServer::serverThreadFunc,this);
    LOG_INFO("Web control panel: http://localhost:" << port); return true;
}
void WebControlServer::stop() { if(!running_.load())return; running_.store(false); if(listenFd_>=0){close(listenFd_);listenFd_=-1;} if(thread_.joinable())thread_.join(); }
void WebControlServer::serverThreadFunc() {
    while(running_.load()) {
        struct timeval tv{0,100000}; fd_set f; FD_ZERO(&f); FD_SET(listenFd_,&f);
        if(select(listenFd_+1,&f,nullptr,nullptr,&tv)<=0)continue;
        int c=accept(listenFd_,nullptr,nullptr); if(c<0)continue; handleRequest(c); close(c);
    }
}
void WebControlServer::handleRequest(int fd) {
    char buf[4096]; ssize_t n=read(fd,buf,sizeof(buf)-1); if(n<=0)return; buf[n]=0;
    std::string req(buf); std::string m=req.substr(0,req.find(' '));
    std::string p=req.substr(m.size()+1); p=p.substr(0,p.find(' '));
    if(m=="GET"&&p=="/"){std::string h=getHtmlPage();std::ostringstream r;r<<"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "<<h.size()<<"\r\nConnection: close\r\n\r\n"<<h;auto s=r.str();write(fd,s.data(),s.size());return;}
    if(m=="GET"&&p=="/status"){std::ostringstream j;j<<"{\"speed\":"<<state_.speed.load()<<",\"rpm\":"<<state_.rpm.load()<<",\"gear\":"<<state_.gear.load()<<",\"leftTurn\":"<<(state_.leftTurn.load()?"true":"false")<<",\"rightTurn\":"<<(state_.rightTurn.load()?"true":"false")<<",\"highBeam\":"<<(state_.highBeam.load()?"true":"false")<<",\"handbrake\":"<<(state_.handbrake.load()?"true":"false")<<",\"autoMode\":"<<(state_.autoMode.load()?"true":"false")<<"}";auto b=j.str();std::ostringstream r;r<<"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "<<b.size()<<"\r\nConnection: close\r\n\r\n"<<b;auto s=r.str();write(fd,s.data(),s.size());return;}
    if(m=="POST"&&p=="/control"){auto bs=req.find("\r\n\r\n");if(bs==std::string::npos)return;std::string body=req.substr(bs+4);std::istringstream ss(body);std::string pair;while(std::getline(ss,pair,'&')){auto eq=pair.find('=');if(eq==std::string::npos)continue;std::string k=pair.substr(0,eq),v=pair.substr(eq+1);if(k=="speed")state_.speed.store(std::stod(v));else if(k=="rpm")state_.rpm.store(std::stod(v));else if(k=="gear")state_.gear.store(std::stoi(v));else if(k=="leftTurn")state_.leftTurn.store(v=="true");else if(k=="rightTurn")state_.rightTurn.store(v=="true");else if(k=="highBeam")state_.highBeam.store(v=="true");else if(k=="handbrake")state_.handbrake.store(v=="true");else if(k=="autoMode")state_.autoMode.store(v=="true");}std::string r="HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";write(fd,r.data(),r.size());return;}
    std::string r="HTTP/1.1 404\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";write(fd,r.data(),r.size());
}
std::string WebControlServer::getHtmlPage() const {
    return R"HTML(<!DOCTYPE html><html><head><meta charset="utf-8"><title>Signal Gateway Control</title>
<style>body{font-family:monospace;background:#1a1a2e;color:#eee;margin:40px}h1{color:#00d4ff}.panel{background:#16213e;padding:20px;border-radius:10px;margin:10px 0}.control{margin:15px 0}label{display:inline-block;width:120px}input[type=range]{width:300px;vertical-align:middle}.val{display:inline-block;width:60px;color:#00d4ff}button{background:#0f3460;color:#fff;border:1px solid #00d4ff;padding:8px 20px;border-radius:5px;cursor:pointer;margin:5px}button:hover{background:#1a5276}button.active{background:#00d4ff;color:#000}.status{color:#aaa;font-size:12px;margin-top:10px}</style></head>
<body><h1>Signal Gateway Control Panel</h1><div class="panel"><h3>Mode</h3><button id="btnAuto" class="active" onclick="setMode(true)">Auto</button><button id="btnManual" onclick="setMode(false)">Manual</button></div>
<div class="panel"><h3>Vehicle</h3><div class="control"><label>Speed km/h</label><input type="range" id="speed" min="0" max="260" value="0" oninput="document.getElementById('speedVal').textContent=this.value;send('speed',this.value)"><span class="val" id="speedVal">0</span></div>
<div class="control"><label>RPM</label><input type="range" id="rpm" min="0" max="8000" value="800" step="100" oninput="document.getElementById('rpmVal').textContent=this.value;send('rpm',this.value)"><span class="val" id="rpmVal">800</span></div>
<div class="control"><label>Gear</label><button onclick="send('gear',-1)">R</button><button onclick="send('gear',0)">N</button><button onclick="send('gear',1)">1</button><button onclick="send('gear',2)">2</button><button onclick="send('gear',3)">3</button><button onclick="send('gear',4)">4</button><button onclick="send('gear',5)">5</button><button onclick="send('gear',6)">6</button></div>
<h3>Body</h3><button id="btnLeft" onclick="this.classList.toggle('active');send('leftTurn',this.classList.contains('active'))">Left Turn</button><button id="btnRight" onclick="this.classList.toggle('active');send('rightTurn',this.classList.contains('active'))">Right Turn</button><button id="btnBeam" onclick="this.classList.toggle('active');send('highBeam',this.classList.contains('active'))">High Beam</button><button id="btnBrake" onclick="this.classList.toggle('active');send('handbrake',this.classList.contains('active'))">Handbrake</button></div>
<div class="status" id="status">Ready</div>
<script>function send(k,v){fetch('/control',{method:'POST',body:k+'='+v});document.getElementById('status').textContent='Sent: '+k+'='+v}function setMode(a){send('autoMode',a);document.getElementById('btnAuto').classList.toggle('active',a);document.getElementById('btnManual').classList.toggle('active',!a)}setInterval(function(){fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('status').textContent='speed='+d.speed+' rpm='+d.rpm+' gear='+d.gear+' auto='+d.autoMode}).catch(e=>{})},1000)</script>
</body></html>)HTML";
}
