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

#!/usr/bin/env python

import json
import argparse
from data_aggregator import DataAggregator

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("run_dir", help="path to directory containing extraction data files", type=str)
    parser.add_argument("step", help="time step to process", type=int)
    
    args = parser.parse_args()

    aggregator = DataAggregator(args.run_dir, args.step)
    aggregated = aggregator.run()

    print("all done.")


    



