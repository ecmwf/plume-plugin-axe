
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

#include "eckit/log/Log.h"
#include "eckit/config/LocalConfiguration.h"

#include "plugin_config.h"



namespace area_extractor {


PluginConfig::PluginConfig(const eckit::Configuration& conf, const std::vector<std::string>& params) : 
    config_{conf}, params_{params} {

    // run soem basic checks on the user config
    checkUserConfig();

    // output strategy
    outputStrategy_ = conf.getString("writer");

    // extract user requests from the configuration file
    std::vector<eckit::LocalConfiguration> requests = conf.getSubConfigurations("requests");
    

    int user_counter = 0;
    for (const auto& conf_req : requests) {

        std::string user = conf_req.getString("user");

        // fill-in use request
        requests_.push_back(UserRequest(conf_req));

        // map user->uid (NB: only one request entry per user admitted)
        if (user_to_uid_.find(user) != user_to_uid_.end()) {
            eckit::Log::warning() << "User: " << user << " has multiple requests! => only using last one in list." << std::endl;
        }
        user_to_uid_.insert(std::make_pair(user,user_counter++));
    }

    // map param->uid
    for (int p=0; p<params_.size(); p++) {
        param_to_uid_.insert(std::make_pair(params_[p], p));
    }

    // map uid->param
    for (const auto& p: param_to_uid_) {
        uid_to_param_.insert(std::make_pair(p.second, p.first));
    }

    // map uid->user
    for (const auto& p: user_to_uid_) {
        uid_to_user_.insert(std::make_pair(p.second,p.first));
    }

    // store users
    for (const auto& ui : user_to_uid_) {
        users_.push_back( ui.first );
    }

}

PluginConfig::~PluginConfig() {

}


std::string PluginConfig::userName(int uid) const {
    try {
        return uid_to_user_.at(uid);
    } catch (std::exception& e) {
        eckit::Log::warning() << "User UID:" << uid << " NOT FOUND!" << std::endl;
        eckit::Log::warning() << e.what() << std::endl;
    }
}

int PluginConfig::user_uid(const std::string& userName) const {
    try {
        return user_to_uid_.at(userName);
    } catch (std::exception& e) {
        eckit::Log::warning() << "User:" << userName << " NOT FOUND!" << std::endl;
        eckit::Log::warning() << e.what() << std::endl;
    }
}

std::string PluginConfig::paramName(int uid) const {
    try {
        return uid_to_param_.at(uid);
    } catch (std::exception& e) {
        eckit::Log::warning() << "Parameter UID:" << uid << " NOT FOUND!" << std::endl;
        eckit::Log::warning() << e.what() << std::endl;
    }
}

int PluginConfig::param_uid(const std::string& paramName) const {
    try {
        return param_to_uid_.at(paramName);
    } catch (std::exception& e) {
        eckit::Log::warning() << "Parameter:" << paramName << " NOT FOUND!" << std::endl;
        eckit::Log::warning() << e.what() << std::endl;
    }
}


std::ostream& operator<<(std::ostream& ss, const PluginConfig& obj) {
    ss << "{";
    ss << "outputStrategy:" << obj.outputStrategy_ << "," << std::endl;
    ss << "requests: [" << std::endl;
    for (const auto& req: obj.requests()) {
        ss << req << std::endl;
    }
    ss << "]," << std::endl;
    ss << "params: [";
    for(int p=0; p<obj.params_.size(); p++) {
        ss << obj.params_[p];
        if (p != obj.params_.size()-1) {
            ss << ",";
        }
    }
    ss << "]";
    ss << "}";
    return ss;
}


const std::vector<UserRequest>& PluginConfig::requests() const {
    return requests_;
}


void PluginConfig::checkUserConfig() {
    // Add configuration checks here..
}

} // namespace area_extractor