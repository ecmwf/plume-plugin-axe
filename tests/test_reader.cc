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
#include "atlas/field/Field.h"
#include "atlas/array.h"
#include "eckit/config/LocalConfiguration.h"
#include "eckit/config/YAMLConfiguration.h"

#include "atlas/array/ArrayView.h"
#include "atlas/grid/Partitioner.h"
#include "atlas/grid/StructuredGrid.h"
#include "atlas/field/Field.h"
#include "atlas/functionspace/StructuredColumns.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/meshgenerator.h"


#include "data_reader.h"
#include "extracted_data.h"
#include "plugin_config.h"


using namespace eckit::testing;
using namespace area_extractor;

namespace eckit {
namespace test {


static eckit::YAMLConfiguration& testConfig() {

    std::string conf_str(R"YAML({
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

    static eckit::YAMLConfiguration conf(conf_str);
    return conf;
}


CASE("test reader") {

    // atlas fields
    std::vector<std::string> params = {"field1", "field2"};
    std::vector<atlas::Field> fields;

    // create a grid
    atlas::Grid grid("O8");
    atlas::util::Config grid_config;
    grid_config.set("halo", 0);
    grid_config.set("levels", 10);
    grid_config.set("periodic_points", true);
    atlas::functionspace::StructuredColumns fs(grid, atlas::grid::Partitioner("equal_regions"), grid_config);

    // create fields
    for (const auto& param: params) {
        atlas::Field field = fs.createField<double>(atlas::option::name(param));
        fields.push_back(field);
    }

    // data reader
    DataReader reader(fields);

    // test extract data
    PluginConfig plugin_config( testConfig(), params );
    ExtractedData* data = reader.extractData(plugin_config);

    // test update data
    reader.updateData(*data);

    // test data
    EXPECT(data->size() > 0);
    EXPECT(!data->isEmpty());

    // test getting config back
    EXPECT_EQUAL(data->config().users().size(), 2);
    EXPECT_EQUAL(data->config().parameters().size(), 2);
    EXPECT_EQUAL(data->config().outputStrategy(), "CovJSON");


}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}