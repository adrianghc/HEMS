# Copyright (c) 2020-2021 Adrian Georg Herrmann

import os
from flask import Flask, request
import xml.etree.ElementTree as ET

import numpy as np
import pandas as pd

import pickle

from datetime import datetime
import time

import tensorflow as tf
from keras import backend as K


app = Flask(__name__)
app.config["location"] = os.environ.get("LOCATION")

xml_begin = "<boost_serialization signature=\"serialization::archive\" version=\"17\">\n"

energy_max = {
    "berlin": 49,
    "wijchen": 291
}

locations = {
    "berlin": ["52.4652025", "13.3412466"],
    "wijchen": ["51.8235504", "5.7329005"]
}

def r2(y_true, y_pred):
    SS_res =  K.sum(K.square(y_true - y_pred))
    SS_tot = K.sum(K.square(y_true - K.mean(y_true)))
    return 1 - SS_res/(SS_tot + K.epsilon())

# Load model into memory
if os.environ.get("MODEL").endswith(".h5"):
    app.config["model"] = "nn"
    model = tf.keras.models.load_model(os.environ.get("MODEL"), custom_objects={"r2": r2})
elif os.environ.get("MODEL").endswith(".pkl"):
    app.config["model"] = "lr"
    with open(os.environ.get("MODEL"), "rb") as file:
        model = pickle.load(file)
elif os.environ.get("MODEL").endswith(".csv"):
    app.config["model"] = "test"
    with open(os.environ.get("MODEL"), "r") as file:
        model = file.read()


def get_julian_day(time):
    if time.month > 2:
        y = time.year
        m = time.month
    else:
        y = time.year - 1
        m = time.month + 12
    d = time.day + time.hour / 24 + time.minute / 1440 + time.second / 86400
    b = 2 - np.floor(y / 100) + np.floor(y / 400)

    jd = np.floor(365.25 * (y + 4716)) + np.floor(30.6001 * (m + 1)) + d + b - 1524.5
    return jd

def get_angle(time, latitude, longitude):
    # Source: 
    # https://de.wikipedia.org/wiki/Sonnenstand#Genauere_Ermittlung_des_Sonnenstandes_f%C3%BCr_einen_Zeitpunkt

    # 1. Eclipctical coordinates of the sun

    # Julian day
    jd = get_julian_day(time)

    n = jd - 2451545

    # Median ecliptic longitude of the sun<
    l = np.mod(280.46 + 0.9856474 * n, 360)

    # Median anomaly
    g = np.mod(357.528 + 0.9856003 * n, 360)

    # Ecliptic longitude of the sun
    lbd = l + 1.915 * np.sin(np.radians(g)) + 0.01997 * np.sin(np.radians(2*g))


    # 2. Equatorial coordinates of the sun

    # Ecliptic
    eps = 23.439 - 0.0000004 * n

    # Right ascension
    alpha = np.degrees(np.arctan(np.cos(np.radians(eps)) * np.tan(np.radians(lbd))))
    if np.cos(np.radians(lbd)) < 0:
        alpha += 180

    # Declination
    delta = np.degrees(np.arcsin(np.sin(np.radians(eps)) * np.sin(np.radians(lbd))))


    # 3. Horizontal coordinates of the sun

    t0 = (get_julian_day(time.replace(hour=0, minute=0, second=0)) - 2451545) / 36525

    # Median sidereal time
    theta_hg = np.mod(6.697376 + 2400.05134 * t0 + 1.002738 * (time.hour + time.minute / 60), 24)

    theta_g = theta_hg * 15
    theta = theta_g + longitude

    # Hour angle of the sun
    tau = theta - alpha

    # Elevation angle
    h = np.cos(np.radians(delta)) * np.cos(np.radians(tau)) * np.cos(np.radians(latitude))
    h += np.sin(np.radians(delta)) * np.sin(np.radians(latitude))
    h = np.degrees(np.arcsin(h))

    return (h if h > 0 else 0)

