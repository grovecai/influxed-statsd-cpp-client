#pragma once

#include <string>
#include <utility>
#include <vector>
#include "./non_blocking_sender.h"

namespace base {
typedef std::pair<std::string, std::string> TAG;
typedef std::vector<TAG> TAGS;
typedef statsd::AbstractSender Sender;

/**
 * Non blocking statsd client with influxdb extension
 * Best practice: please use a shared instance in you application
 */
class InfluxedStatsdClient {
 public:
  InfluxedStatsdClient();
  explicit InfluxedStatsdClient(std::string ns);
  ~InfluxedStatsdClient();

  /**
   * Initialize  client with a specific sender. Serve the purpose to do unit test.
   * Not recommend to be used in practice unless you know what you are doing.
   */
  explicit InfluxedStatsdClient(Sender* sender);


// statsd high level apis
 public:
  /**
   * Adjusts the specified counter by a given delta.
   *
   * This method is non-blocking and is guaranteed not to throw an exception\
   *
   * @param key
   *     the name of the counter to adjust
   * @param value
   *     the amount to adjust the counter by
   * @param sampleRate
   *     the sampling rate being employed. For example, a rate of 0.1 would tell StatsD that this counter is being sent
   *     sampled every 1/10th of the time.
   */
  void Count(const std::string& key, int64 value, float sampleRate = 1.0) const;

  /**
   * Convenience method equivalent to {Count(key, 1, sampleRate)} with a value of 1.
   */
  void Inc(const std::string& key, float sampleRate = 1.0) const;

  /**
   * Convenience method equivalent to {Count(key, -1, sampleRate)} with a value of -1.
   */
  void Dec(const std::string& key, float sampleRate = 1.0) const;

  /**
   * Records the latest fixed value for the specified named gauge.
   *
   * This method is non-blocking and is guaranteed not to throw an exception.
   *
   * @param key
   *     the name of the gauge
   * @param value
   *     the new reading of the gauge
   * @param sampleRate
   *     the sampling rate being employed. For example, a rate of 0.1 would tell StatsD that this counter is being sent
   *     sampled every 1/10th of the time.
   */
  void Gauge(const std::string& key, double value, float sampleRate = 1.0) const;

  /**
   * Records an execution time in milliseconds for the specified named operation.
   *
   * This method is non-blocking and is guaranteed not to throw an exception.
   *
   * @param key
   *     the name of the timed operation
   * @param ms
   *     the time in milliseconds
   * @param sampleRate
   *     the sampling rate being employed. For example, a rate of 0.1 would tell StatsD that this counter is being sent
   *     sampled every 1/10th of the time.
   */
  void Time(const std::string& key, int64 ms, float sampleRate = 1.0) const;

  /**
   * Records an execution time in milliseconds for the specified named operation and systemTimeMillisAtStart.
   *
   * This method is non-blocking and is guaranteed not to throw an exception.
   *
   * @param key
   *     the name of the timed operation
   * @param ms
   *     the time in milliseconds
   * @param sampleRate
   *     the sampling rate being employed. For example, a rate of 0.1 would tell StatsD that this counter is being sent
   *     sampled every 1/10th of the time.
   */
  void TimeMillisToNow(const std::string& key, int64 systemTimeMillisAtStart, float sampleRate = 1.0) const;

  /**
   * Records an execution time in milliseconds for the specified named operation and systemTimeMicrosAtStart.
   *
   * This method is non-blocking and is guaranteed not to throw an exception.
   *
   * @param key
   *     the name of the timed operation
   * @param ms
   *     the time in milliseconds
   * @param sampleRate
   *     the sampling rate being employed. For example, a rate of 0.1 would tell StatsD that this counter is being sent
   *     sampled every 1/10th of the time.
   */
  void TimeMicrosToNow(const std::string& key, int64 systemTimeMicrosAtStart, float sampleRate = 1.0) const;

// statsd low level apis
 public:
  /* (Low Level Api) manually send a message, all high level apis will eventually invoke this  api
   * type = "c", "g" or "ms"
   */
  void Send(const std::string& key, const std::string value, const std::string& type, float sampleRate = 1.0) const;
  void Send(const std::string& key, const int64 value, const std::string& type, float sampleRate = 1.0) const;

// helpers
 public:
  /**
   * Clone a new instance
   */
  InfluxedStatsdClient Clone() const;

  /**
   * Make a new InfluxedStatsdClient with a new @param ns based on current one.
   */
  InfluxedStatsdClient Ns(std::string ns) const;

  /**
   * Make a new InfluxedStatsdClient with a new @param subNs based on current one.
   * Will generate a new client with namespace as ns + "." + subNs
   */
  InfluxedStatsdClient ImmutableApendSubNs(std::string subNs) const;

  /**
   * append subNs to current ns.
   * Warning: please aware the side effect
   */
  InfluxedStatsdClient& ApendSubNs(std::string subNs);

  /**
   * Make a new InfluxedStatsdClient with a new @param tags based on current one.
   */
  InfluxedStatsdClient Tags(TAGS tags) const;

  /**
   * Make a new InfluxedStatsdClient with a new @param tag inserted based on current one.
   * Will generate a new client with namespace as ns + "." + subNs
   */
  InfluxedStatsdClient ImmutableAddTag(TAG tag) const;

  /**
   * Add a new tag to current tags
   * Warning: please aware the side effect
   */
  InfluxedStatsdClient& AddTag(TAG tag);

 private:
  std::string makeInfluxedKey(const std::string& key) const;
  InfluxedStatsdClient(Sender* sender_, const std::string& ns, const TAGS& tags);

 private:
  Sender* sender_;
  /**
   * every sending keys will has ns_ as prefix
   */
  std::string ns_;
  /**
   * extend keys with tags_ to support influxdb protocol
   * PS: ',' and '=' is not allowed in tag name and its value
   */
  TAGS tags_;

  static std::string NS_DEL;
  static std::string COMMA;
  static std::string EMPTY;
  static std::string TAG_EQ;
};
}
// end namespace

