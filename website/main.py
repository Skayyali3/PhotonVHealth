from flask import Flask, render_template, Response, send_from_directory, session, redirect, request , jsonify
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
        renew_baseline INTEGER DEFAULT 0,
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
    
    cursor.execute("CREATE TABLE IF NOT EXISTS registered_devices (device_id TEXT PRIMARY KEY)")

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

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "GET":
        return render_template("login.html", logged_in=False)
 
    username = request.form.get("username")
    password = request.form.get("password")
    
    if not username or not password:
        return render_template("login.html", logged_in=False, error="Username and password required")
 
    connection = get_db()
    cursor = connection.cursor()
    cursor.execute("SELECT id, password_hashed FROM users WHERE username = ?", (username,))
    user = cursor.fetchone()
    connection.close()
 
    if user and check_password_hash(user[1], password):
        session["user_id"] = user[0]
        session["username"] = username
        return redirect("/dashboard")
 
    return render_template("login.html", logged_in=False, error="Invalid username or password")
 
@app.route("/signup", methods=["GET", "POST"])
def signup():
    if request.method == "GET":
        return render_template("signup.html", logged_in=False)
 
    username = request.form.get("username")
    password = request.form.get("password")
    
    if not username or not password:
        return render_template("signup.html", logged_in=False, error="Username and password required")
 
    hashed = generate_password_hash(password)
 
    connection = get_db()
    cursor = connection.cursor()
 
    try:
        cursor.execute("INSERT INTO users (username, password_hashed) VALUES (?, ?)", (username, hashed))
        connection.commit()
    except sqlite3.IntegrityError:
        connection.close()
        return render_template("signup.html", logged_in=False, error="Username already exists")
 
    connection.close()
    return redirect("/login")

@app.route("/dashboard")
def dashboard():
    if "user_id" not in session:
        return redirect("/")
    devices = get_user_devices(session["user_id"])
    return render_template("dashboard.html", logged_in=True, devices=devices)

@app.route("/graphs")
def graphs():
    if "user_id" not in session:
        return redirect("/")
    
    return render_template("graphs.html", logged_in = True)

@app.route("/devices", methods=["GET", "POST"])
def devices():
    if "user_id" not in session:
        return redirect("/")

    if request.method == "GET":
        return render_template(
            "devices.html",
            logged_in=True,
            devices=get_user_devices(session["user_id"])
        )

    deviceID = request.form.get("device_id", "").strip()
    nickname = request.form.get("nickname", "").strip()
    maxPower = request.form.get("max_power", "").strip()
    isAJAX = request.headers.get("X-Requested-With") == "XMLHttpRequest"

    if not deviceID or not nickname or not maxPower:
        msg = "All fields are required."
        if isAJAX:
            return jsonify(success=False, error=msg), 400
        return render_template("devices.html", logged_in=True, devices=get_user_devices(session["user_id"]), error=msg)

    connection = get_db()
    cursor = connection.cursor()

    cursor.execute(
        "SELECT 1 FROM registered_devices WHERE device_id = ?",
        (deviceID,)
    )
    exists = cursor.fetchone()

    if not exists:
        connection.close()
        msg = "This device is not a shipped/registered product."

        if isAJAX:
            return jsonify(success=False, error=msg), 403

        return render_template(
            "devices.html",
            logged_in=True,
            devices=get_user_devices(session["user_id"]),
            error=msg
        )

    cursor.execute(
        "SELECT 1 FROM devices WHERE device_id = ? AND user_id = ?",
        (deviceID, session["user_id"])
    )
    addedAlready = cursor.fetchone()

    if addedAlready:
        connection.close()
        msg = "You already added this device."

        if isAJAX:
            return jsonify(success=False, error=msg), 409

        return render_template(
            "devices.html",
            logged_in=True,
            devices=get_user_devices(session["user_id"]),
            error=msg
        )

    try:
        cursor.execute("""
            INSERT INTO devices (user_id, device_id, nickname, max_power)
            VALUES (?, ?, ?, ?)
        """, (session["user_id"], deviceID, nickname, maxPower))

        connection.commit()
    except sqlite3.IntegrityError:
        connection.close()
        msg = "A user already owns this device."
        
        if isAJAX:
            return jsonify(success=False, error=msg), 409
        
        return render_template(
            "devices.html",
            logged_in=True,
            devices=get_user_devices(session["user_id"]),
            error=msg
        )
        
    connection.close()

    if isAJAX:
        return jsonify(success=True, device={
            "device_id": deviceID,
            "nickname": nickname,
            "max_power": maxPower
        })

    return redirect("/devices")

@app.route("/devices/<device_id>", methods=["DELETE"])
def delete_device(device_id):
    if "user_id" not in session:
        return jsonify(success=False, error="Unauthorized"), 401
 
    connection = get_db()
    cursor = connection.cursor()
    cursor.execute("DELETE FROM devices WHERE device_id = ? AND user_id = ?",(device_id, session["user_id"]))
    connection.commit()
    deleted = cursor.rowcount
    connection.close()
 
    if deleted:
        return jsonify(success=True)
    return jsonify(success=False, error="Device not found"), 404

