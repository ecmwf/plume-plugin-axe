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
#include <cmath>
#include <memory>

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

#include "plume/data/FieldAccess.h"


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
    std::vector<plume::data::FieldView> fields;

    // create a grid
    atlas::Grid grid("O8");
    atlas::util::Config grid_config;
    grid_config.set("halo", 0);
    grid_config.set("levels", 10);
    grid_config.set("periodic_points", true);
    atlas::functionspace::StructuredColumns fs(grid, atlas::grid::Partitioner("equal_regions"), grid_config);

    // create fields
    for (const auto& param: params) {
        atlas::Field field = fs.createField<FIELD_TYPE_REAL>(atlas::option::name(param));
        fields.push_back(plume::data::FieldView(field));
    }

    // data reader
    DataReader reader(fields);

    // test extract data
    PluginConfig plugin_config( testConfig(), params );
    ExtractedData* data = reader.extractData(plugin_config, 0);

    // test update data
    reader.updateData(*data);

    // test data
    EXPECT(data->size() > 0);
    EXPECT(!data->isEmpty());

    // test getting config back
    EXPECT_EQUAL(data->config().users().size(), 2);
    EXPECT_EQUAL(data->config().parameters().size(), 2);
    EXPECT_EQUAL(data->config().outputStrategy(), "CovJSON");

    // bounding boxes do not depend on the step
    std::unique_ptr<ExtractedData> data_other_step(reader.extractData(plugin_config, 123));
    EXPECT_EQUAL(data_other_step->size(), data->size());

    delete data;
}


CASE("test reader - moving circle") {

    std::string conf_str(R"YAML({
        "writer": "CSV",
        "requests": [
            {
                "user": "user11",
                "tag": "tc",
                "extractions": [
                    {"type": "circle", "track": {
                        "interpolation": "linear",
                        "points": [
                            {"step": 0,  "lat": 0.0, "lon": 0.0,  "radius": 1500},
                            {"step": 10, "lat": 0.0, "lon": 90.0, "radius": 1500}
                        ]
                    }}
                ]
            },
            {
                "user": "user22",
                "tag": "tc-none",
                "extractions": [
                    {"type": "circle", "track": {
                        "interpolation": "none",
                        "points": [
                            {"step": 0,  "lat": 0.0, "lon": 0.0,  "radius": 1500},
                            {"step": 10, "lat": 0.0, "lon": 90.0, "radius": 1500}
                        ]
                    }}
                ]
            }
        ]
    })YAML");
    eckit::YAMLConfiguration conf(conf_str);

    std::vector<std::string> params = {"field1"};
    std::vector<plume::data::FieldView> fields;

    atlas::Grid grid("O8");
    atlas::util::Config grid_config;
    grid_config.set("halo", 0);
    grid_config.set("levels", 1);
    grid_config.set("periodic_points", true);
    atlas::functionspace::StructuredColumns fs(grid, atlas::grid::Partitioner("equal_regions"), grid_config);
    for (const auto& param: params) {
        fields.push_back(plume::data::FieldView(fs.createField<FIELD_TYPE_REAL>(atlas::option::name(param))));
    }

    DataReader reader(fields);
    PluginConfig plugin_config(conf, params);
    EXPECT(plugin_config.hasTimeDependentAreas());

    auto lons_of_user = [](const ExtractedData& data, const std::string& user) {
        std::vector<double> lons;
        for (int i : data.find({.user=user})) {
            double lon = data.get_lon(i);
            lons.push_back(lon > 180.0 ? lon - 360.0 : lon);
        }
        return lons;
    };

    std::unique_ptr<ExtractedData> data0(reader.extractData(plugin_config, 0));
    std::unique_ptr<ExtractedData> data5(reader.extractData(plugin_config, 5));
    std::unique_ptr<ExtractedData> data10(reader.extractData(plugin_config, 10));
    std::unique_ptr<ExtractedData> data11(reader.extractData(plugin_config, 11));

    // linear: circle around lon 0 at step 0, lon 45 at step 5, lon 90 at step 10
    auto lons0 = lons_of_user(*data0, "user11");
    auto lons5 = lons_of_user(*data5, "user11");
    auto lons10 = lons_of_user(*data10, "user11");
    EXPECT(!lons0.empty());
    EXPECT(!lons5.empty());
    EXPECT(!lons10.empty());
    for (double lon : lons0)  { EXPECT(std::abs(lon) < 20.0); }
    for (double lon : lons5)  { EXPECT(std::abs(lon - 45.0) < 20.0); }
    for (double lon : lons10) { EXPECT(std::abs(lon - 90.0) < 20.0); }

    // none: only at exact steps
    EXPECT_EQUAL(lons_of_user(*data0, "user22").size(), lons0.size());
    EXPECT(lons_of_user(*data5, "user22").empty());
    EXPECT_EQUAL(lons_of_user(*data10, "user22").size(), lons10.size());

    // outside the track range
    EXPECT(data11->isEmpty());
}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}