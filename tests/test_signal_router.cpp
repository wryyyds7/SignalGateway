#include <gtest/gtest.h>
#include "routing/SignalRouter.h"
#include "common/Signal.h"
TEST(SignalRouterTest, SubscribeAndPublish){SignalRouter r;int c=0;auto id=r.subscribe("VehicleSpeed",[&](const Signal& s){c++;});Signal s;s.name="VehicleSpeed";s.value=120.0;r.publish(s);EXPECT_EQ(c,1);r.unsubscribe(id);}
TEST(SignalRouterTest, Wildcard){SignalRouter r;int c=0;r.subscribeAll([&](const Signal&){c++;});Signal a;a.name="A";a.value=1.0;Signal b;b.name="B";b.value=2.0;r.publish(a);r.publish(b);EXPECT_EQ(c,2);}
TEST(SignalRouterTest, Unsubscribe){SignalRouter r;int c=0;auto id=r.subscribe("T",[&](const Signal&){c++;});Signal s;s.name="T";s.value=0;r.publish(s);EXPECT_EQ(c,1);r.unsubscribe(id);r.publish(s);EXPECT_EQ(c,1);}
TEST(SignalRouterTest, Selective){SignalRouter r;int sc=0,rc=0;r.subscribe("VehicleSpeed",[&](const Signal&){sc++;});r.subscribe("EngineRPM",[&](const Signal&){rc++;});Signal a;a.name="VehicleSpeed";a.value=1.0;Signal b;b.name="EngineRPM";b.value=2.0;r.publish(a);r.publish(b);EXPECT_EQ(sc,1);EXPECT_EQ(rc,1);}
