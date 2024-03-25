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

    struct FilterKey {
        std::string user;
        int areaIdx{-1};
        // std::string param;
    };

public:

    ExtractedData(const PluginConfig& config);
    ~ExtractedData();

    void addPoint(int user,
                  int area_idx,
                  int point_idx,
                  double lat,
                  double lon,
                  int lev,
                  const std::vector<FIELD_TYPE_REAL>& values);

    int get_user_uid(int pIdx) const { return user_uids_[pIdx]; }

    std::string get_user(int pIdx) const { return config_.userName(user_uids_[pIdx]); }
    int get_area_idx(int pIdx) const { return area_idxs_[pIdx]; }
    int get_idx(int pIdx) const { return point_idxs_[pIdx]; }
    double get_lat(int pIdx) const { return lats_[pIdx]; }
    double get_lon(int pIdx) const { return lons_[pIdx]; }
    int get_lev(int pIdx) const { return levs_[pIdx]; }


    std::string get_param(int pIdx) const { return config_.parameters()[pIdx]; }

    static FilterKey defaultFilterKey() { return {}; }


    // all values for a certain point
    std::vector<FIELD_TYPE_REAL> get_values(int pIdx) const {
        std::vector<FIELD_TYPE_REAL> valuestmp(param_size_);
        for (size_t iparam=0; iparam<param_size_; iparam++) {
            valuestmp[iparam] = values_[iparam][pIdx];
        }
        return valuestmp;
    }

    // all point values for a certain param
    const std::vector<FIELD_TYPE_REAL>& get_param_values(int iparam) const {        
        return values_[iparam];
    }    

    void set_value(int pIdx, const std::vector<FIELD_TYPE_REAL>& values) {
        for (size_t iparam=0; iparam<param_size_; iparam++) {
            values_[iparam][pIdx] = values[iparam];
        }
    }

    int size() const { return point_idxs_.size(); }

    bool isEmpty() const { return !point_idxs_.size(); }

    // Collect data in a rank
    ExtractedData* gather(int collectRank) const;

    // Filter the data according to user name
    ExtractedData filterUser(const std::string& userName) const ;

    // Filter the data according to areaID (rel to each user)
    ExtractedData filterArea(int areaID) const;

    // slice from indexes
    ExtractedData slice(const std::vector<int>& idxs) const;

    // Plugin configuration
    const PluginConfig& config() const {return config_;}

    std::vector<int> find(FilterKey key = defaultFilterKey()) const ;

private:

    void push_values_(const std::vector<FIELD_TYPE_REAL>& values) {
        for (size_t iparam=0; iparam<param_size_; iparam++) {
            values_[iparam].push_back(values[iparam]);
        }
    }



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

    // param values at each point
    size_t param_size_;

    // DIMS: [n_params, n_points]
    std::vector<std::vector<FIELD_TYPE_REAL>> values_;

};


} // namespace area_extractor