# Use this script to get a secret key for the sessions thing to function correctly

import secrets
KEY = secrets.token_hex(64)
print(KEY)