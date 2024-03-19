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

    std::string get_user(int pIdx) const {
        return users_.data() + pIdx * nameBufferLength();
    }

    const std::vector<char>& get_users() const {
        return users_;
    }

    const int& get_area_idx(int pIdx) const {
        return area_idxs_[pIdx];
    }

    const std::vector<int>& get_area_idxs() const {
        return area_idxs_;
    }

    const int& get_idx(int pIdx) const {
        return point_idxs_[pIdx];
    }

    const std::vector<int>& get_idxs() const {
        return point_idxs_;
    }

    const double& get_lat(int pIdx) const {
        return lats_[pIdx];
    }

    const std::vector<double>& get_lats() const {
        return lats_;
    }

    const double& get_lon(int pIdx) const {
        return lons_[pIdx];
    }

    const std::vector<double>& get_lons() const {
        return lons_;
    }

    const int& get_lev(int pIdx) const {
        return levs_[pIdx];
    }

    const std::vector<int>& get_levs() const {
        return levs_;
    }

    std::string get_param(int pIdx) const {
        return params_.data() + pIdx * nameBufferLength();
    }

    const std::vector<char>& get_params() const {
        return params_;
    }

    const FIELD_TYPE_REAL& get_value(int pIdx) const {
        return values_[pIdx];
    }

    const std::vector<FIELD_TYPE_REAL>& get_values() const {
        return values_;
    }

    void set_value(int pIdx, FIELD_TYPE_REAL value) {
        values_[pIdx] = value;
    }

    int size() const {
        return point_idxs_.size();
    };

    bool isEmpty() const {
        return !point_idxs_.size();
    }


    // collect data in a rank
    ExtractedData gather(int collectRank);


public:

    static constexpr int nameBufferLength() { return 32; }

private:

    // internal storage
    std::vector<char> users_;
    std::vector<int> area_idxs_;
    std::vector<int> point_idxs_;
    std::vector<double> lats_;
    std::vector<double> lons_;
    std::vector<int> levs_;
    std::vector<char> params_;
    std::vector<FIELD_TYPE_REAL> values_;

};


} // namespace area_extractor