from flask import Flask, render_template, Response, send_from_directory, session, redirect, request 
from werkzeug.security import generate_password_hash, check_password_hash
from datetime import datetime
import os
import sqlite3
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)
app.secret_key = os.getenv("SECRET_KEY")

if not app.secret_key:
    raise ValueError("Set a secret key in a .env file and keep the .env file in .gitignore (format: SECRET_KEY=abc123)\nWARNING: replace 'abc123' with the first result from get_secret_key.py")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.join(BASE_DIR, "data", "PhotonVHealth.db")

def get_db():
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
    return sqlite3.connect(DB_PATH)

def init_db():
    connection = get_db()
    cursor = connection.cursor()

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE,
        password_hashed TEXT
    )
    """)
    
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS devices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER,
        device_id TEXT UNIQUE, 
        nickname TEXT, 
        max_power INTEGER,
        baseline_power REAL DEFAULT 0,
        baseline_light REAL DEFAULT 0,
        FOREIGN KEY(user_id) REFERENCES users(id)
    )
    """)
    
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS sensor_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id TEXT,
        power REAL,
        light REAL,
        light_percentage REAL,
        temp REAL,
        efficiency REAL,
        health REAL,
        recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(device_id) REFERENCES devices(device_id)
    )
    """)

    connection.commit()
    connection.close()
    
def get_user_devices(user_id):
    connection = get_db()
    cursor = connection.cursor()
    cursor.execute("""
        SELECT device_id, nickname, max_power, baseline_power, baseline_light
        FROM devices WHERE user_id = ?
    """, (user_id,))
    rows = cursor.fetchall()
    connection.close()
    
    return [{"device_id": r[0],"nickname":  r[1],"max_power": r[2],"baseline_power": r[3],"baseline_light": r[4],} for r in rows]

init_db()

@app.route("/")
def homepage():
    if "user_id" in session:
        return redirect("/dashboard")
    return redirect("/login")

@app.route("/login", methods = ["GET", "POST"])
def login():
    if request.method == "GET":
        return render_template("login.html", logged_in=False)

    username = request.form.get("username")
    password = request.form.get("password")
    
    if not username or not password:
        return "Username and password required"

    connection = get_db()
    cursor = connection.cursor()

    cursor.execute("SELECT id, password_hashed FROM users WHERE username = ?",(username,))

    user = cursor.fetchone()
    connection.close()

    if user and check_password_hash(user[1], password):
        session["user_id"] = user[0]
        session["username"] = username
        return redirect("/dashboard")

    return "Invalid username or password"

@app.route("/signup", methods=["GET", "POST"])
def signup():
    if request.method == "GET":
        return render_template("signup.html", logged_in=False)

    username = request.form.get("username")
    password = request.form.get("password")
    
    if not username or not password:
        return "Username and password required"

    hashed = generate_password_hash(password)

    connection = get_db()
    cursor = connection.cursor()

    try:
        cursor.execute("INSERT INTO users (username, password_hashed) VALUES (?, ?)",(username, hashed))
        connection.commit()

    except sqlite3.IntegrityError:
        connection.close()
        return "Username already exists"

    connection.close()
    return redirect("/login")

@app.route("/dashboard")
def dashboard():
    if "user_id" not in session:
        return redirect("/")
    
    return render_template("dashboard.html", logged_in = True)

@app.route("/graphs")
def graphs():
    if "user_id" not in session:
        return redirect("/")
    
    return render_template("graphs.html", logged_in = True)

@app.route("/devices", methods = ["GET", "POST"])
def devices():
    if "user_id" not in session:
        return redirect("/")
    
    if request.method == "GET":
        return render_template("devices.html", logged_in = True)
    
    deviceId = request.form.get("device_id")
    nickname = request.form.get("nickname")
    maxPower = request.form.get("max_power")
    
    if not deviceId or not nickname or not maxPower:
        return "Missing fields."
    
    connection = get_db()
    cursor = connection.cursor()
    
    try:
        cursor.execute("""
            INSERT INTO devices (user_id, device_id, nickname, max_power)
            VALUES (?, ?, ?, ?)             
        """, (session["user_id"], deviceId, nickname, maxPower))
        
        connection.commit()
    except sqlite3.IntegrityError:
        connection.close()
        return "Device already exists."
    
    connection.close()
    
    return render_template("devices.html", logged_in = True)

@app.route("/logout")
def logout():
    session.clear()
    return redirect("/")

@app.context_processor
def inject_year():
    return {"current_year": datetime.now().year}

@app.route('/robots.txt')
def robots_txt():
    return send_from_directory(os.getcwd(), 'robots.txt')

@app.route('/sitemap.xml')
def sitemap():
    urls = [{'loc': 'https://photonvhealth.onrender.com', 'lastmod': datetime.now().date()}]
    return Response(render_template('sitemap.xml', urls=urls), mimetype='application/xml')

@app.route("/health")
def health():
    return "OK", 200

if __name__ == "__main__":
    port = int(os.getenv("PORT", 5000))
    app.run(host = "0.0.0.0", port = port)
