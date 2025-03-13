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
#pragma once

#include <string>


namespace area_extractor {

class TemplatesCOVJSON {

public:

    static std::string range_templ(const std::string& name, const std::string& shape, const std::string& axisNames, const std::string& values, int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"\"" + name + "\": {\n"
            +ind+"  \"type\": \"NdArray\",\n"
            +ind+"  \"dataType\": \"float\",\n"
            +ind+"  \"shape\": ["+shape+"],\n"
            +ind+"  \"axisNames\": ["+axisNames+"],\n"
            +ind+"  \"values\": ["+values+"]\n"
            +ind+"}"
        );

    }


    static std::string domain_templ(const std::string& tvalue, const std::string& values, int& indent) { 

        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"type\": \"Domain\",\n"
            +ind+"  \"axes\": {\n"
            +ind+"    \"t\": {\"values\": ["+tvalue+"]},\n"
            +ind+"    \"composite\": {\n"
            +ind+"      \"dataType\": \"tuple\",\n"
            +ind+"      \"coordinates\": [\"x\", \"y\", \"z\"],\n"
            +ind+"      \"values\": ["+values+"]\n"
            +ind+"    }\n"
            +ind+"  }\n"
            +ind+"}"
        );
    }


    static std::string coverage_templ(const std::string& domain, const std::string& ranges, int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"type\": \"Coverage\",\n"
            +ind+"  \"domain\": "+domain+",\n"
            +ind+"  \"ranges\": {"+ranges+"}\n"
            +ind+"}"
        );
    }


    static std::string crs_templ(int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"coordinates\": [\"x\", \"y\", \"z\"],\n"
            +ind+"  \"system\": {\n"
            +ind+"    \"type\": \"GeographicCRS\",\n"
            +ind+"    \"id\": \"http://www.opengis.net/def/crs/OGC/1.3/CRS84\"\n"
            +ind+"  }\n"
            +ind+"}"
        );
    }


    static std::string param_templ(int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"type\": \"Parameter\",\n"
            +ind+"  \"description\": \"Pressure\",\n"
            +ind+"  \"unit\": {\n"
            +ind+"    \"symbol\": \"\"\n"
            +ind+"  },\n"
            +ind+"  \"observedProperty\": {\n"
            +ind+"    \"id\": \"\",\n"
            +ind+"    \"label\": {\"en\": \"\"}\n"
            +ind+"  }\n"
            +ind+"}"
        );
    }


    static std::string covjson_template(const std::string& coverages, const std::string& referencing, const std::string& parameters, int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"type\": \"CoverageCollection\",\n"
            +ind+"  \"domainType\": \"MultiPoint\",\n"
            +ind+"  \"coverages\": ["+coverages+"],\n"
            +ind+"  \"referencing\": ["+referencing+"],\n"
            +ind+"  \"parameters\": "+parameters+"\n"
            +ind+"}"
        );
    }


    static std::string parameter_definitions(int& indent, const std::vector<std::string>& paramNames) { 
        std::string ind(indent+2, ' ');

        std::string params;
        for (size_t iname=0; iname<paramNames.size(); iname++){
            params += std::string(""
                                  +ind+"{\n"
                                  +ind+"\"" + paramNames[iname] + "\" : {\n"
                                  +ind+"  \"type\": \"Parameter\"\n"
                                  +ind+"}");
            // add comma if not last
            if (iname != paramNames.size()-1) params += ",\n";
          }

        return params;
    }

};

} // namespace area_extractor