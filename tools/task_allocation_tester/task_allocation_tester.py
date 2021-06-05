# Copyright (c) 2020-2021 Adrian Georg Herrmann

# %%
import os
import random
import numpy as np
import matplotlib.pyplot as plt

# %%
def generate_step_function():
    energy_weekly = []
    energy_daily = []
    for day in range(7):
        # Choose a random peak value for the day.
        peak = random.randrange(100, 400)
        energy_day = []
        for t in range(24):
            if t < 8 or t >= 21:
                # Energy production is 0 between 20:00 and 8:00.
                y = 0
            elif t in range(8, 14):
                # Move mean towards the peak
                mean = peak/6 * (t+1-8)
                sigma = 20 + 6 * (t-8)
                y = np.random.normal(mean, sigma)
            elif t in range(14, 21):
                # Move mean away from the peak
                mean = peak - peak/6 * (t+1-14)
                sigma = 56 - 6 * (t-14)
                y = np.random.normal(mean, sigma)
            energy_day += [round(y) if y > 0 else 0]
        energy_weekly += energy_day
        energy_daily.append(energy_day)

    return energy_weekly, energy_daily

# %%
# Analyze frequencies of weekly, daily and hourly energy of step functions
# sampled with `generate_step_function()``.

energy_total_weekly = []
energy_total_daily = []
energy_total_hourly = []

for i in range(10000):
    energy_weekly, energy_daily = generate_step_function()
    energy_total_weekly += [sum(energy_weekly)]
    energy_total_daily += [sum(l) for l in energy_daily]
    energy_total_hourly += [x for x in energy_weekly if x > 0]

fig, ax = plt.subplots(3, 1, figsize=(20, 15))
y_weekly, x_weekly, _ = ax[0].hist(energy_total_weekly, bins=20, label="Frequency of weekly\ntotal energy")
ax[0].tick_params(labelsize=18)
ax[0].legend(prop={"size": 18})
ax[1].hist(energy_total_daily, bins=20, label="Frequency of daily\ntotal energy")
ax[1].tick_params(labelsize=18)
ax[1].legend(prop={"size": 18})
y_hourly, x_hourly, _ = ax[2].hist(energy_total_hourly, bins=20, label="Frequency of hourly\ntotal energy (without 0)")
ax[2].tick_params(labelsize=18)
ax[2].legend(prop={"size": 18})

print(x_weekly[y_weekly.argmax()])
print(x_hourly[y_hourly.argmax()])
print(x_hourly.max())

# %%
# Plot step function
energy_weekly, _ = generate_step_function()
plt.figure(figsize=(20, 5))
plt.xticks(range(0, 200, 24))
plt.tick_params(labelsize=18)
plt.plot(energy_weekly, drawstyle="steps-post", label="Randomly sampled step function")
plt.legend(prop={"size": 18})

# %%
# Write step function to file
if os.environ.get("OUTPUT") is not None:
    path = os.environ.get("OUTPUT")
else:
    path = "../../models/step_function_test.csv"

with open(os.path.join(os.getcwd(), path), "w") as file:
    step_function = file.write(str(energy_weekly).replace(", ", "\n").replace("[", "").replace("]", ""))

# %%
class task:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.start_time = -1
        self.end_time = -1

# %%
def generate_task(num_tasks, mean_height, sigma_height):
    tasks = []
    for i in range(num_tasks):
        while True:
            width = np.random.normal(1, 2.5)
            if width >= 0.5:
                width = np.round(width)
                break
        while True:
            height = np.random.normal(mean_height, sigma_height)
            if height >= 0.5:
                height = np.round(height)
                break

        tasks += [task(width, height)]
    return tasks

# %%
# Analyze frequencies of duty cycles, power ratings and energy consumptions of
# appliances sampled with `generate_task()`.

tasks = generate_task(1000, 50, 130)
widths = [x.width for x in tasks]
heights = [x.height for x in tasks]
areas = [x.height * x.width for x in tasks]

fig, ax = plt.subplots(3, 1, figsize=(15, 10))
ax[0].hist(widths, label="Frequency of appliance duty cycles")
ax[0].tick_params(labelsize=18)
ax[0].legend(prop={"size": 18})
ax[1].hist(heights, label="Frequency of appliance power ratings")
ax[1].tick_params(labelsize=18)
ax[1].legend(prop={"size": 18})
ax[2].hist(areas, label="Frequency of task energy consumptions")
ax[2].tick_params(labelsize=18)
ax[2].legend(prop={"size": 18})

# %%
# Analyze frequency of task area sums for appliances sampled with
# `generate_task()`.

areas = []
for i in range(50000):
    tasks = generate_task(20, 20, 100)
    areas += [sum([x.height * x.width for x in tasks])]

plt.figure(figsize=(20, 5))

y, x, _ = plt.hist(areas, bins=20, label="Frequency of task area sums\n(20 tasks, height mean 20, std 100)", color="C2")
plt.tick_params(labelsize=18)
plt.legend(prop={"size": 18})

print(x[y.argmax()])

# %%
def sort_by_width(x):
    return x.width

def sort_by_height(x):
    return x.height

def sort_by_area(x):
    return x.width * x.height
# 
# %%
def first_fit(tasks, step_function):
    for i, x in enumerate(tasks):
        t1 = -1
        t2 = -1

        for t, step in enumerate(step_function):
            if step < x.height:
                t1 = -1
            else:
                if t1 == -1:
                    t1 = t
                if t - t1 == x.width - 1:
                    t2 = t
                    break

        if t1 != -1 and t2 != -1:
            for t in range(t1, t2+1):
                step_function[t] -= x.height
            tasks[i].start_time = t1
            tasks[i].end_time = t2


