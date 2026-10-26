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
#include <optional>
#include <vector>

namespace area_extractor {

/**
 * @brief Circle at a given time step (radius in km)
 */
struct Circle {
    double lat;
    double lon;
    double radius;
};

/**
 * @brief Circle definition at a model step (radius in km)
 */
struct TrackPoint {
    int step;
    double lat;
    double lon;
    double radius;
};

/**
 * @brief Great-circle (haversine) distance in km between two points (degrees)
 */
double greatCircleDistanceKm(double lat1, double lon1, double lat2, double lon2);

/**
 * @brief Extraction Area (bounding box, fixed circle or time-dependent circle)
 *
 */
class ExtractionArea {

public:

    enum class Shape { BBox, Circle };
    enum class Interpolation { None, Linear };

    // bounding box
    ExtractionArea(int area_id,
                   double north,
                   double south,
                   double east,
                   double west);

    // fixed circle
    ExtractionArea(int area_id, const Circle& circle);

    // time-dependent circle (track points are sorted by step)
    ExtractionArea(int area_id, std::vector<TrackPoint> track, Interpolation interpolation);

    ~ExtractionArea();

    int id() const { return area_id_; }
    double north() const { return north_; }
    double south() const { return south_; }
    double east() const { return east_; }
    double west() const { return west_; }

    Shape shape() const { return shape_; }
    Interpolation interpolation() const { return interpolation_; }
    const std::vector<TrackPoint>& track() const { return track_; }

    // true only for circles defined through a track
    bool isTimeDependent() const { return timeDependent_; }

    // circle effective at a step (nullopt => no extraction at this step)
    std::optional<Circle> circleAt(int step) const;

    bool isPointInside(double lat, double lon, int step) const;

    // only valid for bounding boxes and fixed circles
    bool isPointInside(double lat, double lon) const;

    friend std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj);

private:
    int area_id_;
    Shape shape_{Shape::BBox};

    // bounding box
    double north_{0};
    double south_{0};
    double east_{0};
    double west_{0};

    // circle (a fixed circle is stored as a single-point track)
    std::vector<TrackPoint> track_;
    Interpolation interpolation_{Interpolation::Linear};
    bool timeDependent_{false};

};

} // namespace area_extractor
