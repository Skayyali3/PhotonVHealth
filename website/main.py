from flask import Flask, render_template, send_from_directory, Response, jsonify, request
from datetime import datetime
import os

app = Flask(__name__)

@app.route('/')
def index():
  return render_template("index.html")

latestData = {}

@app.route('/api/data', methods=['POST'])
def receive_data():
    global latestData
    latestData = request.json
    return jsonify({"status": "ok"})

@app.route('/api/latest')
def get_latest():
    return jsonify(latestData)

@app.context_processor
def inject_year():
    return {"current_year": datetime.now().year}

@app.route('/robots.txt')
def robots_txt():
    return send_from_directory(os.getcwd(), 'robots.txt')

@app.route('/sitemap.xml')
def sitemap():
    urls = [{'loc': 'https://example.com', 'lastmod': datetime.now().date()}]
    return Response(render_template('sitemap.xml', urls=urls), mimetype='application/xml')

if __name__ == "__main__":
    app.run(debug = True)
