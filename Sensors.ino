#include <Arduino.h>
#include <LiquidCrystal.h>
#include "avr/sleep.h"

/**
 * using LCD library
 * Columns: 16 (0 - 15)
 * Rows: 2 (0 - 1)
 * GND/1
 * Vdd/2
 * V0/3 -> POTI || GND
 * RS/4 -> 13
 * R/W/5 -> GND
 * E/6 -> 12
 * DB4/11 -> 11
 * DB5/12 -> 10
 * DB6/13 -> 9
 * DB7/14 -> 8
 * A/15 -> R220 -> Vdd
 * K/16 -> GND
 */

#define HORN A0                    //horn output (used as digital output)
#define TEMPSENSOR A1              //temperatur sensor
#define FUELSENSOR A2              //fuel sender / sensor
#define BREAKFLUID_WARNLIGHT A3    //break fluid warning light (used as digital output)
#define DISPLAY_LED_POWER A5       //display led power
#define BTN_FAN 2                  //fan enable button
#define IGNITION_ON 3                //power on key switched
#define FAN 4                      //fan on/off control
#define BTN_BREAKFLUID_WARNLIGHT 5 //button to enable break fluid warn light
#define BTN_HORN 6                 //button to enable horn
#define FUEL_WARN_LAMP 7           //fuel warn lamp

#define FAN_ON_TEMP 95             //temperatur to turn on fan [°C]
#define FAN_OFF_TEMP 90            //temperatur to turn off fan [°C]
#define FUEL_WARN_THRESHOLD 5      //number of display digits before warning lamp is turned on
#define ABSZERO 273.15
#define MODULO 5
#define SLEEP_TIME 60000           //delay before Atmega goes to sleep 1 minute
#define DELAY 100

LiquidCrystal lcd(13, 12, 11, 10, 9, 8); //define the LCD Pins
const int numReadings = 5;

float temperaturSensorValue=0.0;
int fuelSensorValue = 0;
int readBtnFan = 0;
int readBtnHorn = 0;
int readBtnBreak = 0;

int temperatur = 0;
int prevTemperatur = 0;
long fuelLevel = 0;
long prevFuelLevel = 0;
long time = 0;
long powerOffTime = 0;
int enableSleep = 0;
int fanOn = 0;
int count = 0;

//fuel symbol
byte fuelSymbol[8] = {
        0b11100,
        0b10110,
        0b10111,
        0b10101,
        0b11101,
        0b11101,
        0b11111,
        0b11100
};

//every second dot left off to symbolize a low level
byte mediumChar[8] = {
        0b10101,
        0b01010,
        0b10101,
        0b01010,
        0b10101,
        0b01010,
        0b10101,
        0b01010
};

/**
 * wakeup routine
 */
void wakeUp() {
    enableSleep = 0;
}

/**
 * send atmega to power off mode
 */
void gotoSleep() {
    //turn stuff off
    digitalWrite(HORN, LOW);
    digitalWrite(BREAKFLUID_WARNLIGHT, LOW);
    digitalWrite(FAN, LOW);

    //go to sleep
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW); //wakeup on button fan turned on
    attachInterrupt(1, wakeUp, HIGH); //wakeup on power turned on

    sleep_mode();
    sleep_disable();
    detachInterrupt(0);
    detachInterrupt(1);
}

/**
 * calculate temperature
 *
 * @return int Temperatur °C
 */
int calculateTemperature(float sensorValue) {
    float T0 = 30.0 + ABSZERO;          // T0 nominal Temp. of NTC = 25°C
    float R0 = 10000.0;                 // R0 nominal resistance at T0;
    float B = 3435;                     // beta value (material constant) from data sheet
    float RV = 2200.0;                  // value series resistor (Vorwiderstandswert)
    float VA_VB = sensorValue / 1023.0; // current sensor value proportion
    float RN = RV * VA_VB / (1 - VA_VB);  // current resistance of NTC sensor

    float t =  T0 * B / (B + T0 * log(RN / R0)) - ABSZERO;
    return int(t);
}


/**
 * write the temp and bargraph to the display
 * 
 * float sensorValue
 */
void handleTempReading(float sensorValue) {

    String text = "";
    temperatur = calculateTemperature(sensorValue);

    //turn fan on
    if (temperatur >= FAN_ON_TEMP && fanOn == 0) {
        fanOn = 1;
        digitalWrite(FAN, HIGH);
    } else if (temperatur < FAN_OFF_TEMP && readBtnFan == HIGH) {
        fanOn = 0;
        digitalWrite(FAN, LOW);
    }

    if (temperatur != prevTemperatur) {
        prevTemperatur = temperatur;
        lcd.setCursor(0, 1); //second row
        lcd.print("                ");
        lcd.setCursor(0, 1);

        if (temperatur >= 100) {
            text = "T" + String(temperatur);
        } else if (temperatur <= 9) {
            text = "T00" + String(temperatur);
        } else {
            text = "T0" + String(temperatur);
        }
        lcd.print(text);

        if (temperatur >= 90) {
            for (unsigned int i = 0; i <= temperatur - 90; i++) {
                if (i >= 6)
                    break;
                lcd.setCursor(i + 4, 1);
                lcd.print('\377');
            }
        }
        if (temperatur >= 96) {
            for (unsigned int i = 0; i <= temperatur - 96; i++) {
                if (i >= 6)
                    break;
                lcd.setCursor(i + 10, 1);
                lcd.write(byte(1));
            }
        }
    }
}

