#include "gtest/gtest.h"
#include "debug.hpp"
#include "reflection.hpp"
#include "reflectiontest.pb.h"
#include "graphdsl.hpp"

using namespace std;

TEST(ReflectionTest, EqualCompareTest)
{
    testmsg msg;
    msg.set_str("foo");
    msg.set_db(2.33);
    msg.set_boolean(false);
    msg.set_int_32(666);

    google::protobuf::Message *msgp = &msg;
    EXPECT_TRUE( reflectedCompare(msgp,
                    "str",
                    netalgo::Relationship::equal,
                    "foo"));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "str",
                    netalgo::Relationship::equal,
                    "bar"));

    EXPECT_TRUE(reflectedCompare(msgp,
                    "db",
                    netalgo::Relationship::equal,
                    2.33));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "db",
                    netalgo::Relationship::equal,
                    6.66));
    
    EXPECT_TRUE(reflectedCompare(msgp,
                    "boolean",
                    netalgo::Relationship::equal,
                    false));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "boolean",
                    netalgo::Relationship::equal,
                    true));

    EXPECT_TRUE(reflectedCompare(msgp,
                    "int_32",
                    netalgo::Relationship::equal,
                    666));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "int_32",
                    netalgo::Relationship::equal,
                    233));
}

TEST(ReflectionTest, SmallerCompareTest)
{
    testmsg msg;
    msg.set_str("ABC");
    msg.set_int_32(42);
    msg.set_db(-6.2);
    msg.set_boolean(true);
    google::protobuf::Message *msgp = &msg;
    EXPECT_TRUE(reflectedCompare(msgp,
                    "str",
                    netalgo::Relationship::smaller,
                    "BBC"
                    ));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "str",
                    netalgo::Relationship::smaller,
                    "AAA"
                    ));
    EXPECT_TRUE(reflectedCompare(msgp,
                    "int_32",
                    netalgo::Relationship::smaller,
                    43
                    ));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "int_32",
                    netalgo::Relationship::smaller,
                    41
                    ));
    EXPECT_TRUE(reflectedCompare(msgp,
                    "db",
                    netalgo::Relationship::smaller,
                    -6.1
                    ));
    EXPECT_TRUE(reflectedCompare(msgp,
                    "db",
                    netalgo::Relationship::smaller,
                    6.1
                    ));
    EXPECT_FALSE(reflectedCompare(msgp,
                    "boolean",
                    netalgo::Relationship::smaller,
                    true
                    ));

}
