#pragma once

#include<string>

namespace base {
namespace statsd {
class DummySender: public AbstractSender {
 public:
  DummySender() {
    message_="";
  }
  ~DummySender() {}
  void Send(const std::string& message) {
    message_ = message;
  }

 public:
  std::string message_;
};
}
}
