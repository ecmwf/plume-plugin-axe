#include <map>
#include <iostream>

#include "atlas/array.h"
#include "atlas/functionspace/NodeColumns.h"


#include "plugin_types.h"
#include "data_reader.h"


namespace area_extractor {

DataReader::DataReader(std::vector<atlas::Field> fields) : 
    fields_{fields} {

    for (const auto& field : fields_){
        fieldViewMap_.insert({field.name(), field});
    }
}


DataReader::~DataReader() {}


ExtractedData* DataReader::extractData(const PluginConfig& config) {

    // create_data
    ExtractedData* data = new ExtractedData{config};

    std::vector<atlas::array::ArrayView<const FIELD_TYPE_REAL,2>> fieldViewVector;
    for (const auto& field : fields_){
        fieldViewVector.push_back(atlas::array::make_view<const FIELD_TYPE_REAL,2>(field));
    }

    lonlats_ = fields_[0].functionspace().lonlat();    
    auto lonLatArray = atlas::array::make_view<const double,2>( lonlats_ );

    ghost_ = fields_[0].functionspace().ghost();
    auto ghostArray = atlas::array::make_view<int,1>( ghost_ );

    double pointLat;
    double pointLon;

    int nlevs = fields_[0].levels();

    for (int iPt = 0; iPt < lonLatArray.shape(0); iPt++) {

        if (!ghostArray(iPt)) {

            pointLon = lonLatArray(iPt, 0);
            pointLat = lonLatArray(iPt, 1);

            for (const auto& request: config.requests()) {
                for (const auto& area : request.areas()) {

                    if (area.isPointInside(pointLat, pointLon)) {

                        for (int iField=0; iField<fields_.size(); iField++){
                            for (int iLev=0; iLev<nlevs; iLev++){
                                int user_uid = config.user_uid(request.user());
                                int param_uid = config.param_uid(fields_[iField].name());
                                data->addPoint(user_uid, area.id(), iPt, pointLat, pointLon, iLev, param_uid, fieldViewVector[iField](iPt,iLev));
                            }
                        }
                    }   
                }
            }
        }
    }

    return data;
}

// Read the fields and update the values
void DataReader::updateData(ExtractedData& data) {

    for (int iPt=0; iPt<data.size(); iPt++) {

        std::string paramName = data.get_param(iPt);

        int ptIDX = data.get_idx(iPt);
        int iLev = data.get_lev(iPt);

        atlas::Field field = fieldViewMap_[paramName];
        auto arr = atlas::array::make_view<const FIELD_TYPE_REAL,2>(field);

        data.set_value(iPt, arr(ptIDX, iLev));
    }
}

} // namespace area_extractor