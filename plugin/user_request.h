#pragma once

#include <vector>
#include <string>

#include "eckit/log/Log.h"
#include "eckit/config/LocalConfiguration.h"

#include "area.h"


namespace area_extractor {

class UserRequest {

public:

    UserRequest(const eckit::Configuration& config);
    ~UserRequest();

    const std::string& user() const { return user_; }
    const std::vector<ExtractionArea>& areas() const {return areas_;}

    friend std::ostream& operator<<(std::ostream& ss, const UserRequest& obj);

private:

    void add_area(double north, double south, double east, double west);

private:

    std::string user_;
    std::vector<ExtractionArea> areas_;
};

} // namespace area_extractor