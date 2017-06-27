
// -------------------------------------------------------------------------
// This project controls access by updating rights with the Sigfox network
//
// Created: 16.05.2017
// Author: Antoine de Chassey
// Code: https://github.com/AntoinedeChassey/MKRFox1200_lock_box
// --------------------------------------------------------------------------

#include "SigFox.h"
#include "ArduinoLowPower.h"
#include "Keypad.h"
#include "Servo.h"
#include "SimpleTimer.h"

// Defines & variables
#define DEBUG true                // Set DEBUG to false to disable serial prints
// Pins
#define SERVO_PIN   5
#define BUZZER_PIN  4
#define RED_PIN     12
#define GREEN_PIN   11
#define BLUE_PIN    10

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
unsigned long previousMillis = 0;            // Will store last updated time
const long interval = 1000 * 60 * 60 * 6;    // Interval at which to ask for a new password (every 6 hours)

typedef union
{
        float voltage;
        uint8_t bytes[4]; // Float - Little Endian (DCBA)
} FLOATUNION_t;

// Objects
// Initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
Servo myServo;
SimpleTimer timer;
RTCZero rtc;

void setup() {
        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(RED_PIN, OUTPUT);
        pinMode(GREEN_PIN, OUTPUT);
        pinMode(BLUE_PIN, OUTPUT);

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

        timerId = timer.setInterval(3000, emptyInputBuffer); // Initialize the timer to executre the emptyInputBuffer function every X seconds

        lock(); // Make sure to lock the box
}

void loop() {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
                // save the last time you read the sensor
                previousMillis = currentMillis;
                getPasswordBySigfox();
        }

        timer.run();
        char inputKey = customKeypad.getKey();

        if(inputKey) {
                switch (inputKey) {
                case '*':
                        lock();
                        blinkRedLED();
                        break;
                case '#':
                        password[0] = '2';
                        password[1] = '0';
                        password[2] = '1';
                        password[3] = '7';
                        tone(BUZZER_PIN, 5000, 50);
                        delay(100);
                        tone(BUZZER_PIN, 2000, 50);
                        delay(100);
                        tone(BUZZER_PIN, 2000, 50);
                        delay(100);
                        tone(BUZZER_PIN, 5000, 50);
                        break;
                default:
                        // Buzz the buzzer
                        tone(BUZZER_PIN, 1000, 100);
                        // Restart the timer
                        timer.restartTimer(timerId);
                        // Disable the timer because someone is typing
                        timer.disable(timerId);
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
                                // Empty buffer
                                emptyInputBuffer();
                                blinkRedLED();
                        }
                        // Enable the timer back
                        timer.enable(timerId);
                }
        }
}

void setLEDColor(int red, int green, int blue) {
        analogWrite(RED_PIN, red);
        analogWrite(GREEN_PIN, green);
        analogWrite(BLUE_PIN, blue);
}

void blinkRedLED(){
        // BLink the LED in RED
        setLEDColor(0, 0, 0); // off
        delay(100);
        setLEDColor(255, 0, 0); // red
        delay(100);
        setLEDColor(0, 0, 0); // off
        delay(100);
        setLEDColor(255, 0, 0); // red
}

void reboot() {
        NVIC_SystemReset();
        while (1) ;
}

void emptyInputBuffer() {
        memset(input, 0, sizeof input);
        counter = 0;
        if(DEBUG) {
                Serial.print(password);
                Serial.print(" : ");
                Serial.println(input);
        }
}

bool passwordIsValid() {
        for(u_int i=0; i<sizeof(input); i++) {
                if(password[i] != input[i]) {
                        return false;
                }
        }
        return true;
}

void open() {
        myServo.attach(SERVO_PIN);
        digitalWrite(SERVO_PIN, HIGH);
        myServo.write(110);
        delay(500);
        digitalWrite(SERVO_PIN, LOW);
        myServo.detach(); // detach servo to stop it working
        emptyInputBuffer();
        setLEDColor(0, 255, 0); // green
        sendAlertBySigfox();
}

void lock() {
        // Buzz the buzzer
        tone(BUZZER_PIN, 5000, 50);
        delay(100);
        tone(BUZZER_PIN, 5000, 50);
        myServo.attach(SERVO_PIN);
        digitalWrite(SERVO_PIN, HIGH);
        myServo.write(35);
        delay(500);
        digitalWrite(SERVO_PIN, LOW);
        myServo.detach(); // detach servo to stop it working
        emptyInputBuffer();
        setLEDColor(255, 0, 0);  // red
}

void getPasswordBySigfox() {
        Serial.println("Fetching new password with Sigfox!");

        // Disable the timer
        //timer.disable(timerId);
        // Set the LED to blue
        setLEDColor(0, 0, 255);

        // Read the input on analog pin 0:
        int sensorValue = analogRead(ADC_BATTERY);
        // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.7V, 3.7v 3Ah):
        float voltage = sensorValue * (3.7 / 400);
        FLOATUNION_t myFloat;
        myFloat.voltage = voltage;
        if (DEBUG) {
                Serial.println("Battery estimation hexadecimal 4 bytes float:");
                for (u_int i=0; i<sizeof(myFloat.bytes); i++)
                {
                        Serial.print(myFloat.bytes[i], HEX); // Print the hex representation of the float
                        Serial.print(' ');
                }
                Serial.println();
        }

        // Start the module
        SigFox.begin();
        // Wait at least 30mS after first configuration (100mS before)
        delay(100);
        // Clears all pending interrupts
        SigFox.status();
        delay(1);

        SigFox.beginPacket();
        SigFox.write(myFloat.bytes, sizeof(myFloat.bytes));

        int ret = SigFox.endPacket(true); // send buffer to SIGFOX network and wait for a response
        if(ret > 0) {
                Serial.println("No transmission");
        } else {
                Serial.println("Transmission ok");
        }

        Serial.println(SigFox.status(SIGFOX));
        Serial.println(SigFox.status(ATMEL));

        if(SigFox.parsePacket()) {
                Serial.println("Response from server:");
                int counter = 0;
                while(SigFox.available()) {
                        // Serial.print("0x");
                        // Serial.println((char) SigFox.read(), HEX);
                        char incomingChar = char(SigFox.read());
                        if(counter < 4 && incomingChar != ' ') {
                                Serial.println(incomingChar);
                                password[counter] = incomingChar;
                        } else {
                                SigFox.read();
                        }
                        counter++;
                }
        } else {
                Serial.println("Could not get any response from the server");
                Serial.println("Check the SigFox coverage in your area");
                Serial.println("If you are indoor, check the 20dB coverage or move near a window");
        }
        Serial.println();

        SigFox.end();

        // Make sure the box is locked with the new password
        lock();
        // Enable the timer back
        //timer.enable(timerId);
}

void sendAlertBySigfox() {
        Serial.println("Sending alert by Sigfox!");
        // Start the module
        SigFox.begin();
        // Wait at least 30mS after first configuration (100mS before)
        delay(100);
        // Clears all pending interrupts
        SigFox.status();
        delay(1);

        SigFox.beginPacket();
        SigFox.write("OPEN", 4);


        int ret = SigFox.endPacket();
        if(ret > 0) {
                Serial.println("No transmission");
        } else {
                Serial.println("Transmission ok");
        }

        Serial.println(SigFox.status(SIGFOX));
        Serial.println(SigFox.status(ATMEL));
        SigFox.end();
}
