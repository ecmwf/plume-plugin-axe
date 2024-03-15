
#include "extracted_data.h"


namespace area_extractor {


// ---------------------- data ----------------------
ExtractedData::ExtractedData(){

}


ExtractedData::~ExtractedData(){

}

void ExtractedData::addPoint(std::string user,
                             int area_idx,
                             int point_idx,
                             double lat,
                             double lon,
                             int lev,
                             std::string param,
                             FIELD_TYPE_REAL value){

    users_.push_back(user);
    area_idxs_.push_back(area_idx);
    point_idxs_.push_back(point_idx);
    lats_.push_back(lat);
    lons_.push_back(lon);
    levs_.push_back(lev);
    params_.push_back(param);
    values_.push_back(value);

}


} // namespace area_extractor