
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


    static std::string metadata_templ(int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"class\": \"od\",\n"
            +ind+"  \"stream\": \"oper\",\n"
            +ind+"  \"levtype\": \"pl\",\n"
            +ind+"  \"date\": \"20170101\",\n"
            +ind+"  \"step\": \"0\",\n"
            +ind+"  \"number\": \"0\"\n"
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


    static std::string coverage_templ(const std::string& metadata, const std::string& domain, const std::string& ranges, int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"area:metadata\": "+metadata+",\n"
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


    static std::string parameter_definitions(int& indent) { 
        std::string ind(indent, ' ');
        return std::string(""
            +ind+"{\n"
            +ind+"  \"u\" : {\n"
            +ind+"    \"type\": \"Parameter\",\n"
            +ind+"    \"description\": \"u-component of wind speed\",\n"
            +ind+"    \"unit\": {\"symbol\": \"m/s\"},\n"
            +ind+"    \"observedProperty\": {\"id\": \"v\", \"label\": {\"en\": \"u-component of wind speed\"}}\n"
            +ind+"  },\n"
            +ind+"  \"v\" : {\n"
            +ind+"    \"type\": \"Parameter\",\n"
            +ind+"    \"description\": \"v-component of wind speed\",\n"
            +ind+"    \"unit\": {\"symbol\": \"m/s\"},\n"
            +ind+"    \"observedProperty\": {\"id\": \"v\", \"label\": {\"en\": \"v-component of wind speed\"}}\n"
            +ind+"  }"
            +ind+"}"
        );
    }

};

} // namespace area_extractor