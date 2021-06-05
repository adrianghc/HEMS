#!/bin/sh
FLASK_APP=energy_production_model_if.py FLASK_ENV=development FLASK_DEBUG=0 LOCATION=wijchen MODEL=../../models/30d-all12-wijchen-[lr].pkl flask run -p 2024
