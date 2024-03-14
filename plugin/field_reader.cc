#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "eckit/exception/Exceptions.h"

#include "atlas/array.h"
#include "atlas/functionspace/NodeColumns.h"

#include "field_reader.h"


namespace area_extractor {

FieldsReader::FieldsReader(std::vector<UserRequest> requests, std::vector<atlas::Field> fields) : 
    fields_{fields},
    numProcValues_{0} {

    lonlats_ = fields_[0].functionspace().lonlat();
    ghost_ = fields_[0].functionspace().ghost();

    // here we go through the requests and find points to extract..
    setupStorage(requests);

}
FieldsReader::~FieldsReader() {

}

void FieldsReader::setupStorage(const std::vector<UserRequest>& requests) {

    std::vector<atlas::array::ArrayView<const FIELD_TYPE_REAL,2>> fieldViewVector;
    for (const auto& field : fields_){
        fieldViewVector.push_back(atlas::array::make_view<const FIELD_TYPE_REAL,2>(field));
    }
    
    auto lonLatArray = atlas::array::make_view<const double,2>( lonlats_ );
    auto ghostArray = atlas::array::make_view<int,1>( ghost_ );

    double pointLat;
    double pointLon;

    int nlevs = fields_[0].levels();

    for (int iPt = 0; iPt < lonLatArray.shape(0); iPt++) {

        if (!ghostArray(iPt)) {

            pointLon = lonLatArray(iPt, 0);
            pointLat = lonLatArray(iPt, 1);

            for (const auto& request: requests) {
                for (const auto& area : request.areas()) {

                    if (area.isPointInside(pointLat, pointLon)) {
                        for (int iField=0; iField<fields_.size(); iField++){
                            for (int iLev=0; iLev<nlevs; iLev++){
                                users_.push_back(request.user());
                                area_idxs_.push_back(area.id());
                                point_idxs_.push_back(iPt);
                                lats_.push_back(pointLat);
                                lons_.push_back(pointLon);
                                levs_.push_back(iLev);
                                params_.push_back(fields_[iField].name());
                                values_.push_back(fieldViewVector[iField](iPt,iLev));
                            }
                        }
                    }
                    
                }
            }
        }
    }


    // update n values
    numProcValues_ = values_.size();

}

// Read the fields and update the values
void FieldsReader::read() {

    // Convenience map fieldName->fieldView
    std::map<std::string, atlas::Field> fieldViewMap;
    for (const auto& field : fields_){
        fieldViewMap.insert({field.name(), field});
    }

    for (int iVal=0; iVal<numProcValues_; iVal++) {

        std::string paramName = params_[iVal];
        atlas::Field field = fieldViewMap[paramName];
        auto arr = atlas::array::make_view<const FIELD_TYPE_REAL,2>(field);

        int ptIDX = point_idxs_[iVal];
        int iLev = levs_[iVal];

        values_[iVal] = arr(ptIDX, iLev);
    }

}


void FieldsReader::writeFile(std::string filename) {

    // Write only if it owns values
    if (ownsValues()) {

        std::ofstream outfile;

        try {
            outfile.open(filename);

            // header 
            outfile << "user,area_idx,point_idx,lat,lon,lev,param,value" << std::endl;

            // values
            for (int iVal=0; iVal<numProcValues_; iVal++) {
                outfile << users_[iVal] << ",";
                outfile << area_idxs_[iVal] << ",";
                outfile << point_idxs_[iVal] << ",";
                outfile << lats_[iVal] << ",";
                outfile << lons_[iVal] << ",";
                outfile << levs_[iVal] << ",";
                outfile << params_[iVal] << ",";
                outfile << values_[iVal];
                outfile << std::endl;
            }

            outfile.close();
        } catch (std::exception& e) {
            eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
        }
    }
}


bool FieldsReader::ownsValues() {
    return numProcValues_;
}

} // namespace area_extractor