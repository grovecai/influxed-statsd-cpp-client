#pragma once

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include "base/thread/thread.h"
#include "base/thread/blocking_queue.h"
#include "./abstract_sender.h"

namespace base {
namespace statsd {

struct SocketData;

class NonBlockingSender: public AbstractSender {
 public:
  static NonBlockingSender* Instance();
  void Send(const std::string& message);
 private:
  NonBlockingSender();
  ~NonBlockingSender();

 private:
  bool initSocket(const std::string& host, int port);
  bool blockingSend(const std::string& message);
  void working();

 private:
  thread::Thread worker_;
  struct SocketData* d;
  thread::BlockingQueue<std::string> metricQueue_;
  bool socketHealthy_;

  DISALLOW_COPY_AND_ASSIGN(NonBlockingSender);
};
}
}
