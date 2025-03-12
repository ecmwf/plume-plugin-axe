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

#include "plugin/area.h"
#include "plugin/area_extractor.h"


using namespace eckit::testing;

namespace eckit {
namespace test {

CASE("test area") {

    // test extraction area
    area_extractor::ExtractionArea area(1, 20.0, 10.0, 40.0, 30.0);

    EXPECT_EQUAL(area.id(), 1);
    EXPECT_EQUAL(area.north(), 20.0);
    EXPECT_EQUAL(area.south(), 10.0);
    EXPECT_EQUAL(area.east(), 40.0);
    EXPECT_EQUAL(area.west(), 30.0);

    EXPECT(area.isPointInside(15.0, 35.0));
    EXPECT(!area.isPointInside(5.0, 35.0));
    EXPECT(!area.isPointInside(15.0, 45.0));
    EXPECT(!area.isPointInside(25.0, 35.0));
    EXPECT(!area.isPointInside(15.0, 25.0));    
}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}