
# coding: utf-8

# # An Introduction to the PAIRS API

# Python API examples.

# In[55]:

import requests                                  # To make URL requests
import datetime                                  # To deal with timestamps
import pytz                                      # To deal with timezones
import pandas as pd                              # To store data in data frames
import os                                        # To write files, interact with the OS.
from time import sleep                           # To wait
import numpy as np                               # Used to store and manipulate raster data
import gdal                                      # To work with GeoTiff files
from matplotlib import pyplot as plt, cm         # To display GeoTiff files


# Enter username and password here.

# In[67]:

myAuthorization = ('sgaitan@nl.ibm.com', 'my_IBM_password')


# ## Point Queries

# Date & time information is specified as "Epoch time". I.e. seconds since 1/1/1970 0:00 UTC.
# 
# Note: It is much safer to always work in UTC.

# In[68]:

utc = pytz.utc
epoch = datetime.datetime(year=1970, month=1, day=1, hour=0, minute=0, second=0, tzinfo=utc)


# PAIRS actually measures the time in milliseconds since 0 epoch time.

# In[69]:

startTime = (datetime.datetime(year=2017, month=1, day=1, hour=0, tzinfo=utc) - epoch).total_seconds() * 1000
endTime = (datetime.datetime(year=2017, month=3, day=31, hour=23, minute=59, tzinfo=utc) - epoch).total_seconds() * 1000


# To submit queries: Call "POST" on https://pairs.res.ibm.com/v2/query

# In[70]:

query = {
    "layers" : [
        {"type" : "raster", "id" : 51},                 # What?
    ],
    "spatial" : {
        "type" : "point", "coordinates" : [34.0, 135.75]   # Where?
    },
    "temporal" : {
        "intervals" : [
            {"start" : startTime, "end" : endTime},        # When?
        ]
    },
    "outputType" : "json",
}


# 34.0, 135.75 is Kyoto.

# In[71]:

URL = 'https://pairs.res.ibm.com/v2/query'
response = requests.post(URL, json = query, auth = myAuthorization)
response.status_code


# In[72]:

response.json()


# We can use pandas to store the data in a data frame.

# In[73]:

responseData = response.json()['data']
responseData = pd.DataFrame(responseData)
responseData


# The timestamps are still in milliseconds since 01/01/1970. We can convert them back to UTC.

# In[74]:

responseData['timestamp'] = (responseData['timestamp'] / 1000).map(datetime.datetime.utcfromtimestamp)
responseData


# ## Raster Queries - Agriculture in Northern California

# As an example, we look at the NDVI data and Daymet maximum temperature for North America. Specifically, the area around San Francisco.
# South West corner: 35.0, -124.0
# North East corner: 40.0, -115.0

# In[75]:

startTime = (datetime.datetime(year=2016, month=9, day=6, hour=0, tzinfo=utc)
             - epoch).total_seconds() * 1000
endTime = (datetime.datetime(year=2016, month=9, day=6, hour=23, minute=59, tzinfo=utc)
           - epoch).total_seconds() * 1000


# In[76]:

query = {
    "layers" : [
        {"type" : "raster", "id" : 51},              # Modis Aqua 13 sattelite, NDVI
        {"type" : "raster", "id" : 15300},           # ECMWF Based Evapotranspiration
    ],
    "spatial" : {
        "type" : "square",
        "coordinates" : [37.0, -120.0, 38.0, -119.0] # SW Lat, SW Long, NE Lat, NE Long
    },
    "temporal" : {
        "intervals" : [
            {"start" : startTime, "end" : endTime},
        ]
    },
    "outputType" : "json",
}


# In[77]:

URL = 'https://pairs.res.ibm.com/v2/query'
queryResponse = requests.post(URL, json = query, auth = myAuthorization)
queryId = queryResponse.json()['id']
print queryId


# These queries are more complicated. We actually have to wait a bit. So let's use the queryId to get the status:

# In[78]:

URL = 'https://pairs.res.ibm.com/ws/queryjobs/'
response = requests.get(URL + queryId, auth = myAuthorization)
response.json()


# We need to wait. Which we can automatize:

# In[79]:

URL = 'https://pairs.res.ibm.com/ws/queryjobs/'
while True:
    response = requests.get(URL + queryId, auth = myAuthorization)
    if response.json()['ready']:
        break
    else:
        print response.json()['status']
        sleep(5)
print response.json()['status']


# ### Downloading Raster Data

# We want to download the data. Before doing so, let's check the current working directory where we will save the file:

# In[80]:

os.getcwd()


# We can then download the data making a simple request:

# In[81]:

URL = 'https://pairs.res.ibm.com/ws/queryjobs/download/'
download = requests.get(URL + queryId, auth = myAuthorization, stream = True)
download.ok


# To write to file:

# In[82]:

with open('PAIRS_raster_example.zip', 'wb') as file:
    for block in download.iter_content(1024):
        file.write(block)


# ### Reading and Displaying Raster Data with GDAL and Matplotlib

# (Note: You should extract the files by hand first. Python can do this if necessary.)

# First we load the data into a numpy array.

# In[35]:

fileName='MODIS, Aqua, 13 (NASA 250m resolution satellite)-NDVI-09_05_2016T00_00_00.tiff'
daymetGeoTiff = gdal.Open(fileName)
daymetGeoTiffBand = daymetGeoTiff.GetRasterBand(1)
daymetGeoTiffData = np.array(daymetGeoTiffBand.ReadAsArray(), dtype = np.float)
daymetGeoTiffData[ daymetGeoTiffData == np.float(daymetGeoTiffBand.GetNoDataValue()) ] = np.nan
daymetGeoTiff = None


# Then we display it using matplotlib.

# In[36]:

plt.figure(figsize = (20, 10), dpi = 80) # Sets the size and resolution of the plot
plt.imshow(-daymetGeoTiffData, cmap = cm.Spectral) # Loads the raster data and sets the colormap
plt.colorbar()  # Adds the color bar
plt.axis('off') # Removes the axes
plt.show()      # Displays the image


# In[37]:

fileName='Reference Evapotranspiration-ECMWF based reference evapotranspiration-09_06_2016T00_00_00.tiff'
modisGeoTiff = gdal.Open(fileName)
modisGeoTiffBand = modisGeoTiff.GetRasterBand(1)
modisGeoTiffData = np.array(modisGeoTiffBand.ReadAsArray(), dtype = np.float)
modisGeoTiffData[ modisGeoTiffData == np.float(modisGeoTiffBand.GetNoDataValue()) ] = np.nan
modisGeoTiff = None


# In[38]:

plt.figure(figsize = (20, 10), dpi = 80) # Sets the size and resolution of the plot
plt.imshow(modisGeoTiffData, cmap = cm.YlGn) # Loads the raster data and sets the colormap
plt.colorbar()  # Adds the color bar
plt.axis('off') # Removes the axes
plt.show()      # Displays the image


# In[ ]:



