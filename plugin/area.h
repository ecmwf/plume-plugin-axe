#pragma once

#include <iostream>

namespace area_extractor {

/**
 * @brief Extraction Area
 * 
 */
class ExtractionArea {

public:

    ExtractionArea(int area_id,
                   double north,
                   double south,
                   double east,
                   double west);

    ~ExtractionArea();

    int id() const { 
        return area_id_;
    }
    
    double north() const { 
        return north_;
    }

    double south() const { 
        return south_;
    }

    double east() const { 
        return east_;
    }

    double west() const { 
        return west_;
    }


    bool isPointInside(double lat, double lon) const {
        return ((lat <= north_) && (lat >= south_) && (lon <= east_) && (lon >= west_));
    }

    friend std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj);

private:
    int area_id_;
    double north_;
    double south_;
    double east_;
    double west_;

};

} // namespace area_extractor