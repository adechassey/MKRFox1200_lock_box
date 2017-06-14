
// -------------------------------------------------------------------------
// This project controls access by updating rights with the Sigfox network
//
// Created: 16.05.2017
// Author: Antoine de Chassey
// Code: https://github.com/AntoinedeChassey/MKRFOX1200_weather_station
// --------------------------------------------------------------------------

#include "SigFox.h"
#include "ArduinoLowPower.h"
#include "Keypad.h"
#include "Servo.h"
#include "SimpleTimer.h"

// Defines & variables
#define DEBUG true                // Set DEBUG to false to disable serial prints
#define SLEEPTIME 15 * 60 * 1000  // Set the delay to 15 minutes (15 min x 60 seconds x 1000 milliseconds)
#define SERVO 5                   // Set the servo attached pin

const byte ROWS = 4;  // Four rows
const byte COLS = 4;  // Four columns
// Define the cymbols on the buttons of the keypads
byte hexaKeys[ROWS][COLS] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};   // Connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9};   // Connect to the column pinouts of the keypad

char password[4]   = {'2', '0', '1', '7'};   // The default password byte array
char input[4]      = {};                     // The input byte array - 4 digits
int counter        = 0;                      // The counter used to fill the array
int timerId;

typedef struct __attribute__ ((packed)) sigfox_message {
        uint8_t lastMessageStatus;
} SigfoxMessage;

// Stub for message which will be sent
SigfoxMessage msg;

// Objects
// Initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
Servo myServo;
SimpleTimer timer;

void setup() {
        if (DEBUG) {
                // We are using Serial1 instead than Serial because we are going in standby
                // and the USB port could get confused during wakeup. To read the DEBUG prints,
                // connect pins 13-14 (TX-RX) to a 3.3V USB-to-serial converter
                Serial1.begin(115200);
                while (!Serial1) {}
        }

        if (!SigFox.begin()) {
                // Something is really wrong, try rebooting
                // Reboot is useful if we are powering the board using an unreliable power source
                // (eg. solar panels or other energy harvesting methods)
                reboot();
        }

        // Send module to standby until we need to send a message
        SigFox.end();

        if (DEBUG) {
                // Enable DEBUG prints and LED indication if we are testing
                SigFox.debug();
        }

        myServo.attach(SERVO);  // attaches the servo on pin 9 to the servo object
        timerId = timer.setInterval(3000, emptyInput);   // Initialize the timer
}

void loop() {
        timer.run();
        char inputKey = customKeypad.getKey();

        if (inputKey) {
                timer.restartTimer(timerId);
                if (counter == sizeof(input)) {
                        counter = 0;
                }
                input[counter] = inputKey;
                counter++;

                Serial.print(password);
                Serial.print(" : ");
                Serial.println(input);

                if (passwordIsValid())
                        open();

                if (counter == sizeof(input)) {
                        Serial.println("Waiting for new input...");
                }
        }

        // // Start the module
        // SigFox.begin();
        // // Wait at least 30ms after first configuration (100ms before)
        // delay(100);
        //
        // // We can only read the module temperature before SigFox.end()
        // t = SigFox.internalTemperature();
        // msg.moduleTemperature = convertoFloatToInt16(t, 60, -60);
        //
        // // Clears all pending interrupts
        // SigFox.status();
        // delay(1);
        //
        // SigFox.beginPacket();
        // SigFox.write((uint8_t*)&msg, 12);
        //
        // msg.lastMessageStatus = SigFox.endPacket();
        //
        // SigFox.end();
        // //Sleep for 15 minutes
        // LowPower.sleep(SLEEPTIME);
}

void reboot() {
        NVIC_SystemReset();
        while (1) ;
}

void emptyInput(){
        memset(input, 0, sizeof input);
        Serial.print(password);
        Serial.print(" : ");
        Serial.println(input);
}

bool passwordIsValid() {
        for(int i=0; i<sizeof(input); i++) {
                if(password[i] != input[i]) {
                        return false;
                }
        }
        return true;
}

void open() {
        myServo.write(180);
        delay(2000);
        lock();
}

void lock() {
        myServo.write(90);
        delay(100);
}
