#include <gtest/gtest.h>
#include "routing/SignalRouter.h"
#include "service/SomeipPublisher.h"
#include <thread>
#include <chrono>
TEST(SomeipPublisherTest, SignalToMessage){SignalRouter r;SomeipPublisher p(r);p.start();Signal s;s.name="VehicleSpeed";s.value=100.0;s.timestamp_ns=Signal::now();r.publish(s);std::this_thread::sleep_for(std::chrono::milliseconds(50));EXPECT_GE(p.publishedCount(),1);auto m=p.recentMessages();ASSERT_FALSE(m.empty());EXPECT_GT(m[0].payloadSize,0);p.stop();}
TEST(SomeipPublisherTest, UnknownIgnored){SignalRouter r;SomeipPublisher p(r);p.start();Signal s;s.name="Unknown";s.value=0;r.publish(s);std::this_thread::sleep_for(std::chrono::milliseconds(50));EXPECT_EQ(p.publishedCount(),0);p.stop();}
