# Copyright (c) 2020-2021 Adrian Georg Herrmann

import os
from flask import Flask, request
from scipy import interpolate
import numpy as np
import time
import datetime

app = Flask(__name__)
app.config["location"] = os.environ.get("LOCATION")
datapath = os.environ.get("DATAPATH") if os.environ.get("DATAPATH") else "data"

weather_cache = {}
weather_params = [ "temp", "humid", "press", "cloud", "rad" ]
stations = [ "wijchen1", "wijchen2", "berlin1", "berlin2" ]

def preprocess_cache(weather_cache):
    for station in stations:
        for param in weather_params:
            to_del = []
            for key, val in weather_cache[station][param].items():
                if val is None:
                    to_del.append(key)
            for x in to_del:
                del weather_cache[station][param][x]

def get_time_ranges(weather_cache):
    time_start = {
        "wijchen1": min(weather_cache["wijchen1"]["temp"].keys()),
        "wijchen2": min(weather_cache["wijchen2"]["temp"].keys()),
        "berlin1": min(weather_cache["berlin1"]["temp"].keys()),
        "berlin2": min(weather_cache["berlin2"]["temp"].keys()),
    }
    time_end = {
        "wijchen1": max(weather_cache["wijchen1"]["temp"].keys()),
        "wijchen2": max(weather_cache["wijchen2"]["temp"].keys()),
        "berlin1": max(weather_cache["berlin1"]["temp"].keys()),
        "berlin2": max(weather_cache["berlin2"]["temp"].keys())
    }

    for station in stations:
        for param in weather_params:
            try:
                min_ = min(weather_cache[station][param].keys())
            except ValueError:
                min_ = time_start[station]
            try:
                max_ = max(weather_cache[station][param].keys())
            except ValueError:
                max_ = time_end[station]

            if min_ > time_start[station]:
                time_start[station] = min_

            if max_ < time_end[station]:
                time_end[station] = max_

    time_ranges = {
        "wijchen1": range(time_start["wijchen1"], time_end["wijchen1"] + 1, 900),
        "wijchen2": range(time_start["wijchen2"], time_end["wijchen2"] + 1, 900),
        "berlin1": range(time_start["berlin1"], time_end["berlin1"] + 1, 900),
        "berlin2": range(time_start["berlin2"], time_end["berlin2"] + 1, 900)
    }

    return time_ranges

def interpolate_time_in_cache(weather_cache, time_ranges):
    weather_cache_ = {}

    for station in stations:
        weather_cache_[station] = {}

        for param in weather_params:
            keys = list(weather_cache[station][param].keys())
            values = list(weather_cache[station][param].values())

            if not len(keys) or not len(values):
                continue

            f = interpolate.interp1d(keys, values)

            weather_cache_[station][param] = dict(zip(time_ranges[station], f(time_ranges[station])))

    return weather_cache_


# Read cache file into memory, if existing.
# Contact the author for a sample of data, see doc/thesis.pdf, page 72.
cache_path = os.path.join(os.getcwd(), datapath, "weather.npy")
if os.path.isfile(cache_path):
    weather_cache = np.load(cache_path, allow_pickle=True).item()
    preprocess_cache(weather_cache)
    time_ranges = get_time_ranges(weather_cache)
    weather_cache = interpolate_time_in_cache(weather_cache, time_ranges)

@app.route('/')
def get_weather():
    time_ = request.args.get("time")
    station = request.args.get("station")
    if time_ is None:
        return "", 204
    if station is None:
        station = '1'

    if not app.config["location"] in ["wijchen", "berlin"]:
        location = "wijchen"
    else:
        location = app.config["location"]

    if location == "wijchen":
        if station == '1':
            cache_key = "wijchen1"
        else:
            cache_key = "wijchen2"
    else:
        if station == '1':
            cache_key = "berlin1"
        else:
            cache_key = "berlin2"

    # Catch invalid timestamps.
    try:
        timestamp = int(time.mktime(datetime.datetime.strptime(time_, "%Y%m%d%H%M").timetuple()))
    except:
        return "", 204

    weather_dict = {}

    for param in weather_params:
        weather_dict[param] = weather_cache[cache_key][param][timestamp]

    weather_str = (
        str(weather_dict["temp"]) + "\n" + str(weather_dict["humid"]) + "\n" +
        str(weather_dict["press"]) + "\n" + str(weather_dict["cloud"]) + "\n" + str(weather_dict["rad"])
    )
    return weather_str, 200
