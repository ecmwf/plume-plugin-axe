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
#include <memory>

#include "plume/PluginCore.h"
#include "plume/Plugin.h"

#include "plugin_config.h"
#include "plugin_types.h"
#include "data_reader.h"
#include "user_request.h"
#include "data_writer.h"

#include "atlas/field/Field.h"


namespace area_extractor {


// ==========================================================================================
class AreaExtractorCore : public plume::PluginCore {
public:
    AreaExtractorCore(const eckit::Configuration& conf);

    ~AreaExtractorCore();

    void setup() override;

    void run() override;

    void teardown() override;

    constexpr static const char* type() { return "area-extractor-core"; }

private:

    // runs every N steps
    int runsEvery_;

    // plugin configuration
    PluginConfig config_;

    // field reader
    std::unique_ptr<DataReader> reader_;

    // extracted data
    std::unique_ptr<ExtractedData> data_;

    // data writer
    std::unique_ptr<DataWriter> writer_;

    // aggregated data
    std::unique_ptr<ExtractedData> globaldata_;
};
// ==========================================================================================


// ==========================================================================================
class AreaExtractor : public plume::Plugin {

public:
    AreaExtractor();

    static std::vector<std::string> requestedFields() {
        std::vector<std::string> vec = {
            "u",
            "v",
            "t"
        };
        return vec;
    }

    plume::Protocol negotiate() override {
        plume::Protocol protocol;
        protocol.requireAtlasVersion("0.46.0");
        protocol.requirePlumeVersion("0.5.0");
        protocol.require<int>("NSTEP");

        // Request the necessary Atlas fields
        for (auto& fld: requestedFields()) {
            protocol.require<atlas::Field>(fld);
        }

        return protocol;
    }

    // Return the static instance
    static const AreaExtractor& instance();

    std::string version() const override { return "0.1.0"; }

    std::string gitsha1(unsigned int count) const override { return "undefined"; }

    virtual std::string plugincoreName() const override { return AreaExtractorCore::type(); }
};
// ==========================================================================================

}  // namespace area_extractor
