import os
import atexit
import random
import binascii
from apscheduler.scheduler import Scheduler
from flask import Flask, render_template, request, json
from test.test_string_literals import byte
from Crypto.Cipher import AES
from click.decorators import password_option

app = Flask(__name__)

# Password generation scheduler
cron = Scheduler(daemon=True)
# Explicitly kick off the background thread
cron.start()

# Encryption
encryption_suite = AES.new(b'This is a key123', AES.MODE_CFB, b'This is an IV456')

password = '2017'
tempStore = []
dateStore = []

@cron.interval_schedule(hours=2)
def generatePassword():
    global password
    password = ''.join(random.choice('#ABCD0123456789') for _ in range(4))
    print('Generated password: ' + password)
    return password
    
# Shutdown your cron thread if the web process is stopped
atexit.register(lambda: cron.shutdown(wait=False))
    
@app.route('/')
def hello():
    return 'Welcome to Python Flask!'

@app.route('/data/<device>', methods=['POST'])
def addMessage(device):
    content = request.json # grab the json data from the POST request
    time = int(content['time'])
    time = datetime.datetime.fromtimestamp(time).strftime('%Y-%m-%d %H:%M:%S') # convert epoch time to human readable time
    tempStore.append(content['temp'])
    dateStore.append(time)
    if len(tempStore) > 10: # truncate data after 10 elements to avoid large lists
        del tempStore[0]
        del dateStore[0]
    print(tempStore) # print useful info to the debug console
    print(time)
    print(content['device'])
    return ('', 200)

@app.route('/getPassword')
def getPassword():
    bytesPassword = str.encode(password)
    print(bytesPassword)
    encryptedBytesPassword = encryption_suite.encrypt(bytesPassword)
    print(encryptedBytesPassword)
#     hexPassword = str(binascii.hexlify(bytesPassword), 'ASCII')
#     return json.dumps({"18B407" : { "downlinkData" : hexPassword}})
    hexPassword = str(binascii.hexlify(encryptedBytesPassword), 'ASCII')
#     return json.dumps({"18B407" : { "downlinkData" : hexPassword}}
    return json.dumps({"18B407" : { "downlinkData" : "deadbeefcafebabe"}})
if __name__=="__main__":
    app.run()
