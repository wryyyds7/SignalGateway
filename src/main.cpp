#include "can/CanReceiver.h"
#include "routing/SignalRouter.h"
#include "service/SomeipPublisher.h"
#include "service/IpcPublisher.h"
#include "service/WebControlServer.h"
#include "recorder/SignalRecorder.h"
#include "common/Logger.h"
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_running{true};
void signalHandler(int) { g_running.store(false); }

int main(int argc, char* argv[]) {
    std::string ipcSocket, recordFile;
    int duration=30, webPort=0;
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--help"||a=="-h"){std::cout<<"Usage: signal_gateway [--ipc <path>] [--web <port>] [--record <file>] [--duration <sec>] [--verbose]\n";return 0;}
        if(a=="--ipc"&&i+1<argc)ipcSocket=argv[++i];
        if(a=="--web"&&i+1<argc)webPort=std::atoi(argv[++i]);
        if(a=="--record"&&i+1<argc)recordFile=argv[++i];
        if(a=="--duration"&&i+1<argc)duration=std::atoi(argv[++i]);
        if(a=="--verbose")Logger::setLevel(LogLevel::DEBUG);
    }
    std::signal(SIGINT,signalHandler); std::signal(SIGTERM,signalHandler);
    LOG_INFO("=== Signal Gateway Starting ===");

    SignalRouter router;
    CanReceiver canReceiver;
    SomeipPublisher publisher(router);

    std::unique_ptr<IpcPublisher> ipc;
    if(!ipcSocket.empty()){ipc=std::make_unique<IpcPublisher>(router);if(!ipc->start(ipcSocket)){LOG_ERROR("IPC failed");return -1;}}

    std::unique_ptr<WebControlServer> web; WebControlServer::ManualState manualState;
    if(webPort>0){web=std::make_unique<WebControlServer>(router,manualState);if(!web->start(webPort)){LOG_ERROR("Web failed");return -1;}canReceiver.setManualState(&manualState);}

    std::unique_ptr<SignalRecorder> recorder;
    if(!recordFile.empty()){recorder=std::make_unique<SignalRecorder>(router);if(!recorder->start(recordFile)){LOG_ERROR("Recorder failed");return -1;}}

    canReceiver.setSignalCallback([&router](const Signal& sig){router.publish(sig);});
    if(!canReceiver.start("virtual")){LOG_ERROR("CAN failed");return -1;}
    publisher.start();
    LOG_INFO("Gateway running for " << duration << "s. Ctrl+C to stop.");

    auto st=std::chrono::steady_clock::now();
    while(g_running.load()){
        auto el=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-st).count();
        if(el>=duration)break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::string s="Stats: CAN="+std::to_string(router.publishedCount())+" SOME/IP="+std::to_string(publisher.publishedCount());
        if(ipc)s+=" IPC="+std::to_string(ipc->clientCount());
        LOG_INFO(s);
    }
    LOG_INFO("Shutting down...");
    publisher.stop(); if(ipc)ipc->stop(); if(web)web->stop(); if(recorder)recorder->stop(); canReceiver.stop();
    router.latency().report("End-to-end");
    LOG_INFO("=== Signal Gateway Stopped ===");
    return 0;
}
