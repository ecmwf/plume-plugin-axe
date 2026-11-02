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
#include <vector>

#include "eckit/testing/Test.h"

#include "area.h"
#include "area_extractor.h"
#include "circle_algorithm.h"
#include "geo_utils.h"


using namespace eckit::testing;

namespace eckit {
namespace test {

CASE("test area") {

    // test extraction area
    area_extractor::ExtractionBox area(1, 20.0, 10.0, 40.0, 30.0);

    EXPECT_EQUAL(area.id(), 1);
    EXPECT_EQUAL(area.north(), 20.0);
    EXPECT_EQUAL(area.south(), 10.0);
    EXPECT_EQUAL(area.east(), 40.0);
    EXPECT_EQUAL(area.west(), 30.0);
    EXPECT(!area.isTimeDependent());

    EXPECT(area.isPointInside(15.0, 35.0));
    EXPECT(!area.isPointInside(5.0, 35.0));
    EXPECT(!area.isPointInside(15.0, 45.0));
    EXPECT(!area.isPointInside(25.0, 35.0));
    EXPECT(!area.isPointInside(15.0, 25.0));    
}


CASE("test polymorphic access") {

    std::vector<std::shared_ptr<const area_extractor::ExtractionArea>> areas{
        std::make_shared<const area_extractor::ExtractionBox>(0, 20.0, 10.0, 40.0, 30.0),
        std::make_shared<const area_extractor::ExtractionCircle>(1, area_extractor::Circle{10.0, 20.0, 200.0}),
        std::make_shared<const area_extractor::ExtractionCircle>(2, std::vector<area_extractor::TrackPoint>{{0, 10.0, 20.0, 200.0}},
                                                                 area_extractor::ExtractionCircle::Interpolation::Linear)
    };

    EXPECT(areas[0]->isPointInside(15.0, 35.0, 0));
    EXPECT(areas[1]->isPointInside(10.0, 20.0, 0));
    EXPECT(areas[2]->isPointInside(10.0, 20.0, 0));
    EXPECT(!areas[2]->isPointInside(10.0, 20.0, 1));

    EXPECT(!areas[0]->isTimeDependent());
    EXPECT(!areas[1]->isTimeDependent());
    EXPECT(areas[2]->isTimeDependent());
}


CASE("test great-circle distance") {

    // 1 degree of latitude ~ 111.2 km
    EXPECT(std::abs(area_extractor::greatCircleDistanceKm(0.0, 0.0, 1.0, 0.0) - 111.2) < 0.1);

    // independent of the longitude convention
    EXPECT(area_extractor::greatCircleDistanceKm(10.0, -10.0, 10.0, 350.0) < 1e-6);
}


CASE("test fixed circle") {

    area_extractor::ExtractionCircle area(2, area_extractor::Circle{10.0, 20.0, 200.0});

    EXPECT_EQUAL(area.id(), 2);
    EXPECT(!area.isTimeDependent());

    // ~111 km north of the centre: inside; ~222 km: outside
    EXPECT(area.isPointInside(11.0, 20.0));
    EXPECT(!area.isPointInside(12.0, 20.0));

    // same at any step
    EXPECT(area.isPointInside(11.0, 20.0, 100));
    EXPECT(area.isPointInside(10.0, 20.0, -5));
}


CASE("test circle across the dateline and near the pole") {

    // centre at lon 179.5: point at lon -179.5 is ~110 km away
    area_extractor::ExtractionCircle dateline(0, area_extractor::Circle{0.0, 179.5, 150.0});
    EXPECT(dateline.isPointInside(0.0, -179.5));
    EXPECT(dateline.isPointInside(0.0, 180.5));
    EXPECT(!dateline.isPointInside(0.0, 175.0));

    // centre at lat 89.5: point at lat 89.5 on the opposite meridian is ~111 km away
    area_extractor::ExtractionCircle polar(1, area_extractor::Circle{89.5, 0.0, 150.0});
    EXPECT(polar.isPointInside(89.5, 180.0));
    EXPECT(polar.isPointInside(90.0, 0.0));
    EXPECT(!polar.isPointInside(87.0, 180.0));
}


CASE("test track circle - linear interpolation") {

    using area_extractor::TrackPoint;
    using area_extractor::ExtractionCircle;

    // points given unsorted on purpose
    std::vector<TrackPoint> track{{10, 20.0, 40.0, 300.0},
                                  {0, 10.0, 30.0, 100.0}};
    ExtractionCircle area(3, track, ExtractionCircle::Interpolation::Linear);

    EXPECT(area.isTimeDependent());

    // exact steps
    auto c0 = area.circleAt(0);
    EXPECT(c0.has_value());
    EXPECT_EQUAL(c0->lat, 10.0);
    EXPECT_EQUAL(c0->lon, 30.0);
    EXPECT_EQUAL(c0->radius, 100.0);

    // midpoint
    auto c5 = area.circleAt(5);
    EXPECT(c5.has_value());
    EXPECT(std::abs(c5->lat - 15.0) < 1e-12);
    EXPECT(std::abs(c5->lon - 35.0) < 1e-12);
    EXPECT(std::abs(c5->radius - 200.0) < 1e-12);

    // outside the track range
    EXPECT(!area.circleAt(-1).has_value());
    EXPECT(!area.circleAt(11).has_value());
    EXPECT(!area.isPointInside(10.0, 30.0, 11));

    // the circle moves
    EXPECT(area.isPointInside(10.0, 30.0, 0));
    EXPECT(!area.isPointInside(10.0, 30.0, 10));
    EXPECT(area.isPointInside(20.0, 40.0, 10));
}


CASE("test track circle - interpolation across the dateline") {

    using area_extractor::TrackPoint;
    using area_extractor::ExtractionCircle;

    std::vector<TrackPoint> track{{0, 0.0, 179.0, 100.0},
                                  {2, 0.0, -179.0, 100.0}};
    ExtractionCircle area(0, track, ExtractionCircle::Interpolation::Linear);

    // shortest arc goes through 180, not through 0
    auto c1 = area.circleAt(1);
    EXPECT(c1.has_value());
    EXPECT(std::abs(area_extractor::greatCircleDistanceKm(c1->lat, c1->lon, 0.0, 180.0)) < 1e-6);
}


CASE("test track circle - no interpolation") {

    using area_extractor::TrackPoint;
    using area_extractor::ExtractionCircle;

    std::vector<TrackPoint> track{{0, 10.0, 30.0, 100.0},
                                  {10, 20.0, 40.0, 300.0}};
    ExtractionCircle area(0, track, ExtractionCircle::Interpolation::None);

    EXPECT(area.circleAt(0).has_value());
    EXPECT(area.circleAt(10).has_value());
    EXPECT(!area.circleAt(5).has_value());
    EXPECT(!area.isPointInside(15.0, 35.0, 5));

    // single-point track: only at that step
    ExtractionCircle single(1, {{4, 10.0, 30.0, 100.0}}, ExtractionCircle::Interpolation::Linear);
    EXPECT(single.circleAt(4).has_value());
    EXPECT(!single.circleAt(3).has_value());
    EXPECT(!single.circleAt(5).has_value());
}


CASE("test advect") {

    using area_extractor::Circle;
    using area_extractor::advect;

    const double radiusM = area_extractor::EARTH_RADIUS_KM * 1000.0;
    const double tol = 1e-9;

    // pure eastward wind at the equator
    Circle east = advect(Circle{0.0, 10.0, 100.0}, 10.0, 0.0, 900.0);
    EXPECT(std::abs(east.lat - 0.0) < tol);
    EXPECT(std::abs(east.lon - (10.0 + 10.0 * 900.0 / radiusM * area_extractor::RAD_TO_DEG)) < tol);
    EXPECT_EQUAL(east.radius, 100.0);

    // eastward wind at 60N moves twice as many degrees of longitude
    Circle east60 = advect(Circle{60.0, 10.0, 100.0}, 10.0, 0.0, 900.0);
    EXPECT(std::abs((east60.lon - 10.0) - 2.0 * (east.lon - 10.0)) < 1e-6);

    // pure northward wind
    Circle north = advect(Circle{10.0, 20.0, 100.0}, 0.0, 10.0, 900.0);
    EXPECT(std::abs(north.lat - (10.0 + 10.0 * 900.0 / radiusM * area_extractor::RAD_TO_DEG)) < tol);
    EXPECT(std::abs(north.lon - 20.0) < tol);

    // dt = 0 => no move
    Circle still = advect(Circle{10.0, 20.0, 100.0}, 10.0, 10.0, 0.0);
    EXPECT(std::abs(still.lat - 10.0) < tol);
    EXPECT(std::abs(still.lon - 20.0) < tol);

    // dateline crossing => longitude wrapped to (-180, 180]
    Circle wrapped = advect(Circle{0.0, 179.9, 100.0}, 20.0, 0.0, 3600.0);
    EXPECT(wrapped.lon < -179.0);

    // pole => latitude clamped
    Circle pole = advect(Circle{89.9, 0.0, 100.0}, 0.0, 100.0, 36000.0);
    EXPECT_EQUAL(pole.lat, 90.0);
}


namespace {

// algorithm stub: moves the circle 1 degree east at each update
class StubAlgorithm : public area_extractor::CircleAlgorithm {
public:
    area_extractor::Circle circle() const override { return circle_; }
    void update(const area_extractor::AreaUpdateContext&) override { circle_.lon += 1.0; }
    std::vector<std::string> requiredFields() const override { return {"stub-field"}; }
    bool requiresTimeStep() const override { return true; }
protected:
    void print(std::ostream& ss) const override { ss << "stub"; }
private:
    area_extractor::Circle circle_{10.0, 20.0, 50.0};
};

}  // namespace


CASE("test algorithm circle") {

    using area_extractor::AlgorithmCircle;

    auto algorithm = std::make_shared<StubAlgorithm>();
    std::shared_ptr<const area_extractor::ExtractionArea> area = std::make_shared<const AlgorithmCircle>(3, algorithm);

    EXPECT_EQUAL(area->id(), 3);
    EXPECT(area->isTimeDependent());
    EXPECT(area->requiresTimeStep());
    EXPECT_EQUAL(area->requiredFields().size(), 1);
    EXPECT_EQUAL(area->requiredFields()[0], "stub-field");

    EXPECT(area->isPointInside(10.0, 20.0, 0));
    EXPECT(!area->isPointInside(10.0, 21.0, 0));

    // the area follows the algorithm circle
    std::map<std::string, plume::data::FieldView> fields;
    area_extractor::AreaUpdateContext ctx{1, 900.0, fields, atlas::FunctionSpace()};
    area->update(ctx);
    EXPECT(!area->isPointInside(10.0, 20.0, 1));
    EXPECT(area->isPointInside(10.0, 21.0, 1));

    // boxes and fixed circles do not need runtime data
    area_extractor::ExtractionBox box(0, 20.0, 10.0, 40.0, 30.0);
    EXPECT(box.requiredFields().empty());
    EXPECT(!box.requiresTimeStep());
}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}