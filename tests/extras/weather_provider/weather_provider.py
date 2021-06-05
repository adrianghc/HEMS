from flask import Flask, request

app = Flask(__name__)

@app.route('/')
def get_weather():
    time = request.args.get("time")
    if time == "999901010000":
        return "1\n2\n3\nPsych!\nBlub", 200
    else:
        return "1\n2\n3\n4\n5", 200
