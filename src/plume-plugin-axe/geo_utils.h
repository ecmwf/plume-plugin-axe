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
#pragma once

#include <cmath>
#include <string>

#include "eckit/exception/Exceptions.h"


namespace area_extractor {

// Earth radius [km] (same value used by Atlas/IFS)
inline constexpr double EARTH_RADIUS_KM = 6371.229;

inline constexpr double PI = 3.14159265358979323846;

inline constexpr double DEG_TO_RAD = PI / 180.0;

inline constexpr double RAD_TO_DEG = 180.0 / PI;

// km per degree of latitude
inline constexpr double KM_PER_DEG_LAT = EARTH_RADIUS_KM * DEG_TO_RAD;

// wrap a longitude difference to (-180, 180]
inline double wrapLonDiff(double dlon) {
    dlon = std::fmod(dlon, 360.0);
    if (dlon > 180.0) {
        dlon -= 360.0;
    } else if (dlon <= -180.0) {
        dlon += 360.0;
    }
    return dlon;
}

// wrap a longitude to (-180, 180]
inline double wrapLon(double lon) {
    return wrapLonDiff(lon);
}

// validate a circle definition (prefix is prepended to the error message)
inline void checkCircle(const std::string& prefix, double lat, double radius) {
    if (lat < -90.0 || lat > 90.0) {
        throw eckit::UserError(prefix + "circle latitude must be in [-90, 90]", Here());
    }
    if (!(radius > 0.0)) {
        throw eckit::UserError(prefix + "circle radius must be > 0 [km]", Here());
    }
}

} // namespace area_extractor