@app.route("/devices/<device_id>/renew", methods=["POST"])
def renew_device_baseline(device_id):
    if "user_id" not in session:
        return jsonify(success=False), 401

    connection = get_db()
    cursor = connection.cursor()

    cursor.execute("""
        UPDATE devices
        SET renew_baseline = 1
        WHERE device_id = ? AND user_id = ?
    """, (device_id, session["user_id"]))

    connection.commit()
    connection.close()

    return jsonify(success=True)

@app.route("/api/register_device", methods=["POST"])
def register_device():
    data = request.get_json(silent=True)

    if not data:
        return jsonify(success=False, error="Expected JSON"), 400

    device_id = data.get("device_id")

    if not device_id:
        return jsonify(success=False, error="device_id required"), 400

    connection = get_db()
    cursor = connection.cursor()

    cursor.execute(
        "SELECT 1 FROM registered_devices WHERE device_id = ?",
        (device_id,)
    )
    if not cursor.fetchone():
        connection.close()
        return jsonify(success=False, error="Device not authorized"), 403

    cursor.execute(
        "SELECT 1 FROM devices WHERE device_id = ?",
        (device_id,)
    )
    if cursor.fetchone():
        connection.close()
        return jsonify(success=True, message="Already registered")

    cursor.execute("""
        INSERT INTO devices (user_id, device_id, nickname, max_power)
        VALUES (NULL, ?, ?, ?)
    """, (device_id, "Unclaimed Device", 0))

    connection.commit()
    connection.close()

    return jsonify(success=True, message="Device registered")

@app.route("/api/data", methods=["POST"])
def api_data():
    data = request.get_json(silent=True)
    if not data:
        return jsonify(success=False, error="Expected JSON"), 400
 
    device_id = data.get("device_id")
    if not device_id:
        return jsonify(success=False, error="device_id required"), 400
 
    connection = get_db()
    cursor = connection.cursor()
    
    cursor.execute("""
    SELECT 1 FROM registered_devices WHERE device_id = ?
    """, (device_id,))
    
    if not cursor.fetchone():
        connection.close()
        return jsonify(success=False, error="Unauthorized device"), 403
 
    cursor.execute("""
        SELECT max_power, baseline_power, baseline_light
        FROM devices WHERE device_id = ?
    """, (device_id,))
    row = cursor.fetchone()
 
    if not row:
        connection.close()
        return jsonify(success=False, error="Unknown device"), 404
 
    max_power_hw, baseline_power, baseline_light = row
 
    power     = float(data.get("power", 0))
    light     = float(data.get("light", 0))
    light_pct = float(data.get("percentage", 0))
    temp      = float(data.get("temp", 0))
    efficiency = float(data.get("efficiency", 0))

    health = 0.0
    if max_power_hw and max_power_hw > 0 and baseline_power and baseline_power > 0:
        health = min((baseline_power / max_power_hw) * 100.0, 100.0)

    if power > (baseline_power or 0) * 1.1 and light > 600 and temp < 35:
        cursor.execute("""
            UPDATE devices SET baseline_power = ?, baseline_light = ?
            WHERE device_id = ?
        """, (power, light, device_id))
 
    cursor.execute("""
        INSERT INTO sensor_data (device_id, power, light, light_percentage, temp, efficiency, health)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    """, (device_id, power, light, light_pct, temp, efficiency, health))
 
    connection.commit()
    connection.close()
 
    return jsonify(success=True, health=round(health, 1))
 
@app.route("/api/latest/<device_id>")
def api_latest(device_id):
    if "user_id" not in session:
        return jsonify(success=False, error="Unauthorized"), 401
 
    connection = get_db()
    cursor = connection.cursor()
 
    cursor.execute(
        "SELECT nickname FROM devices WHERE device_id = ? AND user_id = ?",
        (device_id, session["user_id"])
    )
    device = cursor.fetchone()
    if not device:
        connection.close()
        return jsonify(success=False, error="Not found"), 404
 
    cursor.execute("""
        SELECT power, light_percentage, temp, efficiency, health, recorded_at
        FROM sensor_data WHERE device_id = ?
        ORDER BY recorded_at DESC LIMIT 1
    """, (device_id,))
    row = cursor.fetchone()
    connection.close()
 
    if not row:
        return jsonify(success=True, data=None)
 
    return jsonify(success=True, data={
        "power":      row[0],
        "light":      row[1],
        "temp":       row[2],
        "efficiency": row[3],
        "health":     row[4],
        "recorded_at": row[5],
    })
 
@app.route("/api/commands/<device_id>")
def api_commands(device_id):
    connection = get_db()
    cursor = connection.cursor()

    cursor.execute("""
        SELECT baseline_power, baseline_light, renew_baseline
        FROM devices
        WHERE device_id = ?
    """, (device_id,))

    row = cursor.fetchone()

    if not row:
        connection.close()
        return jsonify(success=False), 404
    
    renew = bool(row[2])
    
    if renew:
        cursor.execute("""
        UPDATE devices SET renew_baseline = 0 
        WHERE device_id = ?
        """, (device_id,))
        connection.commit()
        
    connection.close()

    return jsonify(
        success=True,
        renew_baseline=renew,
        baseline_power=row[0] or 0,
        baseline_light=row[1] or 0
    )

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
