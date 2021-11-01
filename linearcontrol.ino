/*
  SHT31 Temperature & Humidity Sensor Linear Actuator Control
  modified on 01 Nov 2021
  by Mark Schipper
*/

#include "Wire.h"
#include "SHT31.h"

uint32_t start;
uint32_t stop;

SHT31 sht;

//pin assignment
int relay_1 = 6;  //relay 1 pin to activate coil
int relay_2 = 7;  //relay 2 pin to activate coil

//Actuator door open and close temperature values in celsius - can be changed to suit application needs
int open_door_temp = 29.5;
int close_door_temp = 27.5;

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time relay was updated

// constants won't change:
const long interval = 4300; 

void setup() {
  Serial.begin(115200);
  Wire.begin();

  sht.begin(0x44);    //SHT31 I2C Address

  Wire.setClock(100000);
  uint16_t stat = sht.readStatus();
  Serial.print(stat, HEX);
  Serial.println();
  
  pinMode(relay_1, OUTPUT); //defining relay 1 pin as output
  pinMode(relay_2, OUTPUT); //defining relay 2 pin as output
  
  digitalWrite(relay_1, LOW); //deactivating relay 1
  digitalWrite(relay_2, LOW); //deactivating relay 2
  
}

void loop() {
sht.read();
float h = sht.getHumidity();
float t = sht.getTemperature();

unsigned long currentMillis = millis();
  
//Serial.print is useful if you are using serial monitor on PC and is not required for this code to work 
//Serial.print("Humidity:");
//Serial.print(h, 1);
//Serial.print("\t");
//Serial.print("Temperature:");
//Serial.print(t, 1);
  
  if (t > open_door_temp) {         //if the temperature value is higher than 29C, extend actuator
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      if (relay_1_state == LOW) {
        relay_1_state = HIGH;
      } else {
        relay_1_state = LOW;
      }
      
    digitalWrite(relay_1, relay_1_state);
    digitalWrite(relay_2, LOW);
  }

  else if (t < close_door_temp) {     //if the temperature value is lower than 27.5C, retract actuator
        if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      if (relay_2_state == LOW) {
        relay_2_state = HIGH;
      } else {
        relay_2_state = LOW;
      }
          
    digitalWrite(relay_1, LOW);
    digitalWrite(relay_2, relay_2_state);
  }

  else {
    
  }

  delay(500);  //can be changed based on needs

}
