

#include "eckit/log/Log.h"
#include "eckit/mpi/Comm.h"
#include "eckit/exception/Exceptions.h"

#include "plugin_utils.h"
#include "data_writer.h"


namespace area_extractor {


DataWriterFactory::DataWriterFactory() {}

DataWriterFactory::~DataWriterFactory() {}

DataWriterFactory& DataWriterFactory::instance() {
    static DataWriterFactory theinstance;
    return theinstance;
}

void DataWriterFactory::enregister(const std::string& name, const DataWriterBuilderBase& builder) {
    std::lock_guard<std::mutex> lock(mutex_);
    ASSERT(builders_.find(name) == builders_.end());
    builders_.emplace(std::make_pair(name, std::ref(builder)));
}

void DataWriterFactory::deregister(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = builders_.find(name);
    ASSERT(it != builders_.end());
    builders_.erase(it);
}

std::vector<std::string> DataWriterFactory::list_registered() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> reg_;
    for (const auto& b : builders_) {
        reg_.push_back(b.first);
    }
    return reg_;
}

DataWriter* DataWriterFactory::build(const std::string& name, const eckit::Configuration& config) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = builders_.find(name);
    if (it == builders_.end()) {
        throw eckit::SeriousBug("Builder not found for writer " + name, Here());
    }

    return it->second.get().make(config);
}

DataWriterBuilderBase::DataWriterBuilderBase(const std::string& name) : name_(name) {
    DataWriterFactory::instance().enregister(name, *this);
}

DataWriterBuilderBase::~DataWriterBuilderBase() {
    DataWriterFactory::instance().deregister(name_);
}




} // namespace area_extractor
