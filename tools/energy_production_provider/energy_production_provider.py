# Copyright (c) 2020-2021 Adrian Georg Herrmann

import os
from flask import Flask, request

import pandas as pd
import time
from datetime import datetime

app = Flask(__name__)
app.config["location"] = os.environ.get("LOCATION")
datapath = os.environ.get("DATAPATH") if os.environ.get("DATAPATH") else "data"

data = {
    "berlin": None,
    "wijchen": None
}

# Read all files into memory
for location in ["berlin", "wijchen"]:
    energy = {}
    dir = os.path.join(os.getcwd(), datapath, location)

    for filename in os.listdir(dir):
        with open(os.path.join(dir, filename), "r") as file:
            for line in file:
                key = datetime.strptime(line.split(";")[0], '%Y-%m-%d %H:%M:%S').timestamp()
                energy[key] = int(line.split(";")[1].strip())

    df = pd.DataFrame(
        data={"time": energy.keys(), "energy": energy.values()},
        columns=["time", "energy"]
    )
    data[location] = df.sort_values(by="time", ascending=True)

# Summarize energy data per hour instead of keeping it per 15 minutes
for location in ["berlin", "wijchen"]:
    times = []
    energy = []

    df = data[location]
    for i, row in data[location].iterrows():
        if row["time"] % 3600 == 0:
            try:
                t4 = row["time"]
                e4 = row["energy"]
                e3 = df["energy"][df["time"] == t4 - 900].values[0]
                e2 = df["energy"][df["time"] == t4 - 1800].values[0]
                e1 = df["energy"][df["time"] == t4 - 2700].values[0]

                times += [t4]
                energy += [e1 + e2 + e3 + e4]
            except:
                pass

    df = pd.DataFrame(data={"time": times, "energy_h": energy}, columns=["time", "energy_h"])
    df = df.sort_values(by="time", ascending=True)
    data[location] = data[location].join(df.set_index("time"), on="time", how="right").drop("energy", axis=1)
    data[location].rename(columns={"energy_h": "energy"}, inplace=True)


@app.route('/')
def get_energy_production():
    time_ = request.args.get("time")
    try:
        timestamp = int(time.mktime(datetime.strptime(time_, "%Y%m%d%H%M").timetuple()))
    except:
        return "", 204

    if not app.config["location"] in ["wijchen", "berlin"]:
        location = "wijchen"
    else:
        location = app.config["location"]

    ret = None
    df = data[location]
    if not df[df.time == timestamp].empty:
        ret = str(df[df.time == timestamp]["energy"].values[0])
    else:
        ret = "0"

    return ret, 200
