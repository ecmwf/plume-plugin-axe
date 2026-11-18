#!/usr/bin/env python
#
#  (C) Copyright 2023- ECMWF.
#
#  This software is licensed under the terms of the Apache Licence Version 2.0
#  which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#
#  In applying this licence, ECMWF does not waive the privileges and immunities
#  granted to it by virtue of its status as an intergovernmental organisation nor
#  does it submit to any jurisdiction.
#

"""
Visualise data extracted by the AXE plugin along a prescribed track ("track" circle algorithm).

Plot types:
  map PARAM     animated GIF with one two-panel frame per output step (geographic map + storm-relative
                zoom); the frame images are kept only with --keep-images
  time          one multi-panel figure of storm-relative summary quantities against valid time
  radius PARAM  azimuthal mean of PARAM against distance from the track centre and valid time

Examples:
  plot_track_extraction.py map ws5000 --config plume-config.json --data-dir plume_plugins_output-xxxx-20261004-00 \\
      --skip-frames 8
  plot_track_extraction.py time --config plume-config.json --data-dir plume_plugins_output-xxxx-20261004-00
  plot_track_extraction.py radius sp --config plume-config.json --data-dir plume_plugins_output-xxxx-20261004-00
"""

import argparse
import datetime as dt
import glob
import json
import math
import os
import re
import sys
import tempfile
from dataclasses import dataclass
from typing import Callable, Dict, List, Optional, Tuple

import numpy as np

import matplotlib
matplotlib.use("Agg")
import matplotlib.dates as mdates          # noqa: E402
import matplotlib.pyplot as plt            # noqa: E402
import matplotlib.tri as mtri              # noqa: E402
from matplotlib.patches import Circle as CirclePatch  # noqa: E402


# same constants as the plugin (geo_utils.h)
EARTH_RADIUS_KM = 6371.229
DEG_TO_RAD = math.pi / 180.0
KM_PER_DEG_LAT = EARTH_RADIUS_KM * DEG_TO_RAD

DEFAULT_TSTEP = 200.0
DEFAULT_FPS = 10.0
DEFAULT_BIN_WIDTH = 25.0
DEFAULT_PERCENTILES = (1.0, 99.0)

# height-interpolated fields are named "<param>;hl;<height>" (e.g. "u;hl;5000")
INTERP_RE = re.compile(r"^(?P<base>[^;]+);hl;(?P<height>[0-9.]+)$")
# source fields of the interpolation, written at level 0 (not meaningful) when interpolated fields exist
INTERP_SOURCES = ("u", "v", "z")


# ------------------------------------------------------------------ errors

class TrackPlotError(Exception):
    """User-facing error (bad input/configuration): reported without traceback."""


def warn(msg: str) -> None:
    print(f"warning: {msg}", file=sys.stderr)


def info(msg: str) -> None:
    print(msg, flush=True)


# ------------------------------------------------------------------ data types

@dataclass
class Track:
    user: str
    tag: str
    interpolation: str           # "linear" or "none"
    steps: np.ndarray            # sorted
    lat: np.ndarray
    lon: np.ndarray
    radius: np.ndarray           # [km]

    @property
    def max_radius(self) -> float:
        return float(np.max(self.radius))


@dataclass
class Frame:
    step: int
    lat: np.ndarray
    lon: np.ndarray              # wrapped to (-180, 180]
    fields: Dict[str, np.ndarray]


@dataclass
class ParamSpec:
    name: str                    # name as in the files (or derived name, e.g. "ws;hl;5000")
    safe: str                    # shell/file-friendly name (alias for height-interpolated names)
    label: str
    units: str
    cmap: str
    sources: List[str]
    compute: Callable[[Dict[str, np.ndarray]], np.ndarray]

    def title(self) -> str:
        return f"{self.label} [{self.units}]" if self.units else self.label


# ------------------------------------------------------------------ geometry (same as the plugin)

def wrap_lon_diff(dlon):
    """Wrap a longitude difference to (-180, 180] (geo_utils.h: wrapLonDiff)."""
    dlon = np.fmod(dlon, 360.0)
    dlon = np.where(dlon > 180.0, dlon - 360.0, dlon)
    dlon = np.where(dlon <= -180.0, dlon + 360.0, dlon)
    return dlon


def track_centre(track: Track, step: int) -> Optional[Tuple[float, float, float]]:
    """Circle (lat, lon, radius) at a step, as TrackAlgorithm::circleAt; None outside the track."""
    steps = track.steps
    if step < steps[0] or step > steps[-1]:
        return None
    i = int(np.searchsorted(steps, step, side="right")) - 1
    if steps[i] == step:
        return float(track.lat[i]), float(wrap_lon_diff(track.lon[i])), float(track.radius[i])
    if track.interpolation == "none":
        return None
    t = (step - steps[i]) / (steps[i + 1] - steps[i])
    lat = track.lat[i] + t * (track.lat[i + 1] - track.lat[i])
    lon = track.lon[i] + t * wrap_lon_diff(track.lon[i + 1] - track.lon[i])
    radius = track.radius[i] + t * (track.radius[i + 1] - track.radius[i])
    return float(lat), float(wrap_lon_diff(lon)), float(radius)


def distance_bearing(lat0: float, lon0: float, lat, lon) -> Tuple[np.ndarray, np.ndarray]:
    """Great-circle distance [km] (area.cc: greatCircleDistanceKm) and initial bearing [rad]."""
    phi1 = lat0 * DEG_TO_RAD
    phi2 = np.asarray(lat) * DEG_TO_RAD
    dlam = wrap_lon_diff(np.asarray(lon) - lon0) * DEG_TO_RAD
    sdphi = np.sin((phi2 - phi1) / 2.0)
    sdlam = np.sin(dlam / 2.0)
    h = sdphi * sdphi + np.cos(phi1) * np.cos(phi2) * sdlam * sdlam
    dist = 2.0 * EARTH_RADIUS_KM * np.arcsin(np.sqrt(np.minimum(1.0, h)))
    bearing = np.arctan2(np.sin(dlam) * np.cos(phi2),
                         np.cos(phi1) * np.sin(phi2) - np.sin(phi1) * np.cos(phi2) * np.cos(dlam))
    return dist, bearing


def storm_relative_xy(lat0: float, lon0: float, lat, lon) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Azimuthal-equidistant coordinates [km east, km north] around (lat0, lon0), plus the distance."""
    dist, bearing = distance_bearing(lat0, lon0, lat, lon)
    return dist * np.sin(bearing), dist * np.cos(bearing), dist


# ------------------------------------------------------------------ reading

def load_track(config_path: str) -> Track:
    """Read the single "track" circle (and its request user/tag) from a Plume configuration."""
    try:
        with open(config_path) as f:
            config = json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        raise TrackPlotError(f"cannot read configuration '{config_path}': {e}")

    single_track_msg = ("only a single track is supported: the configuration must hold exactly one "
                        "AreaExtractor plugin with one request with one 'track' circle extraction")

    plugins = [p for p in config.get("plugins", []) if p.get("name") == "AreaExtractor"]
    if len(plugins) != 1:
        raise TrackPlotError(f"{single_track_msg} (found {len(plugins)} AreaExtractor plugins)")
    requests = plugins[0].get("core-config", {}).get("requests", [])
    if len(requests) != 1:
        raise TrackPlotError(f"{single_track_msg} (found {len(requests)} requests)")
    request = requests[0]
    extractions = request.get("extractions", [])
    if len(extractions) != 1:
        raise TrackPlotError(f"{single_track_msg} (found {len(extractions)} extractions)")
    extraction = extractions[0]
    algorithm = extraction.get("algorithm")
    if extraction.get("type") != "circle" or not isinstance(algorithm, dict) or algorithm.get("name") != "track":
        raise TrackPlotError(f"{single_track_msg} (the extraction is not a 'track' circle)")

    interpolation = algorithm.get("interpolation", "linear")
    if interpolation not in ("linear", "none"):
        raise TrackPlotError(f"unknown track interpolation '{interpolation}'")

    points = algorithm.get("points", [])
    if not points:
        raise TrackPlotError("the track has no points")
    try:
        points = sorted(points, key=lambda p: int(p["step"]))
        steps = np.array([int(p["step"]) for p in points])
        lat = np.array([float(p["lat"]) for p in points])
        lon = np.array([float(p["lon"]) for p in points])
        radius = np.array([float(p["radius"]) for p in points])
    except (KeyError, TypeError, ValueError) as e:
        raise TrackPlotError(f"invalid track point (each needs step, lat, lon, radius): {e}")
    if np.any(np.diff(steps) == 0):
        raise TrackPlotError("the track contains duplicate steps")

    for key in ("user", "tag"):
        if key not in request:
            raise TrackPlotError(f"the request has no '{key}'")

    return Track(user=str(request["user"]), tag=str(request["tag"]), interpolation=interpolation,
                 steps=steps, lat=lat, lon=lon, radius=radius)


def find_files(data_dir: str, user: str, tag: str) -> Dict[int, List[str]]:
    """Output files of the request, grouped by step (several files per step: one per proc)."""
    if not os.path.isdir(data_dir):
        raise TrackPlotError(f"data directory '{data_dir}' does not exist")
    pattern = os.path.join(data_dir, f"extracted-areas-step*-proc*_user-{glob.escape(user)}"
                                     f"_tag-{glob.escape(tag)}_req-0.json")
    step_re = re.compile(r"extracted-areas-step(\d+)-proc\d+_")
    files: Dict[int, List[str]] = {}
    for path in glob.glob(pattern):
        m = step_re.match(os.path.basename(path))
        if m:
            files.setdefault(int(m.group(1)), []).append(path)
    if not files:
        raise TrackPlotError(f"no output files found matching '{pattern}'")
    return dict(sorted(files.items()))


def _to_array(values, path: str, name: str) -> np.ndarray:
    try:
        return np.asarray(values, dtype=float)
    except (TypeError, ValueError):
        try:
            return np.array([np.nan if v is None else v for v in values], dtype=float)
        except (TypeError, ValueError):
            raise TrackPlotError(f"{path}: non-numeric values in '{name}'")


def load_frame(step: int, paths: List[str]) -> Frame:
    """Read (and merge across procs) the points and fields of one step.

    The plugin writes point coordinates as [lat, lon, lev] (despite the CRS84 declaration).
    """
    lats, lons = [], []
    fields: Dict[str, List[np.ndarray]] = {}
    for path in sorted(paths):
        try:
            with open(path) as f:
                data = json.load(f)
        except (OSError, json.JSONDecodeError) as e:
            raise TrackPlotError(f"cannot read '{path}': {e}")
        coverages = data.get("coverages", [])
        if len(coverages) != 1:
            raise TrackPlotError(f"{path}: expected exactly one coverage (one track circle), found {len(coverages)}")
        coverage = coverages[0]
        try:
            coords = coverage["domain"]["axes"]["composite"]["values"]
            ranges = coverage["ranges"]
        except (KeyError, TypeError):
            raise TrackPlotError(f"{path}: unexpected CovJSON structure")
        coords = _to_array(coords, path, "coordinates").reshape(-1, 3) if coords else np.empty((0, 3))
        npts = coords.shape[0]
        lats.append(coords[:, 0])
        lons.append(coords[:, 1])
        if fields and set(ranges) != set(fields):
            raise TrackPlotError(f"{path}: fields differ between files of step {step}")
        for name, rng in ranges.items():
            values = _to_array(rng.get("values", []), path, name)
            if values.size != npts:
                raise TrackPlotError(f"{path}: '{name}' has {values.size} values for {npts} points")
            fields.setdefault(name, []).append(values)

    return Frame(step=step,
                 lat=np.concatenate(lats),
                 lon=wrap_lon_diff(np.concatenate(lons)),
                 fields={k: np.concatenate(v) for k, v in fields.items()})


def load_frames(files: Dict[int, List[str]], track: Track) -> List[Frame]:
    """Read all steps, drop empty ones and check the points are inside the track circle."""
    frames = []
    empty = 0
    for i, (step, paths) in enumerate(files.items()):
        frame = load_frame(step, paths)
        if frame.lat.size == 0:
            empty += 1
            continue
        check_frame_geometry(frame, track)
        frames.append(frame)
        if (i + 1) % 200 == 0:
            info(f"  read {i + 1}/{len(files)} steps")
    if empty:
        warn(f"{empty} steps with no points inside the circle were skipped")
    if not frames:
        raise TrackPlotError("no step has points inside the circle")
    return frames


def check_frame_geometry(frame: Frame, track: Track) -> None:
    """Sanity check: all points must be inside the track circle of their step.

    Detects a change of the coordinate order written by the plugin ([lat, lon] today).
    """
    circle = track_centre(track, frame.step)
    if circle is None:
        raise TrackPlotError(f"step {frame.step} has points but is outside the track (steps "
                             f"{track.steps[0]}-{track.steps[-1]}): data and configuration do not match")
    lat0, lon0, radius = circle
    if np.any(np.abs(frame.lat) > 90.0):
        raise TrackPlotError(f"step {frame.step}: latitudes outside [-90, 90] "
                             f"(has the coordinate order of the output changed?)")
    dist, _ = distance_bearing(lat0, lon0, frame.lat, frame.lon)
    tolerance = max(1.0, 0.01 * radius)
    if np.max(dist) > radius + tolerance:
        raise TrackPlotError(f"step {frame.step}: points up to {np.max(dist):.0f} km from the track centre "
                             f"(radius {radius:.0f} km): data and configuration do not match, "
                             f"or the coordinate order of the output has changed")


# ------------------------------------------------------------------ parameters

KNOWN_FIELDS = {
    # name: (label, units, cmap)
    "lnsp": ("Logarithm of surface pressure", "", "viridis"),
    "tcw": ("Total column water", "kg m$^{-2}$", "Blues"),
    "sst": ("Sea surface temperature", "K", "RdYlBu_r"),
    "u": ("U wind component", "m s$^{-1}$", "RdBu_r"),
    "v": ("V wind component", "m s$^{-1}$", "RdBu_r"),
    "z": ("Geopotential", "m$^2$ s$^{-2}$", "viridis"),
}


def _raw(name: str) -> Callable[[Dict[str, np.ndarray]], np.ndarray]:
    return lambda fields: fields[name]


def build_params(field_names: List[str]) -> Dict[str, ParamSpec]:
    """Plottable parameters, keyed by name and by alias."""
    interp = {}
    for name in field_names:
        m = INTERP_RE.match(name)
        if m:
            interp[name] = (m.group("base"), m.group("height"))
    exclude = set(INTERP_SOURCES) if interp else set()

    specs: List[ParamSpec] = []
    for name in sorted(field_names):
        if name in exclude:
            continue
        if name in interp:
            base, height = interp[name]
            label, units, cmap = KNOWN_FIELDS.get(base, (base, "", "viridis"))
            specs.append(ParamSpec(name, f"{base}{height}", f"{label} at {height} m", units, cmap,
                                   [name], _raw(name)))
        else:
            label, units, cmap = KNOWN_FIELDS.get(name, (name, "", "viridis"))
            specs.append(ParamSpec(name, name, label, units, cmap, [name], _raw(name)))

    # derived: surface pressure [hPa]
    if "lnsp" in field_names:
        specs.append(ParamSpec("sp", "sp", "Surface pressure", "hPa", "viridis", ["lnsp"],
                               lambda f: np.exp(f["lnsp"]) / 100.0))

    # derived: wind speed at each height where both u and v are available
    heights = sorted({h for (b, h) in interp.values() if b == "u"} & {h for (b, h) in interp.values() if b == "v"},
                     key=float)
    for h in heights:
        u, v = f"u;hl;{h}", f"v;hl;{h}"
        specs.append(ParamSpec(f"ws;hl;{h}", f"ws{h}", f"Wind speed at {h} m", "m s$^{-1}$", "YlOrRd", [u, v],
                               lambda f, u=u, v=v: np.hypot(f[u], f[v])))

    params: Dict[str, ParamSpec] = {}
    for spec in specs:
        params[spec.name] = spec
        params[spec.safe] = spec
    return params


def accepted_names(params: Dict[str, ParamSpec]) -> str:
    seen = []
    for spec in params.values():
        entry = spec.name if spec.safe == spec.name else f"{spec.safe} ('{spec.name}')"
        if entry not in seen:
            seen.append(entry)
    return ", ".join(seen)


def resolve_param(name: str, params: Dict[str, ParamSpec]) -> ParamSpec:
    if name not in params:
        raise TrackPlotError(f"unknown or unavailable parameter '{name}'. Accepted: {accepted_names(params)}")
    return params[name]


def check_consistent_fields(frames: List[Frame]) -> None:
    expected = set(frames[0].fields)
    for frame in frames[1:]:
        if set(frame.fields) != expected:
            raise TrackPlotError(f"step {frame.step}: fields {sorted(frame.fields)} differ from "
                                 f"step {frames[0].step}: {sorted(expected)}")


# ------------------------------------------------------------------ helpers

def parse_start_date(value: str) -> dt.datetime:
    for fmt in ("%Y%m%d-%H", "%Y%m%d%H"):
        try:
            return dt.datetime.strptime(value, fmt)
        except ValueError:
            pass
    try:
        return dt.datetime.fromisoformat(value)
    except ValueError:
        raise TrackPlotError(f"invalid --start-date '{value}' (expected YYYYMMDD-HH or YYYY-MM-DDTHH:MM)")


def start_date_from_dir(data_dir: str) -> dt.datetime:
    base = os.path.basename(os.path.normpath(data_dir))
    m = re.search(r"(\d{8})-(\d{2})$", base)
    if not m:
        raise TrackPlotError(f"cannot read the start date from the data directory name '{base}' "
                             f"(expected '...-YYYYMMDD-HH'): use --start-date")
    return parse_start_date(f"{m.group(1)}-{m.group(2)}")


def select_frames(frames: List[Frame], skip: int) -> List[Frame]:
    return frames[::skip + 1]


def colour_limits(values: np.ndarray, vmin: Optional[float], vmax: Optional[float]) -> Tuple[float, float]:
    finite = values[np.isfinite(values)]
    if finite.size:
        lo, hi = np.percentile(finite, DEFAULT_PERCENTILES)
    else:
        lo, hi = 0.0, 1.0
    if lo == hi:
        lo, hi = lo - 0.5, hi + 0.5
    lo = float(lo) if vmin is None else vmin
    hi = float(hi) if vmax is None else vmax
    if lo >= hi:
        raise TrackPlotError(f"colour limits are not increasing (vmin={lo:g}, vmax={hi:g}): "
                             f"adjust --vmin/--vmax")
    return lo, hi


def pressure_minimum(frame: Frame) -> Optional[int]:
    lnsp = frame.fields.get("lnsp")
    if lnsp is None or not np.any(np.isfinite(lnsp)):
        return None
    return int(np.nanargmin(lnsp))


def centres_to_edges(c: np.ndarray) -> np.ndarray:
    if c.size == 1:
        return np.array([c[0] - 0.5, c[0] + 0.5])
    mid = (c[1:] + c[:-1]) / 2.0
    return np.concatenate([[c[0] - (mid[0] - c[0])], mid, [c[-1] + (c[-1] - mid[-1])]])


def fmt_time(t: dt.datetime) -> str:
    return t.strftime("%Y-%m-%d %H:%M UTC")


# ------------------------------------------------------------------ map

def map_extent(track: Track) -> Tuple[float, float, float, float]:
    """(west, east, south, north) containing the whole track and its circles (no dateline support)."""
    lons = wrap_lon_diff(track.lon)
    rdeg_lat = track.max_radius / KM_PER_DEG_LAT
    south = max(-90.0, float(np.min(track.lat)) - rdeg_lat)
    north = min(90.0, float(np.max(track.lat)) + rdeg_lat)
    coslat = max(0.05, math.cos(max(abs(south), abs(north)) * DEG_TO_RAD))
    rdeg_lon = track.max_radius / (KM_PER_DEG_LAT * coslat)
    return float(np.min(lons)) - rdeg_lon, float(np.max(lons)) + rdeg_lon, south, north


def plot_map(frames: List[Frame], track: Track, spec: ParamSpec, start: dt.datetime, tstep: float,
             frame_dir: str, vmin: Optional[float], vmax: Optional[float]) -> List[str]:
    """Write one two-panel PNG per frame into frame_dir (created if missing); return their paths."""
    try:
        import cartopy.crs as ccrs
    except ImportError:
        raise TrackPlotError("cartopy is required for the 'map' plot type (e.g. conda env 'earthkit')")

    values = [spec.compute(f.fields) for f in frames]
    lo, hi = colour_limits(np.concatenate(values), vmin, vmax)
    rmax = track.max_radius

    os.makedirs(frame_dir, exist_ok=True)

    # static part of the figure
    fig = plt.figure(figsize=(14, 6), dpi=100)
    gs = fig.add_gridspec(1, 3, width_ratios=[1.35, 1.0, 0.04], wspace=0.25)
    ax_map = fig.add_subplot(gs[0, 0], projection=ccrs.PlateCarree())
    ax_zoom = fig.add_subplot(gs[0, 1])
    ax_cbar = fig.add_subplot(gs[0, 2])

    # map data coordinates are lon/lat (PlateCarree, central longitude 0)
    ax_map.plot(wrap_lon_diff(track.lon), track.lat, "k--", linewidth=1.0, label="prescribed track")
    ax_map.set_extent(map_extent(track), crs=ccrs.PlateCarree())
    ax_map.coastlines(resolution="10m", linewidth=0.7)
    gl = ax_map.gridlines(draw_labels=True, linewidth=0.3, color="grey", alpha=0.6)
    gl.top_labels = False
    gl.right_labels = False
    ax_map.set_title("Map")

    ax_zoom.set_xlim(-rmax, rmax)
    ax_zoom.set_ylim(-rmax, rmax)
    ax_zoom.set_aspect("equal")
    ax_zoom.set_xlabel("km east of track centre")
    ax_zoom.set_ylabel("km north of track centre")
    ax_zoom.axhline(0.0, color="grey", linewidth=0.4)
    ax_zoom.axvline(0.0, color="grey", linewidth=0.4)
    ax_zoom.set_title("Storm-relative")

    # fixed extents: plotting per frame must not rescale the axes
    ax_map.set_autoscale_on(False)
    ax_zoom.set_autoscale_on(False)

    sm = plt.cm.ScalarMappable(norm=plt.Normalize(lo, hi), cmap=spec.cmap)
    fig.colorbar(sm, cax=ax_cbar, extend="both", label=spec.title())

    pmin_style = dict(marker="*", markersize=14, markerfacecolor="red", markeredgecolor="white",
                      linestyle="none")
    centre_style = dict(marker="+", markersize=12, color="black", markeredgewidth=1.5, linestyle="none")
    # legend entries
    ax_map.plot([], [], label="track centre", **centre_style)
    ax_map.plot([], [], label="surface-pressure minimum", **pmin_style)
    ax_map.legend(loc="lower left", fontsize=8, framealpha=0.8)

    if "lnsp" not in frames[0].fields:
        warn("'lnsp' not available: surface-pressure minimum not marked")

    title = fig.suptitle("")
    paths = []
    artists = []
    for i, (frame, vals) in enumerate(zip(frames, values)):
        for a in artists:
            a.remove()
        artists = []

        lat0, lon0, radius = track_centre(track, frame.step)
        x, y, _ = storm_relative_xy(lat0, lon0, frame.lat, frame.lon)
        ok = np.isfinite(vals)

        tri_map = tri_zoom = None
        if np.count_nonzero(ok) >= 3:
            try:
                tri_map = mtri.Triangulation(frame.lon[ok], frame.lat[ok])
                tri_zoom = mtri.Triangulation(x[ok], y[ok])
            except (RuntimeError, ValueError) as e:   # degenerate triangulation (e.g. collinear points)
                warn(f"step {frame.step}: cannot triangulate the points ({e}), drawing them as dots")
                tri_map = tri_zoom = None
        if tri_map is not None:
            artists.append(ax_map.tripcolor(tri_map, vals[ok], shading="gouraud",
                                            cmap=spec.cmap, vmin=lo, vmax=hi))
            artists.append(ax_zoom.tripcolor(tri_zoom, vals[ok], shading="gouraud",
                                             cmap=spec.cmap, vmin=lo, vmax=hi))
        else:
            artists.append(ax_map.scatter(frame.lon, frame.lat, c=vals, s=12, cmap=spec.cmap, vmin=lo, vmax=hi))
            artists.append(ax_zoom.scatter(x, y, c=vals, s=12, cmap=spec.cmap, vmin=lo, vmax=hi))

        artists.append(ax_zoom.add_patch(CirclePatch((0.0, 0.0), radius, fill=False, color="black",
                                                     linewidth=0.8)))
        artists += ax_map.plot([lon0], [lat0], **centre_style)
        artists += ax_zoom.plot([0.0], [0.0], **centre_style)
        ipmin = pressure_minimum(frame)
        if ipmin is not None:
            artists += ax_map.plot([frame.lon[ipmin]], [frame.lat[ipmin]], **pmin_style)
            artists += ax_zoom.plot([x[ipmin]], [y[ipmin]], **pmin_style)

        valid = start + dt.timedelta(seconds=frame.step * tstep)
        title.set_text(f"{spec.title()} — step {frame.step} — {fmt_time(valid)}")

        path = os.path.join(frame_dir, f"{spec.safe}_step{frame.step:05d}.png")
        fig.savefig(path)
        paths.append(path)
        if (i + 1) % 50 == 0 or i + 1 == len(frames):
            info(f"  written {i + 1}/{len(frames)} images")

    plt.close(fig)
    return paths


def make_gif(paths: List[str], gif_path: str, fps: float) -> None:
    from PIL import Image

    duration = max(10, int(round(1000.0 / fps)))   # [ms], GIF resolution is 10 ms

    def frames_iter():
        for p in paths[1:]:
            with Image.open(p) as im:
                yield im.convert("RGB").convert("P", palette=Image.Palette.ADAPTIVE)

    with Image.open(paths[0]) as first:
        first = first.convert("RGB").convert("P", palette=Image.Palette.ADAPTIVE)
        first.save(gif_path, save_all=True, append_images=frames_iter(), duration=duration, loop=0)


# ------------------------------------------------------------------ time series

def plot_time(frames: List[Frame], track: Track, params: Dict[str, ParamSpec], start: dt.datetime,
              tstep: float, output_dir: str) -> str:
    times = [start + dt.timedelta(seconds=f.step * tstep) for f in frames]

    wind = sorted((s for s in params.values() if s.name.startswith("ws;hl;")), key=lambda s: float(s.safe[2:]))
    panels = []   # (title, values)

    if "sp" in params:
        sp = params["sp"]
        panels.append((f"Min {sp.label.lower()} [{sp.units}]",
                       [np.nanmin(sp.compute(f.fields)) for f in frames]))
    else:
        warn("'lnsp' not available: minimum surface pressure and drift panels dropped")
    if wind:
        ws = wind[0]
        panels.append((f"Max {ws.label.lower()} [{ws.units}]",
                       [np.nanmax(ws.compute(f.fields)) for f in frames]))
    else:
        warn("height-interpolated u and v not available: maximum wind speed panel dropped")
    for name in ("tcw", "sst"):
        if name in params:
            s = params[name]
            panels.append((f"Mean {s.label.lower()} [{s.units}]",
                           [np.nanmean(s.compute(f.fields)) for f in frames]))
        else:
            warn(f"'{name}' not available: mean {name} panel dropped")
    if "sp" in params:
        drift = []
        for f in frames:
            lat0, lon0, _ = track_centre(track, f.step)
            i = pressure_minimum(f)
            drift.append(np.nan if i is None else
                         float(distance_bearing(lat0, lon0, f.lat[i], f.lon[i])[0]))
        panels.append(("Pressure minimum to track centre [km]", drift))

    if not panels:
        raise TrackPlotError("none of the fields needed by the 'time' plot type is available")

    fig, axes = plt.subplots(len(panels), 1, figsize=(11, 2.3 * len(panels)), sharex=True, squeeze=False)
    for ax, (label, vals) in zip(axes[:, 0], panels):
        ax.plot(times, vals, linewidth=1.2)
        ax.set_ylabel(label, fontsize=8)
        ax.grid(True, linewidth=0.3)
    axes[-1, 0].xaxis.set_major_formatter(mdates.DateFormatter("%m-%d %HZ"))
    axes[-1, 0].set_xlabel("valid time (UTC)")
    fig.autofmt_xdate()
    fig.suptitle(f"Storm-relative time series — track '{track.tag}' (radius ≤ {track.max_radius:g} km)")
    fig.tight_layout()

    path = os.path.join(output_dir, "time_series.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path


# ------------------------------------------------------------------ radius-time

def plot_radius(frames: List[Frame], track: Track, spec: ParamSpec, start: dt.datetime, tstep: float,
                output_dir: str, bin_width: float, vmin: Optional[float], vmax: Optional[float]) -> str:
    rmax = track.max_radius
    edges = np.arange(0.0, rmax, bin_width)
    edges = np.append(edges, rmax)
    nbins = edges.size - 1

    means = np.full((len(frames), nbins), np.nan)
    for i, f in enumerate(frames):
        lat0, lon0, _ = track_centre(track, f.step)
        dist, _ = distance_bearing(lat0, lon0, f.lat, f.lon)
        vals = spec.compute(f.fields)
        ok = np.isfinite(vals)
        idx = np.clip(np.digitize(dist[ok], edges) - 1, 0, nbins - 1)
        counts = np.bincount(idx, minlength=nbins)
        sums = np.bincount(idx, weights=vals[ok], minlength=nbins)
        with np.errstate(invalid="ignore", divide="ignore"):
            means[i] = np.where(counts > 0, sums / counts, np.nan)

    lo, hi = colour_limits(means.ravel(), vmin, vmax)
    tnum = mdates.date2num([start + dt.timedelta(seconds=f.step * tstep) for f in frames])
    tedges = centres_to_edges(np.asarray(tnum))

    fig, ax = plt.subplots(figsize=(8, 9))
    mesh = ax.pcolormesh(edges, tedges, np.ma.masked_invalid(means), cmap=spec.cmap, vmin=lo, vmax=hi,
                         shading="flat")
    fig.colorbar(mesh, ax=ax, extend="both", label=spec.title())
    ax.yaxis_date()
    ax.yaxis.set_major_formatter(mdates.DateFormatter("%m-%d %HZ"))
    ax.set_xlabel("distance from track centre [km]")
    ax.set_ylabel("valid time (UTC)")
    ax.set_xlim(0.0, rmax)
    ax.set_title(f"Azimuthal mean of {spec.label.lower()} ({bin_width:g} km bins)")
    fig.tight_layout()

    path = os.path.join(output_dir, f"radius_{spec.safe}.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path


# ------------------------------------------------------------------ CLI

def build_parser() -> argparse.ArgumentParser:
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--config", required=True, help="Plume configuration file used for the run")
    common.add_argument("--data-dir", required=True, help="directory with the CovJSON output files")
    common.add_argument("--output-dir", default=".", help="where products are written (default: current directory)")
    common.add_argument("--start-date", help="run start date, YYYYMMDD-HH or YYYY-MM-DDTHH:MM "
                                             "(default: from the --data-dir name, '...-YYYYMMDD-HH')")
    common.add_argument("--tstep", type=float, default=DEFAULT_TSTEP,
                        help=f"model timestep [s] (default: {DEFAULT_TSTEP:g})")

    limits = argparse.ArgumentParser(add_help=False)
    limits.add_argument("--vmin", type=float, help="colour-scale minimum (default: 1st percentile of the data)")
    limits.add_argument("--vmax", type=float, help="colour-scale maximum (default: 99th percentile of the data)")

    param_help = "parameter to plot (e.g. sp, tcw, sst, ws5000 or 'ws;hl;5000'); an unknown name lists the accepted ones"

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="plot_type", required=True, metavar="{map,time,radius}")

    p_map = sub.add_parser("map", parents=[common, limits], help="animated GIF of two-panel frames, one per step")
    p_map.add_argument("param", help=param_help)
    p_map.add_argument("--skip-frames", type=int, default=0,
                       help="output files skipped after each plotted one (default: 0, plot all)")
    p_map.add_argument("--keep-images", action="store_true",
                       help="also keep the frame images (in <output-dir>/map_<param>/); by default only the GIF is kept")
    p_map.add_argument("--fps", type=float, default=DEFAULT_FPS,
                       help=f"GIF frame rate (default: {DEFAULT_FPS:g})")

    sub.add_parser("time", parents=[common], help="multi-panel time series of summary quantities")

    p_rad = sub.add_parser("radius", parents=[common, limits], help="radius-time plot of the azimuthal mean")
    p_rad.add_argument("param", help=param_help)
    p_rad.add_argument("--bin-width", type=float, default=DEFAULT_BIN_WIDTH,
                       help=f"radial bin width [km] (default: {DEFAULT_BIN_WIDTH:g})")
    return parser


def validate_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    if not args.tstep > 0:
        parser.error("--tstep must be > 0")
    if getattr(args, "skip_frames", 0) < 0:
        parser.error("--skip-frames must be >= 0")
    if not getattr(args, "fps", 1.0) > 0:
        parser.error("--fps must be > 0")
    if not getattr(args, "bin_width", 1.0) > 0:
        parser.error("--bin-width must be > 0")
    vmin, vmax = getattr(args, "vmin", None), getattr(args, "vmax", None)
    if vmin is not None and vmax is not None and vmin >= vmax:
        parser.error("--vmin must be < --vmax")
    if not os.path.isfile(args.config):
        parser.error(f"--config '{args.config}' is not a file")
    if not os.path.isdir(args.data_dir):
        parser.error(f"--data-dir '{args.data_dir}' is not a directory")


def map_to_gif(frames: List[Frame], track: Track, spec: ParamSpec, start: dt.datetime,
               args: argparse.Namespace, frame_dir: str, gif_path: str) -> None:
    info(f"plotting {len(frames)} frames of '{spec.name}' ...")
    paths = plot_map(frames, track, spec, start, args.tstep, frame_dir, args.vmin, args.vmax)
    info(f"assembling GIF ({len(paths)} frames, {args.fps:g} fps) ...")
    make_gif(paths, gif_path, args.fps)


def run(args: argparse.Namespace) -> None:
    start = parse_start_date(args.start_date) if args.start_date else start_date_from_dir(args.data_dir)

    track = load_track(args.config)
    files = find_files(args.data_dir, track.user, track.tag)
    info(f"track '{track.tag}': {track.steps.size} points, steps {track.steps[0]}-{track.steps[-1]}; "
         f"{len(files)} output steps; start {fmt_time(start)}, tstep {args.tstep:g} s")

    # validate the parameter on the first file before reading everything
    first = load_frame(*next(iter(files.items())))
    params = build_params(list(first.fields))
    spec = resolve_param(args.param, params) if args.plot_type in ("map", "radius") else None

    info("reading output files ...")
    frames = load_frames(files, track)
    check_consistent_fields(frames)
    params = build_params(list(frames[0].fields))

    os.makedirs(args.output_dir, exist_ok=True)

    if args.plot_type == "map":
        selected = select_frames(frames, args.skip_frames)
        gif_path = os.path.join(args.output_dir, f"map_{spec.safe}.gif")
        if args.keep_images:
            frame_dir = os.path.join(args.output_dir, f"map_{spec.safe}")
            map_to_gif(selected, track, spec, start, args, frame_dir, gif_path)
            info(f"images kept in {frame_dir}")
        else:
            # frames go to a temporary directory, removed afterwards (also on error)
            with tempfile.TemporaryDirectory(prefix=f".map_{spec.safe}_", dir=args.output_dir) as tmp_dir:
                map_to_gif(selected, track, spec, start, args, tmp_dir, gif_path)
        info(f"GIF written to {gif_path}")
    elif args.plot_type == "time":
        info(f"time series written to {plot_time(frames, track, params, start, args.tstep, args.output_dir)}")
    else:
        path = plot_radius(frames, track, spec, start, args.tstep, args.output_dir, args.bin_width,
                           args.vmin, args.vmax)
        info(f"radius-time plot written to {path}")


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    validate_args(parser, args)
    try:
        run(args)
    except TrackPlotError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
