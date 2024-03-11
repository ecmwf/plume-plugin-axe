#pragma once

#include <vector>
#include <string>

#include "area.h"


namespace area_extractor {

class UserRequest {

public:

    UserRequest(std::string user, std::string s3_url) : user_{user}, s3_url_{s3_url} {}
    ~UserRequest(){};

    const std::string& user() const { return user_; }
    const std::string& s3_url() const { return s3_url_; }

    void add_area(double north, double south, double east, double west) {
        int area_idx = areas_.size();
        areas_.push_back(ExtractionArea{area_idx, north, south, east, west});
    }

    const std::vector<ExtractionArea>& areas() const {return areas_;}

    friend std::ostream& operator<<(std::ostream& ss, const UserRequest& obj);

private:

    std::string user_;
    std::string s3_url_;
    std::vector<ExtractionArea> areas_;
};

} // namespace area_extractor