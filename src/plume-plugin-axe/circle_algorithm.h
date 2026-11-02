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
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"

#include "atlas/functionspace/FunctionSpace.h"

#include "plume/data/FieldAccess.h"

#include "area.h"


namespace area_extractor {

/**
 * @brief Runtime data an algorithm may use to update its circle
 */
struct AreaUpdateContext {
    // model step (NSTEP)
    int step;

    // model time step [s] (TSTEP), only set if required by some area
    std::optional<double> tstep;

    // available fields, by name
    const std::map<std::string, plume::data::FieldView>& fields;

    // function space of the fields (lonlat + ghost)
    atlas::FunctionSpace functionspace;
};


/**
 * @brief Algorithm that computes the circle of an extraction area at runtime (abstract)
 *
 */
class CircleAlgorithm {

public:

    virtual ~CircleAlgorithm();

    // circle at a step (nullopt => no extraction at this step)
    virtual std::optional<Circle> circleAt(int step) const = 0;

    // update the circle at the context step (collective over MPI ranks)
    virtual void update(const AreaUpdateContext& ctx) = 0;

    // fields needed by the algorithm
    virtual std::vector<std::string> requiredFields() const { return {}; }

    // true if the algorithm needs the model time step (TSTEP)
    virtual bool requiresTimeStep() const { return false; }

    friend std::ostream& operator<<(std::ostream& ss, const CircleAlgorithm& obj) {
        obj.print(ss);
        return ss;
    }

protected:

    virtual void print(std::ostream& ss) const = 0;
};


/**
 * @brief Move a circle centre by (u*dt, v*dt) metres east/north
 *        (longitude wrapped to (-180, 180], latitude clamped to [-90, 90])
 */
Circle advect(const Circle& circle, double u, double v, double dt);


/**
 * @brief Moves the circle centre with the mean wind (u, v at a given height) inside the circle
 *
 */
class WindAdvectionAlgorithm : public CircleAlgorithm {

public:

    WindAdvectionAlgorithm(const eckit::Configuration& config);

    ~WindAdvectionAlgorithm() override;

    constexpr static const char* type() { return "wind-advected"; }

    // current circle (as of the last update)
    const Circle& circle() const { return circle_; }

    // the current circle, at any step
    std::optional<Circle> circleAt(int /*step*/) const override { return circle_; }

    void update(const AreaUpdateContext& ctx) override;

    std::vector<std::string> requiredFields() const override { return {uName_, vName_}; }

    bool requiresTimeStep() const override { return true; }

protected:

    void print(std::ostream& ss) const override;

private:

    Circle circle_;

    double windHeight_;

    std::string uName_;
    std::string vName_;

    // step of the last update (unset before the first update)
    std::optional<int> lastStep_;
};


/**
 * @brief Circle following a track of model steps (centre and radius given per step)
 *
 */
class TrackAlgorithm : public CircleAlgorithm {

public:

    enum class Interpolation { None, Linear };

    TrackAlgorithm(const eckit::Configuration& config);

    // track points are sorted by step
    TrackAlgorithm(std::vector<TrackPoint> track, Interpolation interpolation);

    ~TrackAlgorithm() override;

    constexpr static const char* type() { return "track"; }

    Interpolation interpolation() const { return interpolation_; }
    const std::vector<TrackPoint>& track() const { return track_; }

    // circle at a step (nullopt outside the track range, or between points without interpolation)
    std::optional<Circle> circleAt(int step) const override;

    // the track does not depend on the model data
    void update(const AreaUpdateContext& /*ctx*/) override {}

protected:

    void print(std::ostream& ss) const override;

private:

    void sortTrack();

private:

    std::vector<TrackPoint> track_;
    Interpolation interpolation_{Interpolation::Linear};
};


// fwd declaration
class CircleAlgorithmBuilderBase;


// factory (registers/deregisters builders and calls "build")
class CircleAlgorithmFactory {

public:  // methods
    static CircleAlgorithmFactory& instance();

    void enregister(const std::string& name, const CircleAlgorithmBuilderBase& builder);
    void deregister(const std::string& name);
    std::vector<std::string> list_registered();

    bool has(const std::string& name) const;

    std::shared_ptr<CircleAlgorithm> build(const std::string& name, const eckit::Configuration& config) const;

private:  // methods
    // Only one instance can be built, inside instance()
    CircleAlgorithmFactory();
    ~CircleAlgorithmFactory();

private:  // members
    mutable std::mutex mutex_;

    std::map<std::string, std::reference_wrapper<const CircleAlgorithmBuilderBase>> builders_;
};


// base builder
class CircleAlgorithmBuilderBase {
public:  // methods
    // Only instantiate from subclasses
    CircleAlgorithmBuilderBase(const std::string& name);
    virtual ~CircleAlgorithmBuilderBase();

    virtual CircleAlgorithm* make(const eckit::Configuration& config) const = 0;

public:  // members
    std::string name_;
};


// a concrete builder for a specific CircleAlgorithm type
template <typename T>
class CircleAlgorithmBuilder : public CircleAlgorithmBuilderBase {
public:  // methods
    // The name of the builder is taken from the type of the built object
    CircleAlgorithmBuilder() : CircleAlgorithmBuilderBase(T::type()) {}

    ~CircleAlgorithmBuilder() override {}

    CircleAlgorithm* make(const eckit::Configuration& config) const override { return new T(config); }
};

} // namespace area_extractor
