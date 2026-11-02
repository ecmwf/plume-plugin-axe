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

#include "geo_utils.h"
#include "circle_algorithm.h"
#include "area.h"


namespace area_extractor {

namespace {

bool isPointInsideCircle(double lat, double lon, const Circle& circle) {

    // cheap latitude-band pre-filter
    if (std::abs(lat - circle.lat) * KM_PER_DEG_LAT > circle.radius) {
        return false;
    }

    return greatCircleDistanceKm(lat, lon, circle.lat, circle.lon) <= circle.radius;
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


// ---------------------- ExtractionArea ----------------------
ExtractionArea::ExtractionArea(int area_id) : area_id_{area_id} {
}


ExtractionArea::~ExtractionArea(){
}


bool ExtractionArea::isPointInside(double lat, double lon) const {
    ASSERT(!isTimeDependent());
    return isPointInside(lat, lon, 0);
}


std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj) {
    ss << "{id:" << obj.area_id_ << ",";
    obj.print(ss);
    ss << "}";
    return ss;
}


// ---------------------- ExtractionBox ----------------------
ExtractionBox::ExtractionBox(int area_id,  double north, double south, double east, double west) :
    ExtractionArea(area_id), north_{north}, south_{south}, east_{east}, west_{west} {
}


ExtractionBox::~ExtractionBox(){
}


bool ExtractionBox::isPointInside(double lat, double lon, int /*step*/) const {
    return ((lat <= north_) && (lat >= south_) && (lon <= east_) && (lon >= west_));
}


void ExtractionBox::print(std::ostream& ss) const {
    ss << "bbox:(north:" << north_ << ",";
    ss << "south:" << south_ << ",";
    ss << "east:" << east_ << ",";
    ss << "west:" << west_ << ")";
}


// ---------------------- ExtractionCircle ----------------------
ExtractionCircle::ExtractionCircle(int area_id, const Circle& circle) :
    ExtractionArea(area_id), circle_{circle} {
}


ExtractionCircle::~ExtractionCircle(){
}


bool ExtractionCircle::isPointInside(double lat, double lon, int /*step*/) const {
    return isPointInsideCircle(lat, lon, circle_);
}


void ExtractionCircle::print(std::ostream& ss) const {
    ss << "circle:(lat:" << circle_.lat << ",";
    ss << "lon:" << circle_.lon << ",";
    ss << "radius:" << circle_.radius << ")";
}


// ---------------------- AlgorithmCircle ----------------------
AlgorithmCircle::AlgorithmCircle(int area_id, std::shared_ptr<CircleAlgorithm> algorithm) :
    ExtractionArea(area_id), algorithm_{std::move(algorithm)} {

    ASSERT(algorithm_);
}


AlgorithmCircle::~AlgorithmCircle(){
}


bool AlgorithmCircle::isPointInside(double lat, double lon, int step) const {

    std::optional<Circle> circle = algorithm_->circleAt(step);
    if (!circle) {
        return false;
    }

    return isPointInsideCircle(lat, lon, *circle);
}


void AlgorithmCircle::update(const AreaUpdateContext& ctx) const {
    algorithm_->update(ctx);
}


std::vector<std::string> AlgorithmCircle::requiredFields() const {
    return algorithm_->requiredFields();
}


bool AlgorithmCircle::requiresTimeStep() const {
    return algorithm_->requiresTimeStep();
}


void AlgorithmCircle::print(std::ostream& ss) const {
    ss << "algorithm:(" << *algorithm_ << ")";
}


} // namespace area_extractor
