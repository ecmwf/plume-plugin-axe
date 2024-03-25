#include <fstream>
#include <sstream>


#include "data_writer_csv.h"


namespace area_extractor {


// Static instantiation of DataWriterCSV builder
static DataWriterBuilder<DataWriterCSV> writerCSV;


DataWriterCSV::DataWriterCSV(const eckit::Configuration& config) : DataWriter(config) {

}


void DataWriterCSV::writeData(const std::string& filename, const ExtractedData& data) const {

    std::string filename_csv = filename + ".csv";

    // Write only if it owns values
    if (!data.isEmpty()) {

        std::ofstream outfile;
        size_t psize = data.config().parameters().size();

        try {
            outfile.open(filename_csv);

            // header 
            outfile << "user,area_idx,point_idx,lat,lon,lev,";
            for (size_t iparam=0; iparam<psize; iparam++) {
                outfile << data.config().parameters()[iparam];
                if (iparam != psize-1) {
                    outfile << ",";
                }
            }
            outfile << std::endl;

            // values
            for (int iPt=0; iPt<data.size(); iPt++) {
                outfile << data.get_user(iPt) << ",";
                outfile << data.get_area_idx(iPt) << ",";
                outfile << data.get_idx(iPt) << ",";
                outfile << data.get_lat(iPt) << ",";
                outfile << data.get_lon(iPt) << ",";
                outfile << data.get_lev(iPt) << ",";

                for (size_t iparam=0; iparam<psize; iparam++) {
                    outfile << data.get_values(iPt)[iparam];
                    if (iparam != psize-1) {
                        outfile << ",";
                    }
                }
                outfile << std::endl;
            }

            outfile.close();
        } catch (std::exception& e) {
            eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
        }
    }
}


} // namespace area_extractor