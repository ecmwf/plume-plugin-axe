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


#include "area.h"
#include "circle_algorithm.h"
#include "data_reader.h"
#include "extracted_data.h"
#include "geo_utils.h"
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
                    {"type": "circle", "algorithm": {
                        "name": "track",
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
                    {"type": "circle", "algorithm": {
                        "name": "track",
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


CASE("test reader - wind-advected circle") {

    std::string conf_str(R"YAML({
        "writer": "CSV",
        "requests": [
            {
                "user": "user11",
                "tag": "tc",
                "extractions": [
                    {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 0.0, "lon": 0.0, "radius": 1500}},
                    {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 0.0, "lon": 0.0, "radius": 1}},
                    {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 0.0, "lon": 0.0,
                                                     "radius": {"start": 1000, "end": 2000, "time-interval": 24}}}
                ]
            }
        ]
    })YAML");
    eckit::YAMLConfiguration conf(conf_str);

    // uniform eastward wind of 50 m/s
    const double uWind = 50.0;
    const double tstep = 3600.0;
    std::vector<std::string> params = {"u;hl;5000", "v;hl;5000"};
    std::vector<double> values = {uWind, 0.0};
    std::vector<plume::data::FieldView> fields;

    atlas::Grid grid("O8");
    atlas::util::Config grid_config;
    grid_config.set("halo", 0);
    grid_config.set("levels", 1);
    grid_config.set("periodic_points", true);
    atlas::functionspace::StructuredColumns fs(grid, atlas::grid::Partitioner("equal_regions"), grid_config);
    for (size_t i = 0; i < params.size(); i++) {
        atlas::Field field = fs.createField<FIELD_TYPE_REAL>(atlas::option::name(params[i]));
        auto view = atlas::array::make_view<FIELD_TYPE_REAL,2>(field);
        for (atlas::idx_t j = 0; j < view.shape(0); j++) {
            view(j, 0) = values[i];
        }
        fields.push_back(plume::data::FieldView(field));
    }

    DataReader reader(fields);
    PluginConfig plugin_config(conf, params);
    EXPECT(plugin_config.hasTimeDependentAreas());
    EXPECT(plugin_config.requiresTimeStep());

    auto algorithmOf = [&](int iArea) -> const WindAdvectionAlgorithm& {
        auto area = std::dynamic_pointer_cast<const AlgorithmCircle>(plugin_config.requests()[0].areas()[iArea]);
        EXPECT(area);
        auto algorithm = dynamic_cast<const WindAdvectionAlgorithm*>(&area->algorithm());
        EXPECT(algorithm);
        return *algorithm;
    };

    auto lons_of_area = [](const ExtractedData& data, int iArea) {
        std::vector<double> lons;
        for (int i : data.find({.user="user11", .areaIdx=iArea})) {
            double lon = data.get_lon(i);
            lons.push_back(lon > 180.0 ? lon - 360.0 : lon);
        }
        return lons;
    };

    // first update: the configured circle is kept
    reader.updateAreas(plugin_config, 0, tstep);
    EXPECT_EQUAL(algorithmOf(0).circle().lon, 0.0);
    EXPECT_EQUAL(algorithmOf(2).circle().radius, 1000.0);
    std::unique_ptr<ExtractedData> data0(reader.extractData(plugin_config, 0));

    // same step again: no move
    reader.updateAreas(plugin_config, 0, tstep);
    EXPECT_EQUAL(algorithmOf(0).circle().lon, 0.0);

    // after 10 steps: moved east by uWind * tstep * 10 [m]
    reader.updateAreas(plugin_config, 10, tstep);
    double expectedLon = uWind * tstep * 10 / (EARTH_RADIUS_KM * 1000.0) * RAD_TO_DEG;
    EXPECT(std::abs(algorithmOf(0).circle().lon - expectedLon) < 1e-6);
    EXPECT(std::abs(algorithmOf(0).circle().lat) < 1e-9);
    std::unique_ptr<ExtractedData> data10(reader.extractData(plugin_config, 10));

    // the extracted points follow the circle
    auto lons0 = lons_of_area(*data0, 0);
    auto lons10 = lons_of_area(*data10, 0);
    EXPECT(!lons0.empty());
    EXPECT(!lons10.empty());
    for (double lon : lons0)  { EXPECT(std::abs(lon) < 20.0); }
    for (double lon : lons10) { EXPECT(std::abs(lon - expectedLon) < 20.0); }
    EXPECT(lons0 != lons10);

    // a circle without grid points is not moved
    EXPECT(lons_of_area(*data10, 1).empty());
    EXPECT_EQUAL(algorithmOf(1).circle().lon, 0.0);

    // time-dependent radius: 1000 km at the first step, linear over 24 h (TSTEP = 1 h), then 2000 km
    EXPECT(std::abs(algorithmOf(2).circle().radius - (1000.0 + 1000.0 * 10.0 / 24.0)) < 1e-9);
    EXPECT(std::abs(algorithmOf(2).circle().lon - expectedLon) < 1e-6);
    size_t nPoints0 = lons_of_area(*data0, 2).size();
    size_t nPoints10 = lons_of_area(*data10, 2).size();
    EXPECT(nPoints0 > 0);
    EXPECT(nPoints10 >= nPoints0);

    reader.updateAreas(plugin_config, 30, tstep);
    EXPECT_EQUAL(algorithmOf(2).circle().radius, 2000.0);
    std::unique_ptr<ExtractedData> data30(reader.extractData(plugin_config, 30));
    EXPECT(lons_of_area(*data30, 2).size() >= nPoints10);
}