def normalize(df, energy_max, with_energy=False):
    min = {
        "energy": 0,
        "angles": -90,
        "temp": -50,
        "humid": 0,
        "press": 870,
        "cloud": 0,
        "rad": 0
    }

    max = {
        "energy": energy_max,
        "angles": 90,
        "temp": 50,
        "humid": 100,
        "press": 1085,
        "cloud": 100,
        "rad": 4000
    }

    if with_energy:
        df["energy"] = df["energy"].apply(lambda x: (x - min["energy"]) / (max["energy"] - min["energy"]))
    df["angles"] = df["angles"].apply(lambda x: (x - min["angles"]) / (max["angles"] - min["angles"]))
    df["temp1"] = df["temp1"].apply(lambda x: (x - min["temp"]) / (max["temp"] - min["temp"]))
    df["temp2"] = df["temp2"].apply(lambda x: (x - min["temp"]) / (max["temp"] - min["temp"]))
    df["humid1"] = df["humid1"].apply(lambda x: (x - min["humid"]) / (max["humid"] - min["humid"]))
    df["humid2"] = df["humid2"].apply(lambda x: (x - min["humid"]) / (max["humid"] - min["humid"]))
    df["press1"] = df["press1"].apply(lambda x: (x - min["press"]) / (max["press"] - min["press"]))
    df["press2"] = df["press2"].apply(lambda x: (x - min["press"]) / (max["press"] - min["press"]))
    df["cloud1"] = df["cloud1"].apply(lambda x: (x - min["cloud"]) / (max["cloud"] - min["cloud"]))
    df["cloud2"] = df["cloud2"].apply(lambda x: (x - min["cloud"]) / (max["cloud"] - min["cloud"]))
    df["rad1"] = df["rad1"].apply(lambda x: (x - min["rad"]) / (max["rad"] - min["rad"]))
    df["rad2"] = df["rad2"].apply(lambda x: (x - min["rad"]) / (max["rad"] - min["rad"]))


