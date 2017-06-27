# -*- coding: utf8-*-
'''
Created on 19.06.2017
@author: Antoine de Chassey
@project: MKRFox1200 Lock Box
@URL: https://github.com/AntoinedeChassey/MKRFox1200_lock_box
'''

import os
import atexit
import random
import binascii
from flask import Flask, render_template, request, json
# from apscheduler.scheduler import Scheduler
# from test.test_string_literals import byte
# from Crypto.Cipher import AES
# from click.decorators import password_option

app = Flask(__name__)

# Encryption
# encryption_suite = AES.new(b'This is a key123', AES.MODE_CFB, b'This is an IV456')

deviceId = "" # Your device ID, check it out on https://backend.sigfox.com/
password = "2017" + "0000" # MUST be 8 bytes long (Sigfox downlink - https://backend.sigfox.com/apidocs/callback)

# # Password generation scheduler
# cron = Scheduler(daemon=True)
# # Explicitly kick off the background thread
# cron.start()
# @cron.interval_schedule(hours=2)
def generatePassword():
    global password
    password = ''.join(random.choice('#ABCD0123456789') for _ in range(4))
    # Fill the rest of the password with blank characters (Sigfox downlink message MUST be 8 bytes)
    password += "0000"
    print('Generated password: ' + password)

# Shutdown your cron thread if the web process is stopped
# atexit.register(lambda: cron.shutdown(wait=False))
    
@app.route('/')
def hello():
    return 'The current password is: ' + password[0:4]

@app.route('/getPassword', methods=['GET', 'POST'])
def getPassword():
    if request.method == 'POST':
        generatePassword()
        print(request.get_json(silent=True))
    bytesPassword = str.encode(password)
    hexPassword = str(binascii.hexlify(bytesPassword), 'ASCII')
    print("Hex message: " + hexPassword)
    
#     encryptedBytesPassword = encryption_suite.encrypt(bytesPassword)
#     print(encryptedBytesPassword)
    
    return json.dumps({deviceId : { "downlinkData" : hexPassword}})

if __name__=="__main__":
    app.run()