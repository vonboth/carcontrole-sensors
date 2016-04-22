#include <Arduino.h>
#include <LiquidCrystal/src/LiquidCrystal.h>


//TODO: Öldruckschalter

/**
 * using LCD library
 * Columns: 16 (0 - 15)
 * Rows: 2 (0 - 1)
 *
 */

#define FUELSENSOR A0
#define TEMPSENSOR A1
#define FANSWITCH 7
#define FUEL_WARN_LAMP 8
#define TEMP_WARN_LAMP 9
#define ENGINEON 0
#define FUEL_WARN_THRESHOLD 200
#define FAN_ON_THRESHOLD 200
#define TEMP_WARN_THRESHOLD 400

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); //define the LCD pins (MOSI, MISO required)
int engineState = 0;
int tempSensorValue = 0;
int fuelSensorValue = 0;

/**
 * write the temp bargraph
 */
void handleTempReading(int sensorValue) {

    lcd.setCursor(0, 1); //second row
    lcd.print('C°:');

    /**
     * turn fan on when temp exceeds level
     */
    if (sensorValue > FAN_ON_THRESHOLD) {
        digitalWrite(FANSWITCH, HIGH);
    } else {
        digitalWrite(FANSWITCH, LOW);
    }

    /**
     * turn on a warning lamp when temp exceeds level
     */
    if (sensorValue > TEMP_WARN_THRESHOLD) {
        digitalWrite(TEMP_WARN_LAMP, HIGH);
    } else {
        digitalWrite(TEMP_WARN_LAMP, LOW);
    };
}

/**
 * write the Fuel bargraph
 */
void handleFuelReading(int sensorValue) {

    lcd.setCursor(0, 0); //first row
    lcd.print('FU:');

    //enable the warnlamp
    if (sensorValue < FUEL_WARN_THRESHOLD) {
        digitalWrite(FUEL_WARN_LAMP, HIGH);
    } else {
        digitalWrite(FUEL_WARN_LAMP, LOW);
    }
}

void setup() {
    lcd.begin(16, 2); //setup Display 16 columns, 2 rows
    pinMode(FANSWITCH, OUTPUT);
    pinMode(TEMP_WARN_LAMP, OUTPUT);
    pinMode(FUEL_WARN_LAMP, OUTPUT);
    pinMode(ENGINEON, INPUT);
}

void loop() {
    fuelSensorValue = analogRead(FUELSENSOR);
    tempSensorValue = analogRead(TEMPSENSOR);

    handleTempReading(tempSensorValue);
    handleFuelReading(fuelSensorValue);
}