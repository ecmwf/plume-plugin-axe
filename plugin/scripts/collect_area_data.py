#!/usr/bin/env python

import os
import json
import argparse


# ================================================================
# {
#     "user": "user-1",
#     "extraction_areas": [
#         {"north": 15, "south": -15, "east": 115, "west": 15},
#         {"north": 16, "south": -16, "east": 116, "west": 16},
#         {"north": 17, "south": -17, "east": 117, "west": 17}
#     ],
#     "s3_url": "s3_url_1"
# },
# ================================================================


def aggregate_data(values):
    """
    Aggregate data per user/area (resembles request file)
    """
    requests = {}
    for vals in values:
        (user, area_idx, point_idx, lat, lon, lev, param, value) = vals
        requests.setdefault(user,{"areas":{}})["areas"].setdefault(area_idx, []).append([point_idx, lat, lon, lev, param, value])

    return requests


def read_extracted_data_csv_files(csv_files_path):
    """
    Read area extraction CSV files
    """

    # extracted-areas-step0-proc1.csv
    # extracted-areas-step0-proc0.csv
    # extracted-areas-step1-proc1.csv
    # extracted-areas-step1-proc0.csv
    # extracted-areas-step2-proc1.csv
    # extracted-areas-step2-proc0.csv

    values_at_timestep = {}
    for root,dirs,files in os.walk(csv_files_path, followlinks=False):
        for file in files:
            if file.startswith("extracted-areas-step") and file.endswith(".csv"):

                tail = file.split("extracted-areas-step")[1]
                tstep = int(tail.split("-")[0])
                values = values_at_timestep.setdefault(tstep, [])

                with open(os.path.join(csv_files_path, file), 'r') as f:
                    for rr,row in enumerate(f):

                        if (rr):

                            (user, area_idx, point_idx, lat, lon, lev, param, value,  _) = row.split(",")

                            user = user.strip()
                            area_idx = int(area_idx)
                            point_idx = int(point_idx)
                            lat = float(lat)
                            lon = float(lon)
                            lev = int(lev)
                            param = param.strip()
                            value = float(value)

                            values.append([user, area_idx, point_idx, lat, lon, lev, param, value])

    return values_at_timestep



if __name__ == "__main__":

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("run_dir", help="path to directory containing extraction data files", type=str)
    
    args = parser.parse_args()

    # Read the values from CSV files 
    values_at_timestep = read_extracted_data_csv_files(args.run_dir)

    # Aggregate
    for tstep in sorted(values_at_timestep):

        requests = aggregate_data(values_at_timestep[tstep])

        # Write aggregated data to JSON
        outfile = 'aggregated-areas-step' + str(tstep) + '.json'

        print(f"Writing output file: {outfile}..")
        with open(outfile, 'w', encoding='utf-8') as f:
            json.dump(requests, f, ensure_ascii=False, indent=4)



    



