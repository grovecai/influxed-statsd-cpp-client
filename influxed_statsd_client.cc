#include "./influxed_statsd_client.h"

#include <math.h>
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include<sstream>
#include <iostream>
#include "base/time/timestamp.h"
#include "base/common/basic_types.h"
#include "base/strings/string_printf.h"
#include "./non_blocking_sender.h"

namespace base {

std::string InfluxedStatsdClient::EMPTY = "";
std::string InfluxedStatsdClient::NS_DEL = ".";
std::string InfluxedStatsdClient::COMMA = ",";
std::string InfluxedStatsdClient::TAG_EQ = "=";

static inline std::string concat(const std::vector<std::string>& tags, const std::string DEL) {
  std::stringstream ss;
  bool first = true;
  for (unsigned int i = 0; i < tags.size(); i++) {
    if (!first) {
      ss << DEL;
    }
    ss << tags[i];
    first = false;
  }
  return ss.str();
}

inline bool fequal(float a, float b) {
  const float epsilon = 0.0001;
  return (fabs(a - b) < epsilon);
}


InfluxedStatsdClient::InfluxedStatsdClient() {
  sender_ = statsd::NonBlockingSender::Instance();
  ns_ = EMPTY;
  tags_ = {};
}
InfluxedStatsdClient::InfluxedStatsdClient(std::string ns) {
  sender_ = statsd::NonBlockingSender::Instance();
  ns_ = ns;
  tags_ = {};
}
InfluxedStatsdClient::~InfluxedStatsdClient() {
}

InfluxedStatsdClient::InfluxedStatsdClient(Sender* sender, const std::string& ns, const TAGS& tags) {
  CHECK(sender != NULL)<< "sender is NULL";
  sender_ = sender;
  ns_ = ns;
  tags_ = tags;
}

InfluxedStatsdClient::InfluxedStatsdClient(Sender* sender) {
  sender_ = sender;
  ns_ = EMPTY;
  tags_ = {};
}

InfluxedStatsdClient InfluxedStatsdClient::Clone() const{
  return InfluxedStatsdClient(sender_, ns_, tags_);
}

InfluxedStatsdClient InfluxedStatsdClient::Ns(std::string ns) const{
  return InfluxedStatsdClient(sender_, ns, tags_);
}
InfluxedStatsdClient InfluxedStatsdClient::ImmutableApendSubNs(std::string subNs) const{
  CHECK(ns_!= EMPTY) << "Please make sure namespace is not empty";
  std::string newNs = concat({ns_, subNs}, NS_DEL);
  return InfluxedStatsdClient(sender_, newNs, tags_);
}
InfluxedStatsdClient& InfluxedStatsdClient::ApendSubNs(std::string subNs) {
  CHECK(ns_!= EMPTY) << "Please make sure namespace is not empty";
  ns_ = concat({ns_, subNs}, NS_DEL);
  return *(this);
}
InfluxedStatsdClient InfluxedStatsdClient::Tags(TAGS tags) const{
  return InfluxedStatsdClient(sender_, ns_, tags);
}

InfluxedStatsdClient InfluxedStatsdClient::ImmutableAddTag(TAG tag) const{
  TAGS newTags = tags_;
  newTags.push_back(tag);
  return InfluxedStatsdClient(sender_, ns_, newTags);
}

InfluxedStatsdClient& InfluxedStatsdClient::AddTag(TAG tag) {
  tags_.push_back(tag);
  return *(this);
}

void InfluxedStatsdClient::Dec(const std::string& key,  float sampleRate) const{
  Count(key, -1, sampleRate);
}


void InfluxedStatsdClient::Inc(const std::string& key,  float sampleRate) const{
  Count(key, 1, sampleRate);
}

void InfluxedStatsdClient::Count(const std::string& key, int64 value, float sampleRate) const{
  Send(key, value, "c", sampleRate);
}

void InfluxedStatsdClient::Gauge(const std::string& key, double value, float sampleRate) const{
  Send(key, base::StringPrintf("%.5g", value), "g", sampleRate);
}

void InfluxedStatsdClient::Time(const std::string& key, int64 ms, float sampleRate) const{
  Send(key, ms, "ms", sampleRate);
}


void InfluxedStatsdClient::TimeMillisToNow(const std::string& key, int64 systemTimeMillisAtStart,
     float sampleRate) const{
  int64 now = base::GetTimestamp() / 1000;
  size_t ms = now - systemTimeMillisAtStart;
  ms = ms > 0 ? ms : 0;
  Time(key, ms, sampleRate);
}


void InfluxedStatsdClient::TimeMicrosToNow(const std::string& key, int64 systemTimeMicrosAtStart,
     float sampleRate) const{
  int64 now = base::GetTimestamp();
  size_t ms = (now - systemTimeMicrosAtStart) / 1000;
  ms = ms > 0 ? ms : 0;
  Time(key, ms, sampleRate);
}


std::string InfluxedStatsdClient::makeInfluxedKey(const std::string& key) const{
  std::string influxedKey = key;

  if (ns_ != EMPTY) {
    influxedKey = ns_ + NS_DEL + influxedKey;
  }

  int size = tags_.size();
  if (size == 0) {
    return influxedKey;
  }

  std::vector<std::string> formatTags;
  for (int i = 0; i < size; i++) {
    std::string formated = tags_[i].first + TAG_EQ + tags_[i].second;
    formatTags.push_back(formated);
  }

  return influxedKey + COMMA + concat(formatTags, COMMA);
}

void InfluxedStatsdClient::Send(const std::string& key, std::string value, const std::string &type,
     float sampleRate) const{
  CHECK(sender_ != NULL) << "please do not send metrics before setting sender ";

  std::string influxedKey = makeInfluxedKey(key);

  std::string message = base::StringPrintf("%s:%s|%s", influxedKey.c_str(), value.c_str(), type.c_str());
  if (!fequal(sampleRate, 1.0)) {
    message = base::StringPrintf("%s|@%.5g", message.c_str(), sampleRate);
  }

  sender_->Send(message);
}

void InfluxedStatsdClient::Send(const std::string& key,
                                const int64 value,
                                const std::string& type,
                                float sampleRate) const{
  Send(key, base::StringPrintf("%jd", value), type, sampleRate);
}
}

