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
#include <map>
#include <iostream>

#include "atlas/array.h"
#include "atlas/functionspace/NodeColumns.h"
#include "eckit/exception/Exceptions.h"


#include "plugin_types.h"
#include "data_reader.h"


namespace area_extractor {

namespace {

// interpolated params are named "<param>;<levtype>;<level>" (e.g. "u;hl;70")
bool isInterpolatedField(const std::string& name) {
    return name.find(';') != std::string::npos;
}

// u, v and z are the sources of interpolation for the interpolated params
bool isInterpolationSource(const std::string& name) {
    return name == "u" || name == "v" || name == "z";
}

} // namespace


DataReader::DataReader(std::vector<plume::data::FieldView> fields) :
    fields_{fields} {

    for (const auto& field : fields_){
        fieldViewMap_.insert({field.name(), field});
    }
}


DataReader::~DataReader() {}


ExtractedData* DataReader::extractData(const PluginConfig& config, int step) {

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

    // if interpolated params are requested, u, v and z are only there as sources of
    // interpolation: they are skipped and all the other params must have one level.
    // Otherwise all params must have the same number of levels
    bool hasInterpolated = std::any_of(fields_.begin(), fields_.end(),
                                       [](const auto& field) { return isInterpolatedField(field.name()); });

    int nlevs = hasInterpolated ? 1 : fields_[0].levels();

    for (const auto& field: fields_) {
        if (hasInterpolated && isInterpolationSource(field.name())) {
            continue;
        }
        if (field.levels() != nlevs) {
            eckit::Log::error() << "DataReader::extractData: fields have different number of levels" << std::endl;
            if (hasInterpolated) {
                eckit::Log::error() << "interpolated params are requested, so all params (except interpolation sources u, v, z) must have 1 level"
                                    << ", while " << field.name() << " has " << field.levels() << " levels!" << std::endl;
            } else {
                eckit::Log::error() << fields_[0].name() << " has " << nlevs << " levels" << ", while " << field.name() << " has " << field.levels() << " levels!" << std::endl;
            }
            throw eckit::UserError("DataReader::extractData num levels error!");
        }
    }

    for (int iPt = 0; iPt < lonLatArray.shape(0); iPt++) {

        if (!ghostArray(iPt)) {

            pointLon = lonLatArray(iPt, 0);
            pointLat = lonLatArray(iPt, 1);

            for (const auto& request: config.requests()) {

                int user_uid = config.user_uid(request.user());
                for (const auto& area : request.areas()) {

                    int area_id = area->id();
                    if (area->isPointInside(pointLat, pointLon, step)) {

                        // extract values at each vert level
                        for (int iLev=0; iLev<nlevs; iLev++){

                            std::vector<FIELD_TYPE_REAL> pointValues(fields_.size());
                            for (int iField=0; iField<fields_.size(); iField++){
                                pointValues[iField] = fieldViewVector[iField](iPt,iLev);
                            }

                            data->addPoint(user_uid, area_id, iPt, pointLat, pointLon, iLev, pointValues);
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

    std::vector<FIELD_TYPE_REAL> values(fields_.size());
    for (size_t iPt=0; iPt<data.size(); iPt++) {

        // std::string paramName = data.get_param(iPt);

        int ptIDX = data.get_idx(iPt);
        int iLev = data.get_lev(iPt);

        for (size_t iparam=0; iparam<fields_.size(); iparam++){
            // atlas::Field field = fieldViewMap_[paramName];
            const auto& field = fields_[iparam];
            auto arr = atlas::array::make_view<const FIELD_TYPE_REAL,2>(field);

            values[iparam] = arr(ptIDX, iLev); 
        }

        data.set_value(iPt, values);
    }
}

} // namespace area_extractor