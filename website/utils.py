import re
import logging
from db import get_cursor
from flask_mail import Mail, Message
from dotenv import load_dotenv
import os
from flask import request, has_request_context
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

logger = logging.getLogger(__name__)

def limiter_key():
    if request.is_json and has_request_context:
        data = request.get_json(silent=True) or {}
        return data.get("device_id") or get_remote_address()
    return get_remote_address()

limiter = Limiter(key_func=limiter_key, default_limits=["200 per day", "50 per hour"])
mail = Mail()

load_dotenv()

MAIL_FROM = os.getenv("MAIL_FROM", os.getenv("MAIL_USERNAME"))

def validate_device_id(device_id):
    return bool(re.fullmatch(r"PVH_[A-F0-9]{12}", device_id))
    
def get_user_devices(user_id):
    with get_cursor() as cursor:
        cursor.execute("""
            SELECT device_id, nickname, max_power, baseline_power, baseline_light
            FROM devices WHERE user_id = %s
        """, (user_id,))
        rows = cursor.fetchall()
    
    return [{"device_id": r['device_id'], "nickname": r['nickname'], "max_power": r['max_power'], "baseline_power": r['baseline_power'], "baseline_light": r['baseline_light']} for r in rows]

def send_reset_email(receiverAddress, resetLink):
    body = f"""Hi,

You requested a password reset for your PhotonVHealth account.

Click the link below to reset your password (valid for 1 hour):
{resetLink}

If you didn't request this, you can safely ignore this email.

— PhotonVHealth
"""
    try:
        msg = Message(
            subject="PhotonVHealth — Password Reset",
            recipients=[receiverAddress],
            body=body,
            sender=("PhotonVHealth", MAIL_FROM)
        )
        mail.send(msg)
    except Exception as e:
        logger.error(f"Failed to send reset email: {e}")
        raise