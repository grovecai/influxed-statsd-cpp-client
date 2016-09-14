#include "./non_blocking_sender.h"

#include <netdb.h>
#include <time.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "base/common/gflags.h"
#include "base/common/basic_types.h"
#include "base/common/logging.h"
#include "base/common/closure.h"
#include "./influxed_statsd_client.h"

namespace base {
namespace statsd {
DEFINE_int32(statsd_port, 8125, "statsd port");
DEFINE_string(statsd_host, "127.0.0.1", "statsd host");

// For testing socket not healthy manually
// DEFINE_string(statsd_host, "can_not_be_resolved", "statsd host");

struct SocketData {
  int sock;
  struct sockaddr_in server;

  std::string host;
  unsigned short port;

  char errmsg[1024];
};

NonBlockingSender* NonBlockingSender::Instance() {
  static NonBlockingSender* INSTANCE = new NonBlockingSender();
  return INSTANCE;
}

NonBlockingSender::NonBlockingSender() {
  d = new SocketData;

  bool success = initSocket(FLAGS_statsd_host, FLAGS_statsd_port);
  if (!success) {
    socketHealthy_ = false;
    LOG(ERROR) << "Fail to init socket, please check network connection of this host. Error message: "
    << d->errmsg;
  } else {
    socketHealthy_ = true;
  }

  worker_.Start(::NewCallback(this, &NonBlockingSender::working));
}

NonBlockingSender::~NonBlockingSender() {
  // close socket
  if (d->sock >= 0) {
    close(d->sock);
    d->sock = -1;
    delete d;
    d = NULL;
  }
}

void NonBlockingSender::working() {
  while (true) {
    bool success = blockingSend(this->metricQueue_.Take());
    if (!success) {
      LOG(ERROR) << "Fail to send metric. Error message: " << d->errmsg;
    }
  }
}

void NonBlockingSender::Send(const std::string& message) {
  if ( socketHealthy_ ) {
    metricQueue_.Put(message);
  } else {
    LOG(ERROR) << "Socket is not healthy, can not send message!";
  }
}

bool NonBlockingSender::initSocket(const std::string& host, int port) {
  d->host = host;
  d->port = port;
  d->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (d->sock == -1) {
    snprintf(d->errmsg, sizeof(d->errmsg), "could not create socket, err=%m");
    return false;
  }

  memset(&d->server, 0, sizeof(d->server));
  d->server.sin_family = AF_INET;
  d->server.sin_port = htons(d->port);

  int ret = inet_aton(d->host.c_str(), &d->server.sin_addr);
  if (ret == 0) {
    // host must be a domain, get it from internet
    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    ret = getaddrinfo(d->host.c_str(), NULL, &hints, &result);
    if (ret) {
      snprintf(d->errmsg, sizeof(d->errmsg), "getaddrinfo fail, error=%d, msg=%s", ret, gai_strerror(ret));
      return false;
    }
    struct sockaddr_in* host_addr = (struct sockaddr_in*) result->ai_addr;
    memcpy(&d->server.sin_addr, &host_addr->sin_addr, sizeof(struct in_addr));
    freeaddrinfo(result);
  }

  return true;
}

bool NonBlockingSender::blockingSend(const std::string& message) {
  int ret = sendto(d->sock, message.data(), message.size(), 0, (struct sockaddr *) &d->server,
      sizeof(d->server));
  if (ret == -1) {
    snprintf(d->errmsg, sizeof(d->errmsg), "sendto server fail, host=%s:%d, err=%m", d->host.c_str(),
        d->port);
    return false;
  }

  // For manual test sending messages singletonly
  // std::cout<<"========"<<message<<"\n";

  return true;
}
}
}
