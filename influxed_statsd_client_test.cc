#include "./influxed_statsd_client.h"

#include <iostream>
#include "base/testing/gmock.h"
#include "base/testing/gtest.h"
#include "./dummy_sender.h"

namespace base {
namespace statsd {

class InfluxedStatsdClientTest: public ::testing::Test {
 protected:
  virtual void SetUp() {
    sender = new DummySender();
    client = new InfluxedStatsdClient(sender);
  }
  DummySender* sender;
  InfluxedStatsdClient* client;
};

// BEGIN:low level apis
TEST_F(InfluxedStatsdClientTest, SendWithStringValue) {
  client->Send("key", "121.2", "c", 2.0);
  ASSERT_EQ(sender->message_, "key:121.2|c|@2");
}

TEST_F(InfluxedStatsdClientTest, SendWithInt64SampleRate1) {
  client->Send("key", 121, "c", 1.0);
  ASSERT_EQ(sender->message_, "key:121|c");
}

TEST_F(InfluxedStatsdClientTest, SendWithDefaultSampleRate) {
  client->Send("key", 121, "c");
  ASSERT_EQ(sender->message_, "key:121|c");
}

// END:low level apis

// BEGIN: helpers
TEST_F(InfluxedStatsdClientTest, Ns) {
  InfluxedStatsdClient newClient = client->Ns("ns");
  newClient.Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "ns.key:1|c");

  // verify no side effect to previous one
  client->Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "key:1|c");
}

TEST_F(InfluxedStatsdClientTest, ImmutableApendSubNs) {
  InfluxedStatsdClient nsClient = client->Ns("ns");

  InfluxedStatsdClient subnsClient = nsClient.ImmutableApendSubNs("subns");
  subnsClient.Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "ns.subns.key:1|c");

  // verify no side effect to previous one
  nsClient.Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "ns.key:1|c");
}

TEST_F(InfluxedStatsdClientTest, ApendSubNs) {
  InfluxedStatsdClient nsClient = client->Ns("ns");
  nsClient.ApendSubNs("subns").Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "ns.subns.key:1|c");

  // namespace permanently changed
  nsClient.Send("key", 2, "c");
  ASSERT_EQ(sender->message_, "ns.subns.key:2|c");
}

TEST_F(InfluxedStatsdClientTest, Tags) {
  TAGS tags = { {"tag1", "value1"} };
  InfluxedStatsdClient oneClient = client->Tags(tags);

  // single tag
  oneClient.Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "key,tag1=value1:1|c");

  // two tags
  tags = { {"tag1", "value1"}, {"tag2", "value2"}};
  InfluxedStatsdClient twoClient = client->Tags(tags);
  twoClient.Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "key,tag1=value1,tag2=value2:1|c");

  // no side effect to original one
  client->Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "key:1|c");
}

TEST_F(InfluxedStatsdClientTest, AddTag) {
  client->AddTag({"tag", "value"}).Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "key,tag=value:1|c");

  // tags permanently added
  client->Send("key", 2, "c");
  ASSERT_EQ(sender->message_, "key,tag=value:2|c");
}

TEST_F(InfluxedStatsdClientTest, ImmutableAddTag) {
  client->ImmutableAddTag({"tag", "value"}).Send("key", 1, "c");
  ASSERT_EQ(sender->message_, "key,tag=value:1|c");

  // won't affect original one
  client->Send("key", 2, "c");
  ASSERT_EQ(sender->message_, "key:2|c");
}

// END: helpers

// BEGIN: high level apis
TEST_F(InfluxedStatsdClientTest, Count) {
  client->Count("key", 10);
  ASSERT_EQ(sender->message_, "key:10|c");

  client->Count("key", 10, 0.1);
  ASSERT_EQ(sender->message_, "key:10|c|@0.1");
}

TEST_F(InfluxedStatsdClientTest, Inc) {
  client->Inc("key", 0.01);
  ASSERT_EQ(sender->message_, "key:1|c|@0.01");

  client->Inc("key");
  ASSERT_EQ(sender->message_, "key:1|c");
}

TEST_F(InfluxedStatsdClientTest, Dec) {
  client->Dec("key", 0.01);
  ASSERT_EQ(sender->message_, "key:-1|c|@0.01");

  client->Dec("key");
  ASSERT_EQ(sender->message_, "key:-1|c");
}

TEST_F(InfluxedStatsdClientTest, Gauge) {
  client->Gauge("key", 0.0100000, 0.01);
  ASSERT_EQ(sender->message_, "key:0.01|g|@0.01");

  client->Gauge("key", 1000);
  ASSERT_EQ(sender->message_, "key:1000|g");
}

TEST_F(InfluxedStatsdClientTest, Time) {
  client->Time("key", 279172897979, 0.01);
  ASSERT_EQ(sender->message_, "key:279172897979|ms|@0.01");

  client->Time("key", 279172897979);
  ASSERT_EQ(sender->message_, "key:279172897979|ms");
}

// FIXME: a dummy test
TEST_F(InfluxedStatsdClientTest, TimeMillisToNow) {
  ASSERT_EQ(sender->message_.size() == 0, true);
  client->TimeMillisToNow("key", 0, 0.01);
  ASSERT_EQ(sender->message_.size() > 0, true);
  // std::cout<<"sender->message_:"<<sender->message_<<"\n";
}

// FIXME: a dummy test
TEST_F(InfluxedStatsdClientTest, TimeMicrosToNow) {
  ASSERT_EQ(sender->message_.size() == 0, true);
  client->TimeMicrosToNow("key", 0, 0.01);
  ASSERT_EQ(sender->message_.size() > 0, true);
  // std::cout<<"sender->message_:"<<sender->message_<<"\n";
}

// END: high level apis

// BEGIN test combination
TEST_F(InfluxedStatsdClientTest, ALittleComplexTest) {
  TAGS tags = { {"tag1", "value1"} };
  client->Ns("ns")
      .ImmutableApendSubNs("subns")
      .Tags(tags)
      .ImmutableAddTag({"tag2", "value2"})
      .Gauge("key", 0.0100000, 0.01);
  ASSERT_EQ(sender->message_, "ns.subns.key,tag1=value1,tag2=value2:0.01|g|@0.01");

  // original one still as before
  client->Gauge("key", 0.0100000, 0.01);
  ASSERT_EQ(sender->message_, "key:0.01|g|@0.01");
}
// END test combination

// BEGIN sanity test
TEST(InfluxedStatsdClientSanityTest, dummy) {
  InfluxedStatsdClient nsclient("prefix");
  for (int i = 0; i < 10; i++) {
    nsclient.Gauge("nskey", 0.0100000, 0.01);
  }

  InfluxedStatsdClient client;
  for (int i = 0; i < 10; i++) {
    client.Gauge("key", 0.0100000, 0.01);
  }
  sleep(2);
}
// END sanity test
}
}  // namespace base
