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
    raise ValueError("Set a secret key in a .env file and keep the .env file in .gitignore (format: SECRET_KEY=abc123)\nWARNING: replace abc123 with a real randomly generated secret key and make it long")

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

    connection.commit()
    connection.close()

init_db()

@app.route('/')
def login():
  return render_template("login.html")

@app.route('/signup')
def signup():
    if request.method == "GET":
        return render_template("signup.html")
    
    username = request.form.get("username")
    password = request.form.get("password")
    
    hashed = generate_password_hash(password)
    
    connection = get_db()
    cursor = connection.cursor()
    
    try:
        cursor.execute("""
        INSERT INTO users (username, password_hashed) 
        VALUES (?, ?)
        """, (username, hashed))
        
        connection.commit()
    
    except sqlite3.IntegrityError:
        return "Username already exists, please try again"
    
    return redirect("/login")

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
    app.run(port=5000)
