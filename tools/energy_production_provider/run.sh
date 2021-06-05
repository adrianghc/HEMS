#!/bin/sh
FLASK_APP=energy_production_provider.py FLASK_ENV=development FLASK_DEBUG=0 LOCATION=wijchen DATAPATH=../../data flask run -p 2020
