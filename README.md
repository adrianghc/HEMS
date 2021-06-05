# HEMS - Home Energy Management System

This is a partial implementation of a **Home Energy Management System** for a residential solar installation. It enables the user to schedule appliances in a targeted way, increasing energy self-consumption based on energy production predictions via weather forecasts. These predictions can achieve R² scores of over 0.9 when using global radiation data, or up to 0.8 when global radiation data is not available. The allocation of tasks can reach perfect efficacy if sufficient energy for all appliances is expected to be available, and an efficacy of up to about 63% allocated energy if total task energy consumption is expected to be as high as 150% of the total available energy budget.

This application was conceptualized and developed in the context of a master's thesis in computer science at Freie Universität Berlin, submitted in June 2021. The thesis manuscript serves as documentation and is available in this repository in the `doc` directory.

This prototype implementation consists of a C++ program for Linux and a set of accompanying Python tools.

## Prerequisites

This project can be opened as a Visual Studio Code workspace that includes convenience functions like launch configurations and tasks for building and tests.

The C++ 14-based core portion of the HEMS program (in `src` and `include`) has the Boost C++ library as the only prerequisite. Builds were produced and tested with Boost version 1.73.0, which can be installed with

```
$ sudo apt install libboost1.73-all-dev
```

on Debian-based distributions.

The Python tools (in `tools`) are based on Python 3.8. Each tool lists its dependencies in a corresponding `requirements.txt` file, which can be installed via

```
$ pip3 install -r requirements.txt
```

## Build

```
$ make
```

## Run

To run the HEMS core:

```
$ cd build/native/hems
$ ./launcher
```

To run a tool, go to its directory and

```
$ ./run.sh
```
