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
#include <numeric>
#include <iomanip>
#include <memory>

#include "data_writer_json.h"
#include "covjson_template.h"


namespace area_extractor {


// Static instantiation of DataWriterCOVJSON builder
static DataWriterBuilder<DataWriterCOVJSON> writerJSON;


DataWriterCOVJSON::DataWriterCOVJSON(const eckit::Configuration& config): DataWriter(config) {}

void DataWriterCOVJSON::writeData(const std::string& filename, const ExtractedData& data) const {


    // Gather data in root rank
    std::unique_ptr<ExtractedData> globalData(data.gather(0));


    int indent=0;

    // Write only if it owns values
    if (!globalData->isEmpty()) {
        
        size_t i_request = 0;
        for (const auto& request : globalData->config().requests()) {

            std::string user = request.user();
            std::string tag = request.tag();
            std::vector<ExtractionArea> areas = request.areas();

            std::vector<int> userAreaIdxs(areas.size());
            std::transform(areas.begin(), areas.end(), userAreaIdxs.begin(), [](const ExtractionArea& elem){return elem.id();});

            std::vector<int> user_idxs = globalData->find({.user=user});
            ExtractedData userData = globalData->slice(user_idxs);
            
            std::string covjson = assembleCOVJSON(user, userAreaIdxs, userData, indent);

            // compose the filename
            std::stringstream ss;
            ss << filename 
               << "_user-" << user
               << "_tag-" << tag
               << "_req-" << i_request << ".json";
            std::string filename_covjson{ss.str()};

            try {
                std::ofstream outfile;
                outfile.open(filename_covjson);
                outfile << covjson << std::endl;
                outfile.close();
            } catch (std::exception& e) {
                eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
            }

            i_request++;

        }
    }
}


std::string DataWriterCOVJSON::assembleCOVJSON(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const {

    // assemble coverages
    std::string coverages = assembleCoverages(user, areas, data, indent);

    // CRS
    std::string referencing = TemplatesCOVJSON::crs_templ(indent);

    // parameter definitions    
    std::string parameters = TemplatesCOVJSON::parameter_definitions(indent, data.config().parameters());

    // main template
    std::string json = TemplatesCOVJSON::covjson_template(coverages, referencing, parameters, indent);

    return json;

}

std::string DataWriterCOVJSON::assembleCoverages(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const {

    // find areas in the data
    std::stringstream coverages_ss;

    // prepare domain
    std::string tvalue = "\"\""; // (date, e.g: 2017-01-01T00:00:00) 

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
        std::string coverage = TemplatesCOVJSON::coverage_templ(domain, ranges_ss.str(), indent);

        coverages_ss << coverage;
        if (iarea != areas.size()-1) coverages_ss << ",\n";
        
    }

    return coverages_ss.str();
    
}


} // namespace area_extractor