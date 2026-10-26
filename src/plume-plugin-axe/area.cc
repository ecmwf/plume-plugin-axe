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

#include <algorithm>
#include <cmath>

#include "eckit/exception/Exceptions.h"

#include "area.h"


namespace area_extractor {

namespace {

// Earth radius [km] (same value used by Atlas/IFS)
constexpr double EARTH_RADIUS_KM = 6371.229;

constexpr double PI = 3.14159265358979323846;

constexpr double DEG_TO_RAD = PI / 180.0;

// km per degree of latitude
constexpr double KM_PER_DEG_LAT = EARTH_RADIUS_KM * DEG_TO_RAD;

// wrap a longitude difference to (-180, 180]
double wrapLonDiff(double dlon) {
    dlon = std::fmod(dlon, 360.0);
    if (dlon > 180.0) {
        dlon -= 360.0;
    } else if (dlon <= -180.0) {
        dlon += 360.0;
    }
    return dlon;
}

} // namespace


double greatCircleDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    double phi1 = lat1 * DEG_TO_RAD;
    double phi2 = lat2 * DEG_TO_RAD;
    double sdphi = std::sin((phi2 - phi1) / 2.0);
    double sdlam = std::sin((lon2 - lon1) * DEG_TO_RAD / 2.0);
    double h = sdphi * sdphi + std::cos(phi1) * std::cos(phi2) * sdlam * sdlam;
    return 2.0 * EARTH_RADIUS_KM * std::asin(std::sqrt(std::min(1.0, h)));
}


ExtractionArea::ExtractionArea(int area_id,  double north, double south, double east, double west) :
    area_id_{area_id}, shape_{Shape::BBox}, north_{north}, south_{south}, east_{east}, west_{west} {
}


ExtractionArea::ExtractionArea(int area_id, const Circle& circle) :
    area_id_{area_id}, shape_{Shape::Circle},
    track_{TrackPoint{0, circle.lat, circle.lon, circle.radius}},
    timeDependent_{false} {
}


ExtractionArea::ExtractionArea(int area_id, std::vector<TrackPoint> track, Interpolation interpolation) :
    area_id_{area_id}, shape_{Shape::Circle}, track_{std::move(track)},
    interpolation_{interpolation}, timeDependent_{true} {

    ASSERT(!track_.empty());
    std::sort(track_.begin(), track_.end(),
              [](const TrackPoint& a, const TrackPoint& b){ return a.step < b.step; });
}


ExtractionArea::~ExtractionArea(){
}


std::optional<Circle> ExtractionArea::circleAt(int step) const {

    if (shape_ != Shape::Circle) {
        return std::nullopt;
    }

    if (!timeDependent_) {
        const auto& p = track_.front();
        return Circle{p.lat, p.lon, p.radius};
    }

    // outside the track range
    if (step < track_.front().step || step > track_.back().step) {
        return std::nullopt;
    }

    // first track point with step > requested step
    auto upper = std::upper_bound(track_.begin(), track_.end(), step,
                                  [](int s, const TrackPoint& p){ return s < p.step; });
    const TrackPoint& a = *(upper - 1);

    // exact match
    if (a.step == step) {
        return Circle{a.lat, a.lon, a.radius};
    }

    if (interpolation_ == Interpolation::None) {
        return std::nullopt;
    }

    // linear interpolation (longitude along the shortest arc)
    const TrackPoint& b = *upper;
    double t = static_cast<double>(step - a.step) / static_cast<double>(b.step - a.step);
    double lat = a.lat + t * (b.lat - a.lat);
    double lon = a.lon + t * wrapLonDiff(b.lon - a.lon);
    double radius = a.radius + t * (b.radius - a.radius);

    return Circle{lat, lon, radius};
}


bool ExtractionArea::isPointInside(double lat, double lon, int step) const {

    if (shape_ == Shape::BBox) {
        return ((lat <= north_) && (lat >= south_) && (lon <= east_) && (lon >= west_));
    }

    std::optional<Circle> circle = circleAt(step);
    if (!circle) {
        return false;
    }

    // cheap latitude-band pre-filter
    if (std::abs(lat - circle->lat) * KM_PER_DEG_LAT > circle->radius) {
        return false;
    }

    return greatCircleDistanceKm(lat, lon, circle->lat, circle->lon) <= circle->radius;
}


bool ExtractionArea::isPointInside(double lat, double lon) const {
    ASSERT(!timeDependent_);
    return isPointInside(lat, lon, 0);
}


std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj) {
    ss << "{id:" << obj.area_id_ << ",";
    if (obj.shape_ == ExtractionArea::Shape::BBox) {
        ss << "bbox:(north:" << obj.north_ << ",";
        ss << "south:" << obj.south_ << ",";
        ss << "east:" << obj.east_ << ",";
        ss << "west:" << obj.west_ << ")";
    } else if (!obj.timeDependent_) {
        const auto& p = obj.track_.front();
        ss << "circle:(lat:" << p.lat << ",";
        ss << "lon:" << p.lon << ",";
        ss << "radius:" << p.radius << ")";
    } else {
        ss << "track:(points:" << obj.track_.size() << ",";
        ss << "steps:" << obj.track_.front().step << "-" << obj.track_.back().step << ",";
        ss << "interpolation:" << (obj.interpolation_ == ExtractionArea::Interpolation::Linear ? "linear" : "none") << ")";
    }
    ss << "}";
    return ss;
}


} // namespace area_extractor
