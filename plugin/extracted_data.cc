
#include "eckit/log/Log.h"
#include "eckit/mpi/Comm.h"

#include "plugin_utils.h"
#include "extracted_data.h"


namespace area_extractor {


// ---------------------- data ----------------------
ExtractedData::ExtractedData(const PluginConfig& config) : config_{config} {

}


ExtractedData::~ExtractedData(){

}

void ExtractedData::addPoint(int user,
                             int area_idx,
                             int point_idx,
                             double lat,
                             double lon,
                             int lev,
                             int param,
                             FIELD_TYPE_REAL value){

    user_uids_.push_back(user);
    area_idxs_.push_back(area_idx);
    point_idxs_.push_back(point_idx);
    lats_.push_back(lat);
    lons_.push_back(lon);
    levs_.push_back(lev);
    param_uids_.push_back(param);
    values_.push_back(value);

}



// collect data in a rank
ExtractedData ExtractedData::gather(int collectRank) {

    int data_size_ = size();
    int data_size_global_;

    // global values (only meaningful at root rank)
    std::vector<int> user_uids_glob_;
    std::vector<int> area_idxs_glob_;
    std::vector<int> idxs_glob_;
    std::vector<double> lats_glob_;
    std::vector<double> lons_glob_;
    std::vector<int> levs_glob_;
    std::vector<int> param_uids_glob_;
    std::vector<FIELD_TYPE_REAL> values_glob_;

    // total number of samples for each rank
    std::vector<int> nRankSamplesVec_;
    std::vector<int> SamplesDisplVec_;

    // global data to fill-in
    ExtractedData globalData{config_};

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
        param_uids_glob_.resize(data_size_global_);
        values_glob_.resize(data_size_global_);
    }

    // 2) collect all local vectors
    // void gatherv(const std::vector<T>& send, std::vector<T>& recv, const std::vector<int>& recvcounts, const std::vector<int>& displs, size_t root) const;
    eckit::mpi::comm().gatherv(user_uids_, user_uids_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(area_idxs_, area_idxs_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(point_idxs_, idxs_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(lats_, lats_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(lons_, lons_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(levs_, levs_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(param_uids_, param_uids_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );
    eckit::mpi::comm().gatherv(values_, values_glob_, nRankSamplesVec_, SamplesDisplVec_, collectRank );

    // ---------- write gathered data ----------
    globalData.user_uids_ = user_uids_glob_;
    globalData.area_idxs_ = area_idxs_glob_;
    globalData.point_idxs_ = idxs_glob_;
    globalData.lats_ = lats_glob_;
    globalData.lons_ = lons_glob_;
    globalData.levs_ = levs_glob_;
    globalData.param_uids_ = param_uids_glob_;
    globalData.values_ = values_glob_;

    return globalData;

}



} // namespace area_extractor