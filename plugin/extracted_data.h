#pragma once

#include <vector>
#include <string>

#include "plugin_types.h"

namespace area_extractor {


class ExtractedData {

public:

    ExtractedData();
    ~ExtractedData();

    void addPoint(std::string users,
                  int area_idxs,
                  int point_idxs,
                  double lats,
                  double lons,
                  int levs,
                  std::string params,
                  FIELD_TYPE_REAL values);

    const std::string& get_user(int pIdx) const {
        return users_[pIdx];
    }

    const int& get_area_idx(int pIdx) const {
        return area_idxs_[pIdx];
    }

    const int& get_idx(int pIdx) const {
        return point_idxs_[pIdx];
    }

    const double& get_lat(int pIdx) const {
        return lats_[pIdx];
    }

    const double& get_lon(int pIdx) const {
        return lons_[pIdx];
    }

    const int& get_lev(int pIdx) const {
        return levs_[pIdx];
    }

    const std::string& get_param(int pIdx) const {
        return params_[pIdx];
    }

    const FIELD_TYPE_REAL& get_value(int pIdx) const {
        return values_[pIdx];
    }

    void set_value(int pIdx, FIELD_TYPE_REAL value) {
        values_[pIdx] = value;
    }

    int size() const {
        return users_.size();
    };

    bool isEmpty() const {
        return !users_.size();
    }

private:

    // internal storage
    std::vector<std::string> users_;
    std::vector<int> area_idxs_;
    std::vector<int> point_idxs_;
    std::vector<double> lats_;
    std::vector<double> lons_;
    std::vector<int> levs_;
    std::vector<std::string> params_;
    std::vector<FIELD_TYPE_REAL> values_;

};


} // namespace area_extractor