CASE("test reader - wind-advected circle with inner radius") {

    std::string conf_str(R"YAML({
        "writer": "CSV",
        "requests": [
            {
                "user": "user11",
                "tag": "tc",
                "extractions": [
                    {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 0.0, "lon": 0.0, "radius": 1500}},
                    {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 0.0, "lon": 0.0, "radius": 1500, "inner_radius": 800}},
                    {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 0.0, "lon": 0.0, "radius": 1500, "inner_radius": 2000}}
                ]
            }
        ]
    })YAML");
    eckit::YAMLConfiguration conf(conf_str);

    // eastward wind: 50 m/s within 800 km of (0, 0), 10 m/s elsewhere
    // (O8 points nearest to (0, 0): 2 at ~606 km, 4 at ~1030 km, next at ~1772 km)
    const double uInner = 50.0;
    const double uOuter = 10.0;
    const double innerRadius = 800.0;
    const double tstep = 3600.0;
    std::vector<std::string> params = {"u;hl;5000", "v;hl;5000"};
    std::vector<plume::data::FieldView> fields;

    atlas::Grid grid("O8");
    atlas::util::Config grid_config;
    grid_config.set("halo", 0);
    grid_config.set("levels", 1);
    grid_config.set("periodic_points", true);
    atlas::functionspace::StructuredColumns fs(grid, atlas::grid::Partitioner("equal_regions"), grid_config);
    atlas::Field lonlats = fs.lonlat();
    auto lonLatArray = atlas::array::make_view<const double,2>(lonlats);
    for (const auto& param : params) {
        atlas::Field field = fs.createField<FIELD_TYPE_REAL>(atlas::option::name(param));
        auto view = atlas::array::make_view<FIELD_TYPE_REAL,2>(field);
        for (atlas::idx_t j = 0; j < view.shape(0); j++) {
            bool inner = greatCircleDistanceKm(lonLatArray(j, 1), lonLatArray(j, 0), 0.0, 0.0) <= innerRadius;
            view(j, 0) = (param[0] == 'u') ? (inner ? uInner : uOuter) : 0.0;
        }
        fields.push_back(plume::data::FieldView(field));
    }

    DataReader reader(fields);
    PluginConfig plugin_config(conf, params);

    auto algorithmOf = [&](int iArea) -> const WindAdvectionAlgorithm& {
        auto area = std::dynamic_pointer_cast<const AlgorithmCircle>(plugin_config.requests()[0].areas()[iArea]);
        EXPECT(area);
        auto algorithm = dynamic_cast<const WindAdvectionAlgorithm*>(&area->algorithm());
        EXPECT(algorithm);
        return *algorithm;
    };

    auto points_of_area = [](const ExtractedData& data, int iArea) {
        return data.find({.user="user11", .areaIdx=iArea}).size();
    };

    EXPECT(!algorithmOf(0).innerRadius().has_value());
    EXPECT_EQUAL(*algorithmOf(1).innerRadius(), innerRadius);

    // the inner radius does not change the extracted points
    reader.updateAreas(plugin_config, 0, tstep);
    std::unique_ptr<ExtractedData> data0(reader.extractData(plugin_config, 0));
    EXPECT(points_of_area(*data0, 0) > 0);
    EXPECT_EQUAL(points_of_area(*data0, 1), points_of_area(*data0, 0));
    EXPECT_EQUAL(points_of_area(*data0, 2), points_of_area(*data0, 0));

    reader.updateAreas(plugin_config, 1, tstep);
    double lonFull = algorithmOf(0).circle().lon;
    double lonAnnulus = algorithmOf(1).circle().lon;
    double lonIgnored = algorithmOf(2).circle().lon;

    // annulus: moved by the outer wind only
    double expectedAnnulus = uOuter * tstep / (EARTH_RADIUS_KM * 1000.0) * RAD_TO_DEG;
    EXPECT(std::abs(lonAnnulus - expectedAnnulus) < 1e-6);

    // full circle: the inner wind also contributes
    std::cout << "lonFull: " << lonFull << ", lonAnnulus: " << lonAnnulus << std::endl;
    EXPECT(lonFull > lonAnnulus + 1e-4);

    // inner radius larger than the radius: ignored
    EXPECT_EQUAL(lonIgnored, lonFull);
}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}