from flask import Flask, render_template, send_from_directory, Response, jsonify, request
from datetime import datetime
import os
import sqlite3

def get_db():
    return sqlite3.connect("website/data/PhotonVHealth.db")

def init_db():
    connection = get_db()
    cursor = connection.cursor()
    
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS sensor_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id TEXT,
        power REAL,
        light REAL,
        light_intensity REAL,
        temperature REAL,
        efficiency REAL,
        health REAL,
        created_at TEXT
    )
    """)
    
    connection.commit()
    connection.close()
    
init_db()

app = Flask(__name__)

@app.route('/')
def index():
  return render_template("index.html")

latestData = {}

@app.route('/api/data', methods=['POST'])
def receive_data():
    global latestData
    
    data = request.json
    latestData = data
    
    connection = get_db()
    cursor = connection.cursor()
    
    cursor.execute("""
    INSERT INTO sensor_data (
        device_id, power, light, light_intensity, 
        temperature, efficiency, health, created_at
    )
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        data.get("device_id"), 
        data.get("power"),
        data.get("light"),
        data.get("light_intensity"),
        data.get("temperature"),
        data.get("efficiency"),
        data.get("health"),
        datetime.now().isoformat()
        ))
    
    connection.commit()
    connection.close()
    
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
