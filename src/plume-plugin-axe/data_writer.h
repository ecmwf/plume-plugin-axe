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

#include <mutex>
#include "plugin_types.h"
#include "extracted_data.h"

#include "eckit/config/LocalConfiguration.h"


namespace area_extractor {


class DataWriter {
public:
    DataWriter(const eckit::Configuration& config) : config_{config} {};
    virtual ~DataWriter(){};
    virtual void writeData(const std::string& filename, const ExtractedData& data) const = 0;
    constexpr static const char* type() { return "DataWriter"; }
protected:
    eckit::LocalConfiguration config_;
};


// fwd declaration
class DataWriterBuilderBase;


// factory (registers/deregisters builders and calls "build")
class DataWriterFactory {

public:  // methods
    static DataWriterFactory& instance();

    void enregister(const std::string& name, const DataWriterBuilderBase& builder);
    void deregister(const std::string& name);
    std::vector<std::string> list_registered();

    DataWriter* build(const std::string& name, const eckit::Configuration& config = eckit::LocalConfiguration{}) const;

private:  // methods
    // Only one instance can be built, inside instance()
    DataWriterFactory();
    ~DataWriterFactory();

private:  // members
    mutable std::mutex mutex_;

    std::map<std::string, std::reference_wrapper<const DataWriterBuilderBase>> builders_;
};


// base builder
class DataWriterBuilderBase {
public:  // methods
    // Only instantiate from subclasses
    DataWriterBuilderBase(const std::string& name);
    virtual ~DataWriterBuilderBase();

    virtual DataWriter* make(const eckit::Configuration& config) const = 0;

public:  // members
    std::string name_;
};


// a concrete builder for a specific DataWriter type
template <typename T>
class DataWriterBuilder : public DataWriterBuilderBase {
public:  // methods
    // The name of the builder is taken from the type of the built object
    DataWriterBuilder() : DataWriterBuilderBase(T::type()) {}

    ~DataWriterBuilder() override {}

    DataWriter* make(const eckit::Configuration& config) const override { return new T(config); }
};





} // namespace area_extractor