/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */
#include "eckit/testing/Test.h"
#include "eckit/config/YAMLConfiguration.h"

#include "plugin_config.h"


using namespace eckit::testing;
using namespace area_extractor;

namespace eckit {
namespace test {

CASE("test valid config") {
 
    std::string conf_valid_str(R"YAML({
        "writer": "CovJSON",
        "requests": [
            {
                "user": "user11",
                "tag": "extraction_area_1",
                "extractions": [
                    {"north": 50, "south": 4, "east": 75, "west": 14},
                    {"north": 60, "south": 5, "east": 76, "west": 15}
                ]
            },
            {
                "user": "user22",
                "tag": "extraction_area_2",
                "extractions": [
                    {"north": 65, "south": 14, "east": 88, "west": 7}
                ]
            }
        ]
    })YAML");

    eckit::YAMLConfiguration conf_valid(conf_valid_str);

    // test valid config
    PluginConfig config_valid(conf_valid, {"param1", "param2", "param3"});
    EXPECT(config_valid.users().size() == 2);
    EXPECT(config_valid.parameters().size() == 3);
    EXPECT(config_valid.requests().size() == 2);
    EXPECT(config_valid.outputStrategy() == "CovJSON");

    // test user names
    EXPECT(config_valid.userName(0) == "user11");
    EXPECT(config_valid.userName(1) == "user22");
    EXPECT(config_valid.user_uid("user11") == 0);
    EXPECT(config_valid.user_uid("user22") == 1);

}

CASE("test invalid config") {

    // config with invalid request type
    std::string conf_invalid_str1(R"YAML({
        "writer": "CovJSON",
        "requests": "invalid_request_type"
    })YAML");
    eckit::YAMLConfiguration conf_invalid1(conf_invalid_str1);

    // test that invalid request type throws
    EXPECT_THROWS(PluginConfig(conf_invalid1, {"param1", "param2", "param3"}));


    // config with invalid write type
    std::string conf_invalid_str2(R"YAML({
        "writer": [],
        "requests": []
    })YAML");
    eckit::YAMLConfiguration conf_invalid2(conf_invalid_str2);

    // test that invalid write type throws
    EXPECT_THROWS(PluginConfig(conf_invalid2, {"param1", "param2", "param3"}));
}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}