#include <fstream>
#include <sstream>

#include "eckit/log/Log.h"
#include "eckit/exception/Exceptions.h"

#include "data_writer.h"



namespace area_extractor {



void DataWriterCSV::write(const std::string& filename, const ExtractedData& data) const {

    // Write only if it owns values
    if (!data.isEmpty()) {

        std::ofstream outfile;

        try {
            outfile.open(filename);

            // header 
            outfile << "user,area_idx,point_idx,lat,lon,lev,param,value" << std::endl;

            // values
            for (int iPt=0; iPt<data.size(); iPt++) {
                outfile << data.get_user(iPt) << ",";
                outfile << data.get_area_idx(iPt) << ",";
                outfile << data.get_idx(iPt) << ",";
                outfile << data.get_lat(iPt) << ",";
                outfile << data.get_lon(iPt) << ",";
                outfile << data.get_lev(iPt) << ",";
                outfile << data.get_param(iPt) << ",";
                outfile << data.get_value(iPt);
                outfile << std::endl;
            }

            outfile.close();
        } catch (std::exception& e) {
            eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
        }
    }
}


} // namespace area_extractor