def next_fit(tasks, step_function):
    for i, x in enumerate(tasks):
        it_begin = 0
        t1 = -1
        t2 = -1

        for t, step in enumerate(step_function[it_begin:]):
            if step < x.height:
                t1 = -1
            else:
                if t1 == -1:
                    t1 = t
                if t - t1 == x.width - 1:
                    t2 = t
                    break

        if t1 == -1 or t2 == -1:
            for t, step in enumerate(step_function[:it_begin]):
                if step < x.height:
                    t1 = -1
                else:
                    if t1 == -1:
                        t1 = t
                    if t - t1 == x.width - 1:
                        t2 = t
                        break

        if t1 != -1 and t2 != -1:
            for t in range(t1, t2+1):
                step_function[t] -= x.height
            tasks[i].start_time = t1
            tasks[i].end_time = t2

        if t2 == len(step_function):
            it_begin = 0
        else:
            it_begin = t2 + 1


def best_fit(tasks, step_function):
    for i, x in enumerate(tasks):
        best_pos = -1
        max_area = 0
        cur_area = 0

        t1 = -1
        t2 = -1

        for t, step in enumerate(step_function):
            if step < x.height:
                cur_area = 0
                t1 = -1
            else:
                if t1 == -1:
                    cur_area += step - x.height
                    t1 = t
                if t - t1 < x.width - 1:
                    cur_area += step - x.height
                elif t - t1 == x.width - 1:
                    cur_area += step - x.height
                    t2 = t

                    if cur_area > max_area:
                        max_area = cur_area
                        best_pos = t1

                    break

        if t1 != -1 and t2 != -1:
            for t in range(t1, t2+1):
                step_function[t] -= x.height
            tasks[i].start_time = best_pos
            tasks[i].end_time = best_pos + x.width - 1

# %%
# Evaluate all combinations of task sorting orders and allocation heuristics.

orders = ["width", "height", "area"]
heuristics = ["first_fit", "next_fit", "best_fit"]

efficacy = {
    "width": {
        "first_fit": [],
        "next_fit": [],
        "best_fit": [],
    },
    "height": {
        "first_fit": [],
        "next_fit": [],
        "best_fit": [],
    },
    "area": {
        "first_fit": [],
        "next_fit": [],
        "best_fit": [],
    }
}

for order in orders:
    for heuristic in heuristics:
        for i in range(5000):
            step_function, _ = generate_step_function()
            original_energy = sum(step_function)
            step_function_old = step_function.copy()

            tasks = generate_task(20, 20, 100)
            total_area = 0
            for x in tasks:
                total_area += x.width * x.height

            if order == "width":
                tasks.sort(key=sort_by_width)
            elif order == "height":
                tasks.sort(key=sort_by_height)
            elif order == "area":
                tasks.sort(key=sort_by_area)

            if heuristic == "first_fit":
                first_fit(tasks, step_function)
            elif heuristic == "next_fit":
                next_fit(tasks, step_function)
            elif heuristic == "best_fit":
                best_fit(tasks, step_function)

            remaining_energy = sum(step_function)
            efficacy[order][heuristic] += [(original_energy - remaining_energy) / min(total_area, original_energy)]

fig, ax = plt.subplots(3, 3, figsize=(20, 15))
y00, x00, _ = ax[0][0].hist(efficacy["width"]["first_fit"], bins=20, label="Desc. width,\nfirst-fit", color="C2")
ax[0][0].tick_params(labelsize=18)
ax[0][0].legend(prop={"size": 18})
y01, x01, _ = ax[0][1].hist(efficacy["width"]["next_fit"], bins=20, label="Desc. width,\nnext-fit", color="C2")
ax[0][1].tick_params(labelsize=18)
ax[0][1].legend(prop={"size": 18})
y02, x02, _ = ax[0][2].hist(efficacy["width"]["best_fit"], bins=20, label="Desc. width,\nbest-fit", color="C2")
ax[0][2].tick_params(labelsize=18)
ax[0][2].legend(prop={"size": 18})
y10, x10, _ = ax[1][0].hist(efficacy["height"]["first_fit"], bins=20, label="Desc. height,\nfirst-fit", color="C2")
ax[1][0].tick_params(labelsize=18)
ax[1][0].legend(prop={"size": 18})
y11, x11, _ = ax[1][1].hist(efficacy["height"]["next_fit"], bins=20, label="Desc. height,\nnext-fit", color="C2")
ax[1][1].tick_params(labelsize=18)
ax[1][1].legend(prop={"size": 18})
y12, x12, _ = ax[1][2].hist(efficacy["height"]["best_fit"], bins=20, label="Desc. height,\nbest-fit", color="C2")
ax[1][2].tick_params(labelsize=18)
ax[1][2].legend(prop={"size": 18})
y20, x20, _ = ax[2][0].hist(efficacy["area"]["first_fit"], bins=20, label="Desc. area,\nfirst-fit", color="C2")
ax[2][0].tick_params(labelsize=18)
ax[2][0].legend(prop={"size": 18})
y21, x21, _ = ax[2][1].hist(efficacy["area"]["next_fit"], bins=20, label="Desc. area,\nnext-fit", color="C2")
ax[2][1].tick_params(labelsize=18)
ax[2][1].legend(prop={"size": 18})
y22, x22, _ = ax[2][2].hist(efficacy["area"]["best_fit"], bins=20, label="Desc. area,\nbest-fit", color="C2")
ax[2][2].tick_params(labelsize=18)
ax[2][2].legend(prop={"size": 18})

print(x00[y00.argmax()])
print(x01[y01.argmax()])
print(x02[y02.argmax()])
print(x10[y10.argmax()])
print(x11[y11.argmax()])
print(x12[y12.argmax()])
print(x20[y20.argmax()])
print(x21[y21.argmax()])
print(x22[y22.argmax()])