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
 * @brief Extraction Area (abstract)
 *
 */
class ExtractionArea {

public:

    explicit ExtractionArea(int area_id);

    virtual ~ExtractionArea();

    int id() const { return area_id_; }

    // true if the area changes with the model step
    virtual bool isTimeDependent() const = 0;

    // true if the point is inside the area at the given model step
    virtual bool isPointInside(double lat, double lon, int step) const = 0;

    // only valid for areas that do not depend on the model step
    bool isPointInside(double lat, double lon) const;

    friend std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj);

protected:

    virtual void print(std::ostream& ss) const = 0;

private:
    int area_id_;

};


/**
 * @brief Extraction Box (lat/lon bounding box)
 *
 */
class ExtractionBox : public ExtractionArea {

public:

    ExtractionBox(int area_id,
                  double north,
                  double south,
                  double east,
                  double west);

    ~ExtractionBox() override;

    using ExtractionArea::isPointInside;

    double north() const { return north_; }
    double south() const { return south_; }
    double east() const { return east_; }
    double west() const { return west_; }

    bool isTimeDependent() const override { return false; }

    bool isPointInside(double lat, double lon, int step) const override;

protected:

    void print(std::ostream& ss) const override;

private:
    double north_;
    double south_;
    double east_;
    double west_;

};


/**
 * @brief Extraction Circle (fixed, or following a track of model steps)
 *
 */
class ExtractionCircle : public ExtractionArea {

public:

    enum class Interpolation { None, Linear };

    // fixed circle
    ExtractionCircle(int area_id, const Circle& circle);

    // time-dependent circle (track points are sorted by step)
    ExtractionCircle(int area_id, std::vector<TrackPoint> track, Interpolation interpolation);

    ~ExtractionCircle() override;

    using ExtractionArea::isPointInside;

    Interpolation interpolation() const { return interpolation_; }
    const std::vector<TrackPoint>& track() const { return track_; }

    // true only for circles defined through a track
    bool isTimeDependent() const override { return timeDependent_; }

    // circle effective at a step (nullopt => no extraction at this step)
    std::optional<Circle> circleAt(int step) const;

    bool isPointInside(double lat, double lon, int step) const override;

protected:

    void print(std::ostream& ss) const override;

private:
    // a fixed circle is stored as a single-point track
    std::vector<TrackPoint> track_;
    Interpolation interpolation_{Interpolation::Linear};
    bool timeDependent_{false};

};

} // namespace area_extractor
