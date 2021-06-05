from flask import Flask, request

app = Flask(__name__)

@app.route('/')
def get_energy_production():
    time = request.args.get("time")
    if time == "999901010000":
        return "Tomato juice", 200
    else:
        return "9.6", 200
