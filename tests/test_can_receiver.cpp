#include <gtest/gtest.h>
#include "can/CanReceiver.h"
#include <thread>
#include <chrono>
#include <cmath>
class CanReceiverTest : public ::testing::Test {
protected:
    CanReceiver rx;
    void SetUp() override { rx.start("test", false); std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    void TearDown() override { rx.stop(); }
};
TEST_F(CanReceiverTest, DbcLoading){int c=0;rx.setSignalCallback([&c](const Signal&){c++;});rx.injectFrame(CanFrame(0x100,{0xE8,0x2E,0x82,0x32,0,0,0,0}));std::this_thread::sleep_for(std::chrono::milliseconds(200));EXPECT_GE(c,3);}
TEST_F(CanReceiverTest, SignalValue){double v=0;rx.setSignalCallback([&](const Signal& s){if(s.name=="VehicleSpeed")v=s.asDouble();});uint16_t sr=12000;rx.injectFrame(CanFrame(0x200,{0x03,(uint8_t)(sr&0xFF),(uint8_t)((sr>>8)&0xFF),0,0,0,0,0}));std::this_thread::sleep_for(std::chrono::milliseconds(200));EXPECT_NEAR(v,120.0,0.1);}
TEST_F(CanReceiverTest, BoolSignal){bool lt=false;rx.setSignalCallback([&](const Signal& s){if(s.name=="LeftTurnSignal")lt=s.asBool();});rx.injectFrame(CanFrame(0x400,{0x01,0,0,0,0,0,0,0}));std::this_thread::sleep_for(std::chrono::milliseconds(200));EXPECT_TRUE(lt);}
