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

#include <algorithm>
#include <cmath>
#include <sstream>

#include "eckit/exception/Exceptions.h"
#include "eckit/log/Log.h"
#include "eckit/mpi/Comm.h"

#include "atlas/array.h"

#include "plugin_types.h"
#include "geo_utils.h"
#include "circle_algorithm.h"


namespace area_extractor {

namespace {

// field name of a param interpolated at a height (e.g. "u;hl;5000"), formatted
// as Plume does for the "height" key (integral heights without decimals)
std::string heightFieldName(const std::string& param, double height) {
    std::stringstream ss;
    ss << param << ";hl;";
    if (height == std::floor(height)) {
        ss << static_cast<long long>(height);
    } else {
        ss << height;
    }
    return ss.str();
}

const plume::data::FieldView& findField(const AreaUpdateContext& ctx, const std::string& name) {
    auto it = ctx.fields.find(name);
    if (it == ctx.fields.end()) {
        throw eckit::UserError("CircleAlgorithm: required field '" + name + "' not available", Here());
    }
    return it->second;
}

} // namespace


// ---------------------- CircleAlgorithm ----------------------
CircleAlgorithm::~CircleAlgorithm() {
}


Circle advect(const Circle& circle, double u, double v, double dt) {

    const double radiusM = EARTH_RADIUS_KM * 1000.0;

    // displacement [deg] (dlon evaluated at the current latitude)
    double dlat = v * dt / radiusM * RAD_TO_DEG;
    double dlon = u * dt / (radiusM * std::cos(circle.lat * DEG_TO_RAD)) * RAD_TO_DEG;

    double lat = circle.lat + dlat;
    if (lat > 90.0 || lat < -90.0) {
        eckit::Log::warning() << "advect: circle latitude " << lat << " clamped to [-90, 90]" << std::endl;
        lat = std::max(-90.0, std::min(90.0, lat));
    }

    double lon = wrapLon(circle.lon + dlon);

    return Circle{lat, lon, circle.radius};
}


// ---------------------- WindAdvectionAlgorithm ----------------------
static CircleAlgorithmBuilder<WindAdvectionAlgorithm> windAdvectionAlgorithmBuilder;


WindAdvectionAlgorithm::WindAdvectionAlgorithm(const eckit::Configuration& config) {

    std::string prefix = std::string("WindAdvectionAlgorithm: ");

    for (const auto& key : {"lat", "lon", "radius"}) {
        if (!config.has(key)) {
            throw eckit::UserError(prefix + "missing key '" + key + "'", Here());
        }
    }

    circle_ = Circle{config.getDouble("lat"), config.getDouble("lon"), config.getDouble("radius")};
    checkCircle(prefix, circle_.lat, circle_.radius);

    windHeight_ = config.getDouble("wind_height", 5000.0);
    if (!(windHeight_ > 0.0)) {
        throw eckit::UserError(prefix + "'wind_height' must be > 0 [m]", Here());
    }

    uName_ = heightFieldName("u", windHeight_);
    vName_ = heightFieldName("v", windHeight_);
}


WindAdvectionAlgorithm::~WindAdvectionAlgorithm() {
}


void WindAdvectionAlgorithm::update(const AreaUpdateContext& ctx) {

    // first update: the configured circle is the circle at this step
    if (!lastStep_) {
        lastStep_ = ctx.step;
        return;
    }

    // nothing to do if the step has not advanced
    if (ctx.step <= *lastStep_) {
        return;
    }

    if (!ctx.tstep) {
        throw eckit::SeriousBug("WindAdvectionAlgorithm::update: TSTEP not provided", Here());
    }

    auto uArray = atlas::array::make_view<const FIELD_TYPE_REAL,2>(findField(ctx, uName_));
    auto vArray = atlas::array::make_view<const FIELD_TYPE_REAL,2>(findField(ctx, vName_));

    atlas::Field lonlats = ctx.functionspace.lonlat();
    auto lonLatArray = atlas::array::make_view<const double,2>(lonlats);

    atlas::Field ghost = ctx.functionspace.ghost();
    auto ghostArray = atlas::array::make_view<int,1>(ghost);

    // local sums of u, v and number of points inside the current circle
    double sums[3] = {0.0, 0.0, 0.0};
    for (int iPt = 0; iPt < lonLatArray.shape(0); iPt++) {

        if (ghostArray(iPt)) {
            continue;
        }

        double pointLon = lonLatArray(iPt, 0);
        double pointLat = lonLatArray(iPt, 1);

        // cheap latitude-band pre-filter
        if (std::abs(pointLat - circle_.lat) * KM_PER_DEG_LAT > circle_.radius) {
            continue;
        }

        if (greatCircleDistanceKm(pointLat, pointLon, circle_.lat, circle_.lon) <= circle_.radius) {
            sums[0] += uArray(iPt, 0);
            sums[1] += vArray(iPt, 0);
            sums[2] += 1.0;
        }
    }

    // global sums (collective)
    eckit::mpi::comm().allReduceInPlace(sums, 3, eckit::mpi::sum());

    if (sums[2] == 0.0) {
        eckit::Log::warning() << "WindAdvectionAlgorithm: no grid points inside the circle at step "
                              << ctx.step << " => circle not moved" << std::endl;
    } else {
        double uMean = sums[0] / sums[2];
        double vMean = sums[1] / sums[2];
        double dt = *ctx.tstep * (ctx.step - *lastStep_);
        circle_ = advect(circle_, uMean, vMean, dt);
    }

    lastStep_ = ctx.step;

    if (eckit::mpi::comm().rank() == 0) {
        eckit::Log::info() << "WindAdvectionAlgorithm: step " << ctx.step
                           << " circle centre (lat:" << circle_.lat << ", lon:" << circle_.lon << ")" << std::endl;
    }
}


void WindAdvectionAlgorithm::print(std::ostream& ss) const {
    ss << type() << ":(lat:" << circle_.lat << ",";
    ss << "lon:" << circle_.lon << ",";
    ss << "radius:" << circle_.radius << ",";
    ss << "wind:" << uName_ << "/" << vName_ << ")";
}


// ---------------------- CircleAlgorithmFactory ----------------------
CircleAlgorithmFactory::CircleAlgorithmFactory() {}

CircleAlgorithmFactory::~CircleAlgorithmFactory() {}

CircleAlgorithmFactory& CircleAlgorithmFactory::instance() {
    static CircleAlgorithmFactory theinstance;
    return theinstance;
}

void CircleAlgorithmFactory::enregister(const std::string& name, const CircleAlgorithmBuilderBase& builder) {
    std::lock_guard<std::mutex> lock(mutex_);
    ASSERT(builders_.find(name) == builders_.end());
    builders_.emplace(std::make_pair(name, std::ref(builder)));
}

void CircleAlgorithmFactory::deregister(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = builders_.find(name);
    ASSERT(it != builders_.end());
    builders_.erase(it);
}

std::vector<std::string> CircleAlgorithmFactory::list_registered() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> reg_;
    for (const auto& b : builders_) {
        reg_.push_back(b.first);
    }
    return reg_;
}

bool CircleAlgorithmFactory::has(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return builders_.find(name) != builders_.end();
}

std::shared_ptr<CircleAlgorithm> CircleAlgorithmFactory::build(const std::string& name, const eckit::Configuration& config) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = builders_.find(name);
    if (it == builders_.end()) {
        throw eckit::UserError("Unknown circle algorithm '" + name + "'", Here());
    }

    return std::shared_ptr<CircleAlgorithm>(it->second.get().make(config));
}

CircleAlgorithmBuilderBase::CircleAlgorithmBuilderBase(const std::string& name) : name_(name) {
    CircleAlgorithmFactory::instance().enregister(name, *this);
}

CircleAlgorithmBuilderBase::~CircleAlgorithmBuilderBase() {
    CircleAlgorithmFactory::instance().deregister(name_);
}

} // namespace area_extractor
