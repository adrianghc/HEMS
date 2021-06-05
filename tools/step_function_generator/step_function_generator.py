# Copyright (c) 2020-2021 Adrian Georg Herrmann

import os
import random
import numpy as np

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

# Generate and write step function to file
energy_weekly, _ = generate_step_function()
if os.environ.get("OUTPUT") is not None:
    path = os.environ.get("OUTPUT")
else:
    path = "../../models/step_function_test.csv"

with open(os.path.join(os.getcwd(), path), "w") as file:
    step_function = file.write(str(energy_weekly).replace(", ", "\n").replace("[", "").replace("]", ""))
