
# ============================================================
# ================= complete CovJSON example =================
# ============================================================
# {
#     "type": "CoverageCollection",
#     "domainType": "MultiPoint",
#     "coverages": [
#         {
#             "mars:metadata": {
#                 "class": "od",
#                 "stream": "oper",
#                 "levtype": "pl",
#                 "date": "20170101",
#                 "step": "0",
#                 "number": "0",
#             },
#             "type": "Coverage",
#             "domain": {
#                 "type": "Domain",
#                 "axes": {
#                     "t": {"values": ["2017-01-01T00:00:00"]},
#                     "composite": {
#                         "dataType": "tuple",
#                         "coordinates": ["x", "y", "z"],
#                         "values": [[1, 20, 1], [2, 21, 3], [3, 17, 7]],
#                     },
#                 },
#             },
#             "ranges": {
#                 "t": {
#                     "type": "NdArray",
#                     "dataType": "float",
#                     "shape": [3],
#                     "axisNames": ["t"],
#                     "values": [
#                         264.93115234375,
#                         263.83115234375,
#                         265.12313132266,
#                     ],
#                 },
#                 "p": {
#                     "type": "NdArray",
#                     "dataType": "float",
#                     "shape": [3],
#                     "axisNames": ["t"],
#                     "values": [
#                         9.93115234375,
#                         7.83115234375,
#                         14.12313132266,
#                     ],
#                 },
#             },
#         },
#         {
#             "mars:metadata": {
#                 "class": "od",
#                 "stream": "oper",
#                 "levtype": "pl",
#                 "date": "20170101",
#                 "step": "1",
#                 "number": "0",
#             },
#             "type": "Coverage",
#             "domain": {
#                 "type": "Domain",
#                 "axes": {
#                     "t": {"values": ["2017-01-01T01:00:00"]},
#                     "composite": {
#                         "dataType": "tuple",
#                         "coordinates": ["x", "y", "z"],
#                         "values": [[1, 20, 1], [2, 21, 3], [3, 17, 7]],
#                     },
#                 },
#             },
#             "ranges": {
#                 "t": {
#                     "type": "NdArray",
#                     "dataType": "float",
#                     "shape": [3],
#                     "axisNames": ["t"],
#                     "values": [
#                         266.93115234375,
#                         293.83115234375,
#                         165.12313132266,
#                     ],
#                 },
#                 "p": {
#                     "type": "NdArray",
#                     "dataType": "float",
#                     "shape": [3],
#                     "axisNames": ["t"],
#                     "values": [
#                         1.93115234375,
#                         22.83115234375,
#                         12.12313132266,
#                     ],
#                 },
#             },
#         },
#     ],
#     "referencing": [
#         {
#             "coordinates": ["x", "y", "z"],
#             "system": {
#                 "type": "GeographicCRS",
#                 "id": "http://www.opengis.net/def/crs/OGC/1.3/CRS84",
#             },
#         }
#     ],
#     "parameters": {
#         "t": {
#             "type": "Parameter",
#             "description": "Temperature",
#             "unit": {"symbol": "K"},
#             "observedProperty": {"id": "t", "label": {"en": "Temperature"}},
#         },
#         "p": {
#             "type": "Parameter",
#             "description": "Pressure",
#             "unit": {"symbol": "pa"},
#             "observedProperty": {"id": "p", "label": {"en": "Pressure"}},
#         },
#     },
# }
# ============================================================
# ============================================================
# ============================================================


range_templ = {
    "type": "NdArray",
    "dataType": "float",
    "shape": [], # N values
    "axisNames": [], # there should be only 1 value (e.g. "t")
    "values": [] # e.g. [111.11, 222.22, 333.33],
}

metadata_templ = {
    "class": "od",
    "stream": "oper",
    "levtype": "pl",
    "date": "20170101",
    "step": "0",
    "number": "0"
}

domain_templ = {
    "type": "Domain",
    "axes": {
        "t": {"values": []}, # there should be only 1 value (e.g. ["2017-01-01T00:00:00"])
        "composite": {
            "dataType": "tuple",
            "coordinates": ["x", "y", "z"],
            "values": [] # list of x,y,z coords (e.g. [[1, 20, 1], [2, 21, 3]])
        }
    }
}

# aggregate: range, metadata, domain -> coverage
coverage_templ = {
    "area:metadata": {},
    "type": "Coverage",
    "domain": {},
    "ranges": {}
}

# CRS goes into referencing
crs_templ = {
    "coordinates": ["x", "y", "z"],
    "system": {
        "type": "GeographicCRS",
        "id": "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
    }
}

# Parameter description
param_templ = {
    "type": "Parameter",
    "description": "Pressure",
    "unit": {
        "symbol": "" # e.g. "pa"
    },
    "observedProperty": {
        "id": "", # e.g. "p" 
        "label": {"en": ""} # e.g. "Pressure"
    }
}


covjson_template = {
    "type": "CoverageCollection",
    "domainType": "MultiPoint",
    "coverages": [],
    "referencing": [],
    "parameters": {}
}


parameter_definitions = {
    "p" : {
        "type": "Parameter",
        "description": "Pressure",
        "unit": {"symbol": "Pa"},
        "observedProperty": {"id": "p", "label": {"en": "Pressure"}}
    },
    "u" : {
        "type": "Parameter",
        "description": "u-component of wind speed",
        "unit": {"symbol": "m/s"},
        "observedProperty": {"id": "v", "label": {"en": "u-component of wind speed"}}
    },
    "v" : {
        "type": "Parameter",
        "description": "v-component of wind speed",
        "unit": {"symbol": "m/s"},
        "observedProperty": {"id": "v", "label": {"en": "v-component of wind speed"}}
    }
}