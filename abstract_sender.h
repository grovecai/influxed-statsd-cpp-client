#pragma once

#include<string>

namespace base {
namespace statsd {
class AbstractSender {
 public:
  virtual ~AbstractSender() {}
  virtual void Send(const std::string& message) = 0;
};
}
}
