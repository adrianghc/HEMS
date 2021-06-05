#!/bin/sh
FLASK_APP=collection/weather_provider/weather_provider.py FLASK_ENV=development FLASK_DEBUG=0 flask run -p 5010
