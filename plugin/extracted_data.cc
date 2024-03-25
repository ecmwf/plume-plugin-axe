
#include "eckit/log/Log.h"
#include "eckit/mpi/Comm.h"

#include "plugin_utils.h"
#include "extracted_data.h"


namespace area_extractor {


// ---------------------- data ----------------------
ExtractedData::ExtractedData(const PluginConfig& config) : 
    config_{config}, 
    param_size_{config_.parameters().size()} {

        values_.resize(param_size_);
}


ExtractedData::~ExtractedData(){

}

void ExtractedData::addPoint(int user,
                             int area_idx,
                             int point_idx,
                             double lat,
                             double lon,
                             int lev,
                             const std::vector<FIELD_TYPE_REAL>& values){

    user_uids_.push_back(user);
    area_idxs_.push_back(area_idx);
    point_idxs_.push_back(point_idx);
    lats_.push_back(lat);
    lons_.push_back(lon);
    levs_.push_back(lev);

    push_values_(values);

}



// collect data in a rank
ExtractedData* ExtractedData::gather(int collectRank) const {

    int data_size_ = size();
    int data_size_global_;

    // global values (only meaningful at root rank)
    std::vector<int> user_uids_glob_;
    std::vector<int> area_idxs_glob_;
    std::vector<int> idxs_glob_;
    std::vector<double> lats_glob_;
    std::vector<double> lons_glob_;
    std::vector<int> levs_glob_;

    // std::vector<int> param_uids_glob_;
    // std::vector<FIELD_TYPE_REAL> values_glob_;
    std::vector<std::vector<FIELD_TYPE_REAL>> values_glob_(param_size_);

    // total number of samples for each rank
    std::vector<int> nRankSamplesVec_;
    std::vector<int> SamplesDisplVec_;

    // global data to fill-in
    ExtractedData* globalData = new ExtractedData{config_};

    size_t mpi_size = eckit::mpi::comm().size();
    size_t mpi_rank = eckit::mpi::comm().rank();

    nRankSamplesVec_.resize( mpi_size );
    SamplesDisplVec_.resize( mpi_size );

    // let's gather all nRankSamples_ in nRankSamplesVec_ in root rank
    eckit::mpi::comm().gather<int>(data_size_, nRankSamplesVec_, collectRank);

    // displacements (only meaningful at root rank)
    std::partial_sum(nRankSamplesVec_.begin(), nRankSamplesVec_.end()-1, SamplesDisplVec_.begin()+1);

    // let's count all samples in all ranks (only meaningful at root rank)
    data_size_global_ = 0;
    if (mpi_rank == collectRank) {
        data_size_global_ = int(std::accumulate(nRankSamplesVec_.begin(), nRankSamplesVec_.end(), 0));

        // Resize collective storage (only meaningful at root rank)
        user_uids_glob_.resize(data_size_global_);
        area_idxs_glob_.resize(data_size_global_);
        idxs_glob_.resize(data_size_global_);
        lats_glob_.resize(data_size_global_);
        lons_glob_.resize(data_size_global_);
        levs_glob_.resize(data_size_global_);

        // param_uids_glob_.resize(data_size_global_);
        for (size_t iparam=0; iparam<param_size_; iparam++) {
            values_glob_[iparam].resize(data_size_global_);
        }
    }

    // 2) collect all local vectors
    // void gatherv(const std::vector<T>& send, std::vector<T>& recv, const std::vector<int>& recvcounts, const std::vector<int>& displs, size_t root) const;
    eckit::mpi::comm().gatherv(user_uids_, user_uids_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(area_idxs_, area_idxs_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(point_idxs_, idxs_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(lats_, lats_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(lons_, lons_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(levs_, levs_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );

    for (size_t iparam=0; iparam<param_size_; iparam++) {
        eckit::mpi::comm().gatherv(values_[iparam], values_glob_[iparam], nRankSamplesVec_, SamplesDisplVec_, collectRank );
    }


    // ---------- write gathered data ----------
    globalData->user_uids_ = user_uids_glob_;
    globalData->area_idxs_ = area_idxs_glob_;
    globalData->point_idxs_ = idxs_glob_;
    globalData->lats_ = lats_glob_;
    globalData->lons_ = lons_glob_;
    globalData->levs_ = levs_glob_;
    globalData->values_ = values_glob_;

    return globalData;

}


ExtractedData ExtractedData::filterUser(const std::string& userName) const {
    ExtractedData data{config_};
    int userUID = config_.user_uid(userName);

    for (int i=0; i<size(); i++) {
        if (user_uids_[i] == userUID) {
            data.addPoint(user_uids_[i], area_idxs_[i], point_idxs_[i], lats_[i], lons_[i], levs_[i], get_values(i) );
        }
    }
    return data;
}


ExtractedData ExtractedData::filterArea(int areaID) const {
    ExtractedData data{config_};
    for (int i=0; i<size(); i++) {
        if (area_idxs_[i] == areaID) {
            data.addPoint(user_uids_[i], area_idxs_[i], point_idxs_[i], lats_[i], lons_[i], levs_[i], get_values(i) );
        }
    }
    return data;
}


ExtractedData ExtractedData::slice(const std::vector<int>& idxs) const {
    ExtractedData data{config_};
    for (const auto& idx : idxs) {
        data.user_uids_.push_back(user_uids_[idx]);
        data.area_idxs_.push_back(area_idxs_[idx]);
        data.point_idxs_.push_back(point_idxs_[idx]);
        data.lats_.push_back(lats_[idx]);
        data.lons_.push_back(lons_[idx]);
        data.levs_.push_back(levs_[idx]);

        // data.param_uids_.push_back(param_uids_[idx]);
        for (size_t iparam=0; iparam<param_size_; iparam++) {
            data.values_[iparam].push_back(values_[iparam][idx]);
        }
    }
    return data;
}


std::vector<int> ExtractedData::find(FilterKey key) const {

    std::vector<int> fidxs(size());
    std::iota(fidxs.begin(), fidxs.end(), 0);

    // filter out users != key.user
    if (!key.user.empty()) {
        fidxs.erase(std::remove_if(fidxs.begin(), fidxs.end(), 
                                  [this, key](const int& idx){ return config_.userName(user_uids_[idx]) != key.user; }),
                    fidxs.end() );
    }

    // filter out areas != key.areaIdx
    if (key.areaIdx >= 0) {
        fidxs.erase(std::remove_if(fidxs.begin(), fidxs.end(), 
                                  [this, key](const int& idx){ return area_idxs_[idx] != key.areaIdx; }),
                    fidxs.end() );
    }

    // // filter out params != key.param
    // if (!key.param.empty()) {
    //     fidxs.erase(std::remove_if(fidxs.begin(), fidxs.end(), 
    //                               [this, key](const int& idx){ return config_.paramName(param_uids_[idx]) != key.param; }),
    //                 fidxs.end() );
    // }

    return fidxs;
}


} // namespace area_extractor