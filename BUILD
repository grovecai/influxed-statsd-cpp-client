cc_library(name = "influxed_statsd_client",
           srcs = ["*.cc",],
           excludes = ["*_test.cc",],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/thread/BUILD:thread",]
          )

cc_test(name = "influxed_statsd_client_test",
        srcs = [ "*_test.cc",
               ],
        deps = ["//base/testing/BUILD:test_main", ":influxed_statsd_client"]
       )