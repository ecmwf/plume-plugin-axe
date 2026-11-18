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

#include <fstream>
#include <sstream>
#include <iomanip>


#include "data_writer_csv.h"


namespace area_extractor {


// number of decimals of the param values in the output
static constexpr int VALUE_DECIMALS = 6;

// Static instantiation of DataWriterCSV builder
static DataWriterBuilder<DataWriterCSV> writerCSV;


DataWriterCSV::DataWriterCSV(const eckit::Configuration& config) : DataWriter(config) {

}


void DataWriterCSV::writeData(const std::string& filename, const ExtractedData& data) const {

    std::string filename_csv = filename + ".csv";

    // Write only if it owns values
    if (!data.isEmpty()) {

        std::ofstream outfile;

        // only the output params (sources of interpolation u, v, z are left out)
        const std::vector<std::string>& paramNames = data.config().outputParameters();
        const std::vector<size_t>& paramIdxs = data.config().outputParameterIdxs();
        size_t psize = paramIdxs.size();

        try {
            outfile.open(filename_csv);

            // header 
            outfile << "user,area_idx,point_idx,lat,lon,lev,";
            for (size_t iparam=0; iparam<psize; iparam++) {
                outfile << paramNames[iparam];
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

                // param values (fixed number of decimals, not applied to the columns above)
                std::vector<FIELD_TYPE_REAL> values = data.get_values(iPt);
                std::ostringstream values_ss;
                values_ss << std::fixed << std::setprecision(VALUE_DECIMALS);
                for (size_t iparam=0; iparam<psize; iparam++) {
                    values_ss << values[paramIdxs[iparam]];
                    if (iparam != psize-1) {
                        values_ss << ",";
                    }
                }
                outfile << values_ss.str() << std::endl;
            }

            outfile.close();
        } catch (std::exception& e) {
            eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
        }
    }
}


} // namespace area_extractor