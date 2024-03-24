#include <fstream>
#include <sstream>
#include <numeric>
#include <iomanip>

#include "eckit/log/Log.h"
#include "eckit/mpi/Comm.h"
#include "eckit/exception/Exceptions.h"

#include "plugin_utils.h"
#include "data_writer.h"
#include "covjson_template.h"


namespace area_extractor {



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
                // outfile << data.get_param(iPt) << ",";

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




// ======================================== COVJSON =============================================
DataWriterCOVJSON::DataWriterCOVJSON(): DataWriter() {}

void DataWriterCOVJSON::writeData(const std::string& filename, const ExtractedData& data) const {

    int indent=0;

    // Write only if it owns values
    if (!data.isEmpty()) {

        for (const auto& request : data.config().requests()) {

            std::string user = request.user();
            std::vector<ExtractionArea> areas = request.areas();

            std::vector<int> userAreaIdxs(areas.size());
            std::transform(areas.begin(), areas.end(), userAreaIdxs.begin(), [](const ExtractionArea& elem){return elem.id();});

            std::vector<int> user_idxs = data.find({.user=user});
            ExtractedData userData = data.slice(user_idxs);
            
            std::string covjson = assembleCOVJSON(user, userAreaIdxs, userData, indent);


            std::stringstream ss;
            ss << filename << "-" << user << ".json";
            std::string filename_covjson{ss.str()};

            try {
                std::ofstream outfile;
                outfile.open(filename_covjson);
                outfile << covjson << std::endl;
                outfile.close();
            } catch (std::exception& e) {
                eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
            }

        }
    }
}


std::string DataWriterCOVJSON::assembleCOVJSON(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const {

    // assemble coverages
    std::string coverages = assembleCoverages(user, areas, data, indent);

    // CRS
    std::string referencing = TemplatesCOVJSON::crs_templ(indent);

    // parameter definitions    
    std::string parameters = TemplatesCOVJSON::parameter_definitions(indent);

    // main template
    std::string json = TemplatesCOVJSON::covjson_template(coverages, referencing, parameters, indent);

    return json;

}

std::string DataWriterCOVJSON::assembleCoverages(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const {

    // find areas in the data
    std::stringstream coverages_ss;

    // prepare domain
    std::string tvalue = "\"2017-01-01T00:00:00\""; /// TODO: this needs to be changed...

    // metadata /// TODO: this needs to be changed...
    std::string metadata = TemplatesCOVJSON::metadata_templ(indent);


    std::string ind_latlon(indent+16,' ');
    std::string ind_values(indent+13,' ');

    for (size_t iarea=0; iarea<areas.size(); iarea++){

        std::vector<int> user_area_idxs = data.find({.user=user, .areaIdx=areas[iarea]}); 
        ExtractedData areaData = data.slice(user_area_idxs);

        // Create the string for [ [lat,lon,lev], [lat,lon,lev], ... ]
        std::stringstream ss;        
        for (size_t iPt=0; iPt<areaData.size(); iPt++) {
            if (!iPt) ss << "\n";

            ss << ind_latlon << "[" << std::fixed << std::setprecision(3)
                             << areaData.get_lat(iPt) << "," 
                             << areaData.get_lon(iPt) << "," << std::fixed << std::setprecision(0)
                             << areaData.get_lev(iPt) << "]";

            if (iPt != areaData.size()-1) ss << "," << std::endl;
        }
        std::string latlon_str{ss.str()};

        std::string domain = TemplatesCOVJSON::domain_templ(tvalue, latlon_str, indent);

        // ranges (loop over params)
        std::stringstream ranges_ss;
        std::stringstream shape_ss;
        shape_ss << areaData.size();
        std::string shape{shape_ss.str()};

        size_t param_size = areaData.config().parameters().size();
        for (size_t iparam=0; iparam<param_size; iparam++) {

            std::string paramName = areaData.get_param(iparam);
            std::string axisNames = "\"" + paramName + "\"";

            std::stringstream values_ss;
            for (size_t ival=0; ival<areaData.get_param_values(iparam).size(); ival++) {
                if (!ival) values_ss << "\n";
                values_ss << ind_values << std::fixed << std::setprecision(3) << areaData.get_param_values(iparam)[ival];
                if (ival != areaData.size()-1) values_ss << "," << std::endl;
            }

            std::string values{values_ss.str()};
            std::string range = TemplatesCOVJSON::range_templ(paramName, shape, axisNames, values, indent);

            ranges_ss << range;
            if (iparam != param_size-1) ranges_ss << ",\n";
        }

        // assemble coverage
        std::string coverage = TemplatesCOVJSON::coverage_templ(metadata, domain, ranges_ss.str(), indent);

        coverages_ss << coverage;
        if (iarea != areas.size()-1) coverages_ss << ",\n";
        
    }

    return coverages_ss.str();
    
}
// ==============================================================================================


// ========================================= NetCDF =============================================
DataWriterNETCDF::DataWriterNETCDF() : DataWriter() {}

void DataWriterNETCDF::writeData(const std::string& filename, const ExtractedData& data) const {

}
// ==============================================================================================


} // namespace area_extractor
