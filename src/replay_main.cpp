#include "routing/SignalRouter.h"
#include "service/SomeipPublisher.h"
#include "recorder/SignalRecorder.h"
#include "common/Logger.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if(argc<2){std::cout<<"Usage: signal_replay <file.csv> [speed]\n";return 1;}
    std::string fp=argv[1]; double sp=1.0; if(argc>=3)sp=std::stod(argv[2]);
    Logger::setLevel(LogLevel::INFO);
    SignalRouter router; SomeipPublisher pub(router);
    SignalReplayer rep(router,pub);
    if(!rep.replay(fp,sp)){LOG_ERROR("Replay failed");return -1;}
    router.latency().report("Replay→SOME/IP");
    return 0;
}
