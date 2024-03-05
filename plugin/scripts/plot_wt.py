 
#  (C) Copyright 2023- ECMWF.
# 
#  This software is licensed under the terms of the Apache Licence Version 2.0
#  which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# 
#  In applying this licence, ECMWF does not waive the privileges and immunities
#  granted to it by virtue of its status as an intergovernmental organisation nor
#  does it submit to any jurisdiction.
# 

import pandas as pd
import matplotlib.pyplot as plt

wtdata = pd.read_csv("./wt_output.txt", header=None)

# create figure and axis objects with subplots()
fig,ax = plt.subplots()

# first plot and axis
ax.plot(wtdata[0], wtdata[1], color="red",  marker="o")
ax.set_xlabel("step", fontsize = 14)
ax.set_ylabel("$U_{100}$", color="red", fontsize=14)
ax.set_ylim([5,15])

# second plot and axis
ax2=ax.twinx()
ax2.plot(wtdata[0], wtdata[2],color="blue",marker="o")
ax2.set_ylabel("$P_{WT}$ [Watts]",color="blue",fontsize=14)
ax2.set_ylim([500000,2000000])

# save the plot as a file
plt.show()
fig.savefig('step_U100_Power.jpg', format='jpeg', dpi=100, bbox_inches='tight')
