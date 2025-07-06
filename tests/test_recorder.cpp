#include <gtest/gtest.h>
#include "routing/SignalRouter.h"
#include "recorder/SignalRecorder.h"
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
TEST(RecorderTest, RecordAndVerify){SignalRouter r;SignalRecorder rec(r);std::string f="/tmp/test_rec.csv";ASSERT_TRUE(rec.start(f));Signal s;s.name="TestSignal";s.value=42.0;s.timestamp_ns=1000000;r.publish(s);std::this_thread::sleep_for(std::chrono::milliseconds(50));rec.stop();std::ifstream file(f);ASSERT_TRUE(file.is_open());std::string h;std::getline(file,h);EXPECT_NE(h.find("timestamp"),std::string::npos);std::string l;std::getline(file,l);EXPECT_NE(l.find("TestSignal"),std::string::npos);}
