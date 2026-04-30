from flask import Blueprint, jsonify, request, session
from db import get_db
from utils import validate_device_id

api = Blueprint("api", __name__, url_prefix="/api")

@api.route("/register_device", methods=["POST"])
def register_device():
    data = request.get_json(silent=True)

    if not data:
        return jsonify(success=False, error="Expected JSON"), 400

    device_id = (data.get("device_id") or "").strip().upper()

    if not device_id:
        return jsonify(success=False, error="device_id required"), 400
    
    if not validate_device_id(device_id):
        return jsonify(success=False, error="Invalid device ID"), 400

    with get_db() as connection:
        cursor = connection.cursor()

        cursor.execute(
            "SELECT 1 FROM devices WHERE device_id = ?",
            (device_id,)
        )
        if cursor.fetchone():
            return jsonify(success=True, message="Already registered")

        cursor.execute("""
            INSERT INTO devices (user_id, device_id, nickname, max_power)
            VALUES (NULL, ?, ?, ?)
        """, (device_id, "Unclaimed Device", 0))

        connection.commit()

    return jsonify(success=True, message="Device registered")

@api.route("/data", methods=["POST"])
def api_data():
    data = request.get_json(silent=True)
    if not data:
        return jsonify(success=False, error="Expected JSON"), 400
 
    device_id = (data.get("device_id") or "").strip().upper()
    
    if not device_id:
        return jsonify(success=False, error="device_id required"), 400
    
    if not validate_device_id(device_id):
        return jsonify(success=False, error="Invalid device ID"), 400
 
    with get_db() as connection:
        cursor = connection.cursor()
    
        cursor.execute("""
            SELECT max_power, baseline_power, baseline_light
            FROM devices WHERE device_id = ?
        """, (device_id,))
        row = cursor.fetchone()
    
        if not row:
            return jsonify(success=False, error="Unknown device"), 404
    
        maxPower, baselinePower, baselineLight = row
    
        power = float(data.get("power", 0))
        light = float(data.get("light", 0))
        lightIntensity = float(data.get("percentage", 0))
        temp = float(data.get("temp", 0))
        efficiency = float(data.get("efficiency", 0))

        health = 0.0
        if maxPower and maxPower > 0 and baselinePower and baselinePower > 0:
            health = min((baselinePower / maxPower) * 100.0, 100.0)

        if power > (baselinePower or 0) * 1.1 and light > 600 and temp < 35:
            cursor.execute("""
                UPDATE devices SET baseline_power = ?, baseline_light = ?
                WHERE device_id = ?
            """, (power, light, device_id))
    
        cursor.execute("""
            INSERT INTO sensor_data (device_id, power, light, light_percentage, temp, efficiency, health)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        """, (device_id, power, light, lightIntensity, temp, efficiency, health))
    
        connection.commit()
 
    return jsonify(success=True, health=round(health, 1))
 
@api.route("/latest/<device_id>")
def api_latest(device_id):
    if "user_id" not in session:
        return jsonify(success=False, error="Unauthorized"), 401
 
    with get_db() as connection:
        cursor = connection.cursor()
 
        cursor.execute(
            "SELECT nickname FROM devices WHERE device_id = ? AND user_id = ?",
            (device_id, session["user_id"])
        )
        device = cursor.fetchone()
        if not device:
            return jsonify(success=False, error="Not found"), 404
    
        cursor.execute("""
            SELECT power, light_percentage, temp, efficiency, health, recorded_at
            FROM sensor_data WHERE device_id = ?
            ORDER BY recorded_at DESC LIMIT 1
        """, (device_id,))
        row = cursor.fetchone()
    
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
 
@api.route("/commands/<device_id>")
def api_commands(device_id):
    with get_db() as connection:
        cursor = connection.cursor()

        cursor.execute("""
            SELECT baseline_power, baseline_light, renew_baseline
            FROM devices
            WHERE device_id = ?
        """, (device_id,))

        row = cursor.fetchone()

        if not row:
            return jsonify(success=False), 404

        renew = bool(row[2])

        if renew:
            cursor.execute("""
            UPDATE devices SET renew_baseline = 0 
            WHERE device_id = ?
            """, (device_id,))
            connection.commit()

    return jsonify(
        success=True,
        renew_baseline=renew,
        baseline_power=row[0] or 0,
        baseline_light=row[1] or 0
    )