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
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace area_extractor {

// fwd declarations
class CircleAlgorithm;
struct AreaUpdateContext;

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

    // update the area at runtime (collective over MPI ranks, no-op by default)
    virtual void update(const AreaUpdateContext& /*ctx*/) const {}

    // fields needed to update the area at runtime
    virtual std::vector<std::string> requiredFields() const { return {}; }

    // true if updating the area needs the model time step (TSTEP)
    virtual bool requiresTimeStep() const { return false; }

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
 * @brief Extraction Circle (fixed)
 *
 */
class ExtractionCircle : public ExtractionArea {

public:

    ExtractionCircle(int area_id, const Circle& circle);

    ~ExtractionCircle() override;

    using ExtractionArea::isPointInside;

    const Circle& circle() const { return circle_; }

    bool isTimeDependent() const override { return false; }

    bool isPointInside(double lat, double lon, int step) const override;

protected:

    void print(std::ostream& ss) const override;

private:
    Circle circle_;

};


/**
 * @brief Extraction Circle computed at runtime by a circle algorithm
 *
 */
class AlgorithmCircle : public ExtractionArea {

public:

    AlgorithmCircle(int area_id, std::shared_ptr<CircleAlgorithm> algorithm);

    ~AlgorithmCircle() override;

    using ExtractionArea::isPointInside;

    const CircleAlgorithm& algorithm() const { return *algorithm_; }

    bool isTimeDependent() const override { return true; }

    // uses the algorithm circle at the step (no circle => not inside)
    bool isPointInside(double lat, double lon, int step) const override;

    void update(const AreaUpdateContext& ctx) const override;

    std::vector<std::string> requiredFields() const override;

    bool requiresTimeStep() const override;

protected:

    void print(std::ostream& ss) const override;

private:
    // the area is (shallow) const, the algorithm holds the mutable state
    std::shared_ptr<CircleAlgorithm> algorithm_;

};

} // namespace area_extractor