@app.route('/', methods=['POST'])
def get_energy_production():
    time_ = request.args.get("time")
    if time_ is None:
        return "", 204

    if not app.config["location"] in locations:
        location = locations["wijchen"]
    else:
        location = locations[app.config["location"]]

    data = request.get_data().split(b"\n-----\n")
    weather_raw = data[0]
    energy_production_raw = data[1]

    weather_tree = ET.fromstring(weather_raw.decode("utf-8").split(xml_begin)[1])
    energy_production_tree = ET.fromstring(energy_production_raw.decode("utf-8").split(xml_begin)[1])

    weather = {}
    for child in weather_tree:
        if child.tag in ["count", "item_version"]:
            continue
        else:
            for grandchild in child[1]:
                if grandchild.tag in ["count", "item_version"]:
                    continue
                else:
                    time_node = grandchild.find("msg.time")
                    date = datetime.strptime(time_node.find("ptime_date").find("date").text, "%Y%m%d")
                    date = date.replace(hour = int(time_node.find("ptime_time_duration").find("time_duration_hours").text))
                    date = date.replace(minute = int(time_node.find("ptime_time_duration").find("time_duration_minutes").text))

                    timestamp = int(time.mktime(date.timetuple()))

                    station = int(grandchild.find("msg.station").text)
                    temp = float(grandchild.find("msg.temperature").text)
                    humid = int(grandchild.find("msg.humidity").text)
                    press = float(grandchild.find("msg.pressure").text)
                    cloud = int(grandchild.find("msg.cloud_cover").text)
                    rad = float(grandchild.find("msg.radiation").text)

                    if not weather.get(timestamp) or not weather[timestamp]:
                        weather[timestamp] = {}

                    weather[timestamp][station] = {
                        "temp"  : temp,
                        "humid" : humid,
                        "press" : press,
                        "cloud" : cloud,
                        "rad"   : rad
                    }

    energy_production = {}
    for child in energy_production_tree:
        if child.tag in ["count", "item_version"]:
            continue
        else:
            time_node = child[1].find("msg.time")
            date = datetime.strptime(time_node.find("ptime_date").find("date").text, "%Y%m%d")
            date = date.replace(hour = int(time_node.find("ptime_time_duration").find("time_duration_hours").text))
            date = date.replace(minute = int(time_node.find("ptime_time_duration").find("time_duration_minutes").text))

            timestamp = int(time.mktime(date.timetuple()))

            energy = float(child[1].find("msg.energy").text)

            energy_production[timestamp] = {
                "energy" : energy
            }


    # This is the data for the past week, which would be used by an LSTM model.
    # (LSTM model is currently unimplemented.)

    times1 = list(energy_production.keys())
    weather1_end = list(weather.keys()).index(times1[-1])
    energy1 = [x["energy"] for x in energy_production.values()]
    angles1 = [get_angle(datetime.fromtimestamp(x - 3600), float(location[0]), float(location[1])) for x in times1]
    temp11 = [x[1]["temp"] for x in list(weather.values())[:weather1_end+1]]
    temp12 = [x[2]["temp"] for x in list(weather.values())[:weather1_end+1]]
    humid11 = [x[1]["humid"] for x in list(weather.values())[:weather1_end+1]]
    humid12 = [x[2]["humid"] for x in list(weather.values())[:weather1_end+1]]
    press11 = [x[1]["press"] for x in list(weather.values())[:weather1_end+1]]
    press12 = [x[2]["press"] for x in list(weather.values())[:weather1_end+1]]
    cloud11 = [x[1]["cloud"] for x in list(weather.values())[:weather1_end+1]]
    cloud12 = [x[2]["cloud"] for x in list(weather.values())[:weather1_end+1]]
    rad11 = [x[1]["rad"] for x in list(weather.values())[:weather1_end+1]]
    rad12 = [x[2]["rad"] for x in list(weather.values())[:weather1_end+1]]

    df1 = pd.DataFrame(
        data={
            "time": times1, "energy": energy1, "angles": angles1, "temp1": temp11, "temp2": temp12,
            "humid1": humid11, "humid2": humid12, "press1": press11, "press2": press12,
            "cloud1": cloud11, "cloud2": cloud12, "rad1": rad11, "rad2": rad12,
        },
        columns=[
            "time", "energy", "angles", "temp1", "temp2", "humid1", "humid2", "press1", "press2",
            "cloud1", "cloud2", "rad1", "rad2"
        ]
    )
    df1 = df1.sort_values(by="time", ascending=True)


    # This is data for the next week, which is used by a feed-forward neural network or a linear
    # regression model.

    weather2_begin = weather1_end + 1
    times2 = list(weather.keys())[weather2_begin:]
    angles2 = [get_angle(datetime.fromtimestamp(x - 3600), float(location[0]), float(location[1])) for x in times2]
    temp21 = [x[1]["temp"] for x in list(weather.values())[weather2_begin:]]
    temp22 = [x[2]["temp"] for x in list(weather.values())[weather2_begin:]]
    humid21 = [x[1]["humid"] for x in list(weather.values())[weather2_begin:]]
    humid22 = [x[2]["humid"] for x in list(weather.values())[weather2_begin:]]
    press21 = [x[1]["press"] for x in list(weather.values())[weather2_begin:]]
    press22 = [x[2]["press"] for x in list(weather.values())[weather2_begin:]]
    cloud21 = [x[1]["cloud"] for x in list(weather.values())[weather2_begin:]]
    cloud22 = [x[2]["cloud"] for x in list(weather.values())[weather2_begin:]]
    rad21 = [x[1]["rad"] for x in list(weather.values())[weather2_begin:]]
    rad22 = [x[2]["rad"] for x in list(weather.values())[weather2_begin:]]

    df2 = pd.DataFrame(
        data={
            "time": times2, "angles": angles2, "temp1": temp21, "temp2": temp22,
            "humid1": humid21, "humid2": humid22, "press1": press21, "press2": press22,
            "cloud1": cloud21, "cloud2": cloud22, "rad1": rad21, "rad2": rad22,
        },
        columns=[
            "time", "angles", "temp1", "temp2", "humid1", "humid2", "press1", "press2",
            "cloud1", "cloud2", "rad1", "rad2"
        ]
    )
    df2 = df2.sort_values(by="time", ascending=True)


    # Evaluate model

    if app.config["model"] == "nn":
        energy_max_ = energy_max[app.config["location"]]
        normalize(df1, energy_max_, with_energy=True)
        normalize(df2, energy_max_)
        x = df2.drop(["time", "rad1", "rad2"], axis=1).to_numpy()
        y = model.predict(x).reshape(-1)
        y = [y_ * energy_max_ if y_ * energy_max_ > 0 else 0 for y_ in y]
    elif app.config["model"] == "lr":
        x = df2[["angles", "rad1", "rad2"]].to_numpy()
        y = model.predict(x).reshape(-1)
        y = [y_ if y_ > 0 else 0 for y_ in y]
    elif app.config["model"] == "test":
        y = model

    return str(y).replace(", ", "\n").replace("[", "").replace("]", ""), 200