/**
 * write the Fuel bargraph
 * 
 * int sensorValue analog reading from the pin
 */
void handleFuelReading(int sensorValue) {

    //map the value to the lcd fields
    fuelLevel = map(sensorValue, 130, 650, 15, 0);

    if (fuelLevel != prevFuelLevel) {
        prevFuelLevel = fuelLevel;
        //reset lcd row
        lcd.setCursor(1, 0);
        lcd.print("               ");

        //write the first 5 fields with 'mediumChar'
        for (unsigned int i = 1; i <= fuelLevel; i++) {
            lcd.setCursor(i, 0);
            lcd.write(byte(1));
            if (i >= 5)
                break;
        }

        //write the last fields
        for (unsigned int i = 1; i <= fuelLevel - 5; i++) {
            lcd.setCursor(i + 5, 0);
            lcd.print('\377');
        }
    }

    //enable the fuel warn lamp
    if (fuelLevel <= FUEL_WARN_THRESHOLD) {
        //start blinking
        digitalWrite(FUEL_WARN_LAMP, HIGH);
        if (fuelLevel <= 2 && count % MODULO == 0) {
            digitalWrite(FUEL_WARN_LAMP, LOW);
        }
    } else {
        digitalWrite(FUEL_WARN_LAMP, LOW);
    }
}

/**
 * handle break fluid warning lamp
 * turns the break fluid warn lamp on/off
 * 
 * int state current state of the pin HIGH / LOW
 */
void handleBreakFluidWarnlamp(int state) {
    if (state == LOW) {
        digitalWrite(BREAKFLUID_WARNLIGHT, HIGH);
    } else {
        digitalWrite(BREAKFLUID_WARNLIGHT, LOW);
    }
}

/**
 * handle fan button
 * turns the cooler fan on/off
 *
 * int state current state of the pin HIGH / LOW
 */
void handleFan(int state) {
    if (state == LOW) {
        digitalWrite(FAN, HIGH);
    } else if (state == HIGH && fanOn == 0){
        digitalWrite(FAN, LOW);
    }
}

/**
 * handle Horn
 * turns the horn on/off 
 *
 * int state current state of the pin HIGH / LOW
 */
void handleHorn(int state) {
    if (state == LOW) {
        digitalWrite(HORN, HIGH);
    } else {
        digitalWrite(HORN, LOW);
    }
}

/**
 * handle backlight of display
 * 
 * int ignitionState the ignition power state (ignition on/off)
 */
void handleDisplayBacklight(int ignitionState) {
    switch (ignitionState) {
        case 1:
            digitalWrite(DISPLAY_LED_POWER, HIGH);
            break;
        default:
            digitalWrite(DISPLAY_LED_POWER, LOW);
    }
}

/**
 * setup routine
 */
void setup() {
    //Serial.begin(9600);
    //Display setup
    lcd.begin(16, 2); //setup Display 16 columns, 2 rows
    lcd.createChar(0, fuelSymbol); //init fuel char
    lcd.createChar(1, mediumChar); //init mediumChar
    //Init first row and write Fuel Symbol
    lcd.setCursor(0, 0); //first row
    lcd.write(byte(0)); //write fuel symbol

    pinMode(DISPLAY_LED_POWER, OUTPUT);
    pinMode(FAN, OUTPUT);
    pinMode(FUEL_WARN_LAMP, OUTPUT);
    pinMode(HORN, OUTPUT);
    pinMode(BREAKFLUID_WARNLIGHT, OUTPUT);
    
    pinMode(IGNITION_ON, INPUT); //setup power wakeup pin = interrupt enabeld

    pinMode(BTN_FAN, INPUT); //btn fan
    digitalWrite(BTN_FAN, HIGH); //enable internal pullup
    
    pinMode(BTN_BREAKFLUID_WARNLIGHT, INPUT);
    digitalWrite(BTN_BREAKFLUID_WARNLIGHT, HIGH); //enable internal pullup
    
    pinMode(BTN_HORN, INPUT);
    digitalWrite(BTN_HORN, HIGH); //enable internal pullup

    pinMode(TEMPSENSOR, INPUT);
}

/**
 * main loop
 */
void loop() {
    time = millis();
    count++;

    //check power mode and sleep mode to enable sleep
    int ignitionState = digitalRead(IGNITION_ON);
    if (ignitionState == LOW && enableSleep == 1) {
        if (time > (powerOffTime + SLEEP_TIME)) {
            gotoSleep();
        }
    } else if (ignitionState == HIGH && enableSleep == 1) {
        enableSleep = 0;
    }

    handleDisplayBacklight(ignitionState);
    
    //start reading
    fuelSensorValue = analogRead(FUELSENSOR);
    temperaturSensorValue = analogRead(TEMPSENSOR);

    //read button states
    readBtnFan = digitalRead(BTN_FAN);
    readBtnHorn = digitalRead(BTN_HORN);
    readBtnBreak = digitalRead(BTN_BREAKFLUID_WARNLIGHT);

    //handle button actions
    handleBreakFluidWarnlamp(readBtnBreak);
    handleFan(readBtnFan);
    handleHorn(readBtnHorn);

    //handle sensor values
    handleTempReading(temperaturSensorValue);
    handleFuelReading(fuelSensorValue);

    //handle power off mode
    if (ignitionState == LOW && enableSleep == 0 && readBtnFan == HIGH) {
        powerOffTime = time;
        enableSleep = 1;
    }

    delay(DELAY);
}

