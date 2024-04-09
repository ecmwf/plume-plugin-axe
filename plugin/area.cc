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

#include "area.h"


namespace area_extractor {


ExtractionArea::ExtractionArea(int area_id,  double north, double south, double east, double west) :
    area_id_{area_id}, north_{north}, south_{south}, east_{east}, west_{west} {
}


ExtractionArea::~ExtractionArea(){
}


std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj) {
    ss << "{id:" << obj.area_id_ << ",";
    ss << "bbox:(north:" << obj.north_ << ",";
    ss << "south:" << obj.south_ << ",";
    ss << "east:" << obj.east_ << ",";
    ss << "west:" << obj.west_ << ")";
    ss << "}";
    return ss;
}


} // namespace area_extractor