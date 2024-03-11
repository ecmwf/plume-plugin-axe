
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