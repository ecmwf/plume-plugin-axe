#include <fstream>
#include <sstream>
#include <numeric>

#include "eckit/log/Log.h"
#include "eckit/mpi/Comm.h"
#include "eckit/exception/Exceptions.h"

#include "data_writer.h"

#include "plugin_utils.h"

namespace area_extractor {



void DataWriterCSV::writeData(const std::string& filename) const {

    // Write only if it owns values
    if (!data_.isEmpty()) {

        std::ofstream outfile;

        try {
            outfile.open(filename);

            // header 
            outfile << "user,area_idx,point_idx,lat,lon,lev,param,value" << std::endl;

            // values
            for (int iPt=0; iPt<data_.size(); iPt++) {
                outfile << data_.get_user(iPt) << ",";
                outfile << data_.get_area_idx(iPt) << ",";
                outfile << data_.get_idx(iPt) << ",";
                outfile << data_.get_lat(iPt) << ",";
                outfile << data_.get_lon(iPt) << ",";
                outfile << data_.get_lev(iPt) << ",";
                outfile << data_.get_param(iPt) << ",";
                outfile << data_.get_value(iPt);
                outfile << std::endl;
            }

            outfile.close();
        } catch (std::exception& e) {
            eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
        }
    }
}




// ==============================================================================================
DataWriterCOVJSON::DataWriterCOVJSON(const ExtractedData& data) : 
    DataWriter(data) {
}

void DataWriterCOVJSON::writeData(const std::string& filename) const {

    // // Write only if it owns values
    // if (!data_.isEmpty()) {

    //     std::ofstream outfile;

    //     try {
    //         outfile.open(filename);
    //         // outfile << "..." << std::endl;
    //         outfile.close();
    //     } catch (std::exception& e) {
    //         eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
    //     }
    // }

}
// ==============================================================================================





} // namespace area_extractor
