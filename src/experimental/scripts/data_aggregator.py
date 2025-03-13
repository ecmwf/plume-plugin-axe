import os
import json
from pprint import pprint
import copy

import pandas as pd

from covjson_template import range_templ
from covjson_template import domain_templ
from covjson_template import coverage_templ
from covjson_template import crs_templ
from covjson_template import covjson_template
from covjson_template import parameter_definitions



class DataAggregator:

    def __init__(self, run_dir, step) -> None:
        self.run_dir = run_dir
        self.step = step
        self.areafile_name_prefix = "extracted-areas-step"
        self.areafile_name_ext = "csv"


    def run(self):

        # find files for tstep
        files_per_step = self._find_extraction_files()
        print(f"Processing step {self.step}")
        print(files_per_step[self.step])

        # extract values from files
        values_df = self._read_files(files_per_step[self.step])

        # find all users
        users_uniq_df = values_df[["user"]].value_counts(ascending=True).reset_index(name='count')
        print(users_uniq_df["user"])

        # Assemble one covjson file for each user (that includes all the requested areas)
        for user_name in users_uniq_df["user"]:

            print(f"Processing user: {user_name}")
            
            # filter user data
            user_df = values_df[values_df["user"] == user_name]
            
            # assemble the covJSON
            covjson = self._assemble_user_covjson(user_df)

            # write to file
            outfile = f"covjson_user_{user_name}-step{str(self.step)}.json"

            self._write_covjson_file(covjson, outfile)


    def _find_extraction_files(self):
        files_per_step = {}
        for root, dirs, files in os.walk(self.run_dir, followlinks=False):
            for file in files:
                if file.startswith(self.areafile_name_prefix) and file.endswith("."+self.areafile_name_ext):

                    tail = file.split(self.areafile_name_prefix)[1]
                    tstep = int(tail.split("-")[0])

                    # Append to the list of files per area
                    files_per_step.setdefault(int(tstep),[]).append(file)
        return files_per_step


    def _read_files(self, files):
        """
        Read area extraction CSV files
        """

        values_df_list = []
        for file in files:
            print(f"Reading file: {file}")
            df = pd.read_csv( os.path.join(self.run_dir, file))
            values_df_list.append(df)

        values_df = pd.concat(values_df_list, axis=0, ignore_index=True)
        
        return values_df


    def _assemble_user_covjson(self, user_df):
        """
        Assemble the covjson file (for a single user)
        """

        # Covjson template
        covjson = copy.deepcopy(covjson_template)

        # parameters
        parameters = {}

        # Append CRS
        covjson["referencing"].append(copy.deepcopy(crs_templ))

        # ==== Find all unique areas in user_df ====
        users_area_uniq_df = user_df[["area_idx"]].value_counts().reset_index(name='count')

        coverages = []
        for area_name in users_area_uniq_df["area_idx"]:
            print(f"==> Processing area: {area_name}")
            user_area_df = user_df[user_df["area_idx"] == area_name]

            # Append coverage lat/lon values
            coverage = copy.deepcopy(coverage_templ)

            # coverage domain
            coverage["domain"] = copy.deepcopy(domain_templ)            
            coverage["domain"]["axes"]["t"]["values"] = ["2017-01-01T00:00:00"] # TODO take date from run
            coverage["domain"]["axes"]["composite"]["values"] = user_area_df[["lat","lon","lev"]].values.tolist()

            # ---- ranges ----
            # Find all unique areas in user_df
            users_area_param_uniq_df = user_area_df[["param"]].value_counts().reset_index(name='count')
            for rr, row in users_area_param_uniq_df.iterrows():

                param = row["param"]
                count = row["count"]
                print(f"----> Processing param: {param} with N values: {count}")
                user_area_param_df = user_area_df[user_area_df["param"] == param]

                range = copy.deepcopy(range_templ)
                range["shape"] = [count]
                range["axisNames"] = [param]
                range["values"] = user_area_param_df[["value"]].values.tolist()

                # Check if this param has a corresponding definition
                if param not in parameters:
                    parameters[param] = copy.deepcopy(parameter_definitions[param])

                # Insert range to coverage
                coverage["ranges"][param] = range
            
            # Append coverage to list of coverages
            coverages.append(coverage)

        # Append all the coverages to the covjson
        covjson["coverages"] = coverages
        covjson["parameters"] = parameters

        return covjson

    def _write_covjson_file(self, covjson, filename):
        print(f"Writing output file: {filename}..")
        with open(filename, 'w', encoding='utf-8') as f:
            pprint(covjson, f, width=160)


