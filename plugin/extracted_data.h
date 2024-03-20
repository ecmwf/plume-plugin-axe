#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

#include "plugin_types.h"
#include "plugin_config.h"

namespace area_extractor {


class ExtractedData {

public:

    enum class Filter {
        USER,
        AREA,
        PARAM
    };

    ExtractedData(const PluginConfig& config);
    ~ExtractedData();

    void addPoint(int user,
                  int area_idx,
                  int point_idx,
                  double lat,
                  double lon,
                  int lev,
                  int param,
                  FIELD_TYPE_REAL value);

    int get_user_uid(int pIdx) const { return user_uids_[pIdx]; }

    std::string get_user(int pIdx) const { return config_.userName(user_uids_[pIdx]); }

    const std::vector<int>& get_users() const { return user_uids_; }

    int get_area_idx(int pIdx) const { return area_idxs_[pIdx]; }

    const std::vector<int>& get_area_idxs() const { return area_idxs_; }

    int get_idx(int pIdx) const { return point_idxs_[pIdx]; }

    const std::vector<int>& get_idxs() const { return point_idxs_; }

    double get_lat(int pIdx) const { return lats_[pIdx]; }

    const std::vector<double>& get_lats() const { return lats_; }

    double get_lon(int pIdx) const { return lons_[pIdx]; }

    const std::vector<double>& get_lons() const { return lons_; }

    int get_lev(int pIdx) const { return levs_[pIdx]; }

    const std::vector<int>& get_levs() const { return levs_; }

    int get_param_uid(int pIdx) const { return param_uids_[pIdx]; }

    std::string get_param(int pIdx) const { return config_.paramName(param_uids_[pIdx]); }

    const std::vector<int>& get_params() const { return param_uids_; }

    const FIELD_TYPE_REAL& get_value(int pIdx) const { return values_[pIdx]; }

    const std::vector<FIELD_TYPE_REAL>& get_values() const { return values_; }

    void set_value(int pIdx, FIELD_TYPE_REAL value) {
        values_[pIdx] = value;
    }

    int size() const { return point_idxs_.size(); }

    bool isEmpty() const { return !point_idxs_.size(); }


    // Collect data in a rank
    ExtractedData gather(int collectRank);


    // // Filter the data according to a [USER | AREA | PARAM]
    // template <typename T>
    // ExtractedData filter(Filter filter, T value) {
        
    //     std::vector<size_t> filtered_idxs;

    //     if (filter == ExtractedData::Filter::USER){
    //         std::vector<char>::iterator itBegin = user_uids_.begin();
    //         std::vector<char>::iterator itEnd = user_uids_.end();

    //         size_t cc = 0;
    //         for (std::vector<char>::iterator it = itBegin; it != itEnd; it+=nameBufferLength(), cc++ ) {
    //             std::string str(it, it+nameBufferLength());
    //             filtered_idxs.push_back(cc);
    //         }

    //         std::cout << "filtered_idxs.size(): " << filtered_idxs.size() << std::endl;
    //     }

    //     ExtractedData data;

    //     return data;

    // }

private:

    // configuration that describes the data extraction
    PluginConfig config_;

    // internal storage
    std::vector<int> user_uids_;
    std::vector<int> area_idxs_;
    std::vector<int> point_idxs_;
    std::vector<double> lats_;
    std::vector<double> lons_;
    std::vector<int> levs_;
    std::vector<int> param_uids_;
    std::vector<FIELD_TYPE_REAL> values_;

};


} // namespace area_extractor