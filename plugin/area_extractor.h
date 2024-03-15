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

#include <iostream>
#include <string>
#include <vector>

#include "plume/PluginCore.h"
#include "plume/Plugin.h"

#include "plugin_types.h"
#include "field_reader.h"
#include "user_request.h"


namespace area_extractor {


// ==========================================================================================
class PluginCoreAreaExtractor : public plume::PluginCore {
public:
    PluginCoreAreaExtractor(const eckit::Configuration& conf);

    ~PluginCoreAreaExtractor();

    virtual void setup() override;

    virtual void run() override;

    virtual void teardown() override;

    constexpr static const char* type() { return "AreaExtractor-plugincore"; }

private:

    // user requests
    std::vector<UserRequest> requests_;

    // field reader
    FieldsReader* reader_;

    // extracted data
    ExtractedData* data_;

};
// ==========================================================================================


// ==========================================================================================
class PluginAreaExtractor : public plume::Plugin {

public:
    PluginAreaExtractor();

    ~PluginAreaExtractor();

    static std::vector<std::string> requestedFields() {
        std::vector<std::string> vec = {
            "u",
            "v" 
        };
        return vec;
    }

    plume::Protocol negotiate() override {
        plume::Protocol protocol;
        protocol.requireAtlasVersion("0.32.0");
        protocol.requireInt("NSTEP");

        // Request the necessary Atlas fields
        for (auto& fld: requestedFields()) {
            protocol.requireAtlasField(fld);
        }

        return protocol;
    }

    // Return the static instance
    static const PluginAreaExtractor& instance();

    std::string version() const override { return "0.1.0"; }

    std::string gitsha1(unsigned int count) const override { return "undefined"; }

    virtual std::string plugincoreName() const override { return PluginCoreAreaExtractor::type(); }
};
// ==========================================================================================

}  // namespace area_extractor
