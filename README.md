# [MKRFox1200](http://www.sigfox.com/) Lock Box

What if you had to store something precious to be picked up by someone you do not know ?
What if you were far away from the box and had to change the password for security reasons ?

This is where Sigfox comes in ! Indeed, Sigfox allows you to use 4 downlink messages per day. The idea is to be able to update the password controlling the opening of the box. Therefore, when someone has finished using it, the box's password will be set with a newly generated one from a web application. The owner can then decide to share the password to whom he wants.

This repository includes:
- the firmware to upload on the MKRFox1200
- the web API generating a new password

A video presentation is available [here](https://www.youtube.com/watch?v=m9f7ZpouoyQ)!

Below is a diagram showing how the system works:
    <p align="center">
        <img src="img/presentation.png?raw=true">
    </p>

## Hardware Requirements

- an [MKRFox1200](http://www.sigfox.com/) board
- a [4x4 membrane keypad](http://www.ebay.com/itm/4-x-4-Matrix-Array-16-Key-Membrane-Switch-Keypad-Keyboard-for-Arduino-AVR-PI-C-/310511616357)
- a [buzzer](http://www.ebay.com/itm/DC-3-12V-110DB-Discontinuous-Beep-Alarm-Electronic-Buzzer-Sounder-LW-/172369764855?epid=1287039987&hash=item282209e1f7:g:J1wAAOSw-CpX-vgf)
- an [RGB led](https://www.adafruit.com/product/159)
- 2x AA batteries or equivalent

## Installation
Clone the repo: `git clone https://github.com/AntoinedeChassey/MKRFox1200_lock_box`

### MKRFox1200
1. [Activate](https://backend.sigfox.com/activate/arduino) your board on the Sigfox Backend _(you can follow __[this](https://www.arduino.cc/en/Guide/MKRFox1200)__ and  __[this](https://www.arduino.cc/en/Tutorial/SigFoxFirstConfiguration)__ tutorials)_
2. Flash the MKRFox1200 with the firmware located in this folder: _MKRFox1200_lock_box/MKRFox1200/__src___ (I used __[PlatformIO](http://platformio.org/)__ a new great IDE for IoT. After being installed on Atom, the folder "MKRFox1200" can be imported with `File>Open Folder...>Select` and the project will be configured automatically with the Arduino core and libraries - defined in the file `platformio.ini`)

### API - ngrok
Now we'll take a look at ngrok as a method of exposing your Python server publicly so that the Sigfox Backend can GET/POST data to it.
This is particularly useful for testing purposes as we do not have to spend time on server configurations.
1. Install ngrok from __[here](https://ngrok.com/download)__
2. Launch ngrok on port 5000:

```bash
$ ngrok http 5000
```
This will expose your server publicly (on port 5000). To double check that this has worked, copy the address that has appeared in your terminal window under "forwarding" and navigate to it using your browser; it should look something like below. Make note of this as we'll need it again later!
    <p align="center">
        <img src="img/ngrok.png">
    </p>
3. Launch the Python script (if on Windows):
```bash
$ cd MKRFox1200_access_control/API/
$ python app.py
```

### Sigfox Backend Callback
1. Log in __[here](https://backend.sigfox.com/auth/login)__
2. Go to <https://backend.sigfox.com/devicetype/list>, click left on your device row and select "Edit"
3. Under "Downlink data", select the `CALLBACK` "Downlink mode"

    <p align="center">
        <img src="img/device.png">
    </p>

4. Now go to the "CALLBACKS" section on the left, select "new" on the top right, select "Custom Callback"
    * Type: `DATA` | `BIDIR`
    * Channel `URL`
    * Url pattern: `http://<YOUR_ngrok_SERVER_ADDRESS>/getPassword`
    * Use HTTP Method: `POST`
    * Content Type: `application/json`
    * Body: _(this will be sent to the API, the "data" variable will hold an approximation of the battery voltage level based on a 3.7V Li-Ion cell as a 4 bytes float)_
    ```javascript
    {
          "device" : "{device}",
          "time" : "{time}",
          "duplicate" : "{duplicate}",
          "snr" : "{snr}",
          "rssi" : "{rssi}",
          "avgSnr" : "{avgSnr}",
          "station" : "{station}",
          "lat" : "{lat}",
          "lng" : "{lng}",
          "seqNumber" : "{seqNumber}",
          "data" : "{data}"
    }
    ```

5. Select "OK" to validate
6. __Tick__ the "Downlink" button to activate the newly created callback, make sure it looks like below

    <p align="center">
        <img src="img/downlink.png">
    </p>

## Usage
- Power up the system
- The default password is `2017`
- The buffer storing the input (keys pressed) is 4 characters long
- This buffer is emptied every 3 seconds and every time there is a new key being pressed
- Press `*` to reset the input or __lock__ the box
- A new generated password will be set every 6h by default, see: `const long interval = 1000 * 60 * 60 * 6` (this means a downlink will be asked every 6h = 4 per day to the [Sigfox](http://www.sigidwiki.com/wiki/SIGFOX) Backend, this respects the [ETSI](http://www.etsi.org/) Standards)
- The new password will be stored in the MKRFox1200 flash memory (this means that if you reboot the board, the default password (2017) is set)
- The new password is accessible on the ngrok Python API

  <p align="center">
      <img width="50%" height="50%" src="img/device.png">
  </p>

__Have fun!__

> *Antoine de Chassey*
