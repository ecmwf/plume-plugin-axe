
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "eckit/log/Log.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/config/Configuration.h"
#include "eckit/config/LocalConfiguration.h"

#include "user_request.h"


namespace area_extractor {

class PluginConfig {

public:

    PluginConfig(const eckit::Configuration& conf, const std::vector<std::string>& params);
    ~PluginConfig();

    const std::vector<UserRequest>& requests() const;

    friend std::ostream& operator<<(std::ostream& ss, const PluginConfig& obj);

    std::string userName(int uid) const;
    int user_uid(const std::string& userName) const;

    std::string paramName(int uid) const;
    int param_uid(const std::string& paramName) const;

private:

    void checkUserConfig();

private:

    // the underlying eckit config
    eckit::LocalConfiguration config_;

    // parameters
    std::vector<std::string> params_;

    // output strategy
    std::string outputStrategy_; // [ process_csv | covjson ]

    // user requests
    std::vector<UserRequest> requests_;

    // user to user UID
    std::map<std::string,int> user_to_uid_;
    std::map<int,std::string> uid_to_user_;

    // parameters requested by the plugin
    std::map<std::string,int> param_to_uid_;
    std::map<int,std::string> uid_to_param_;

};

} // namespace area_extractor