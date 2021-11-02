#include <Wire.h>
#include <SHT31.h> // https://github.com/RobTillaart/SHT31
#include <timeObj.h> // https://github.com/leftCoast/LC_baseTools

#define RELAY1_PIN 6  //relay 1 pin to activate coil
#define RELAY2_PIN 7  //relay 2 pin to activate coil
#define SHT31_ADDRESS   0x44

uint32_t start;
uint32_t stop;

SHT31 sht;

int open_door_temp = 29.5;
int close_door_temp = 27.5;
bool        relay1State;
bool        relay2State;
timeObj     relayTimer(4300);  // Its in miliseconds. 1000 ms = a second, 60 seconds = minute 5 minutes..
timeObj     loopTimer(1000);        // I guess you want the loop slowed down?

void setup(void) {
   
   Serial.begin(115200);
   Serial.println(__FILE__);
   Serial.print("SHT31_LIB_VERSION: \t");
   Serial.println(SHT31_LIB_VERSION);

   Wire.begin();
   sht.begin(SHT31_ADDRESS);
   Wire.setClock(100000);

   uint16_t stat = sht.readStatus();
   Serial.print(stat, HEX);
   Serial.println();

   // Relay setup and shut off..
   pinMode(RELAY1_PIN, OUTPUT);
   pinMode(RELAY2_PIN, OUTPUT);   
   digitalWrite(RELAY1_PIN, HIGH);
   digitalWrite(RELAY2_PIN, HIGH);  
   relay1State = false;              // Relay is off.
   relay2State = false;              // Relay is off.

   // Fire up your loop slowing timer.
   loopTimer.start();
}


// A function that turns the fan on and off.
bool switchRelay(bool onOff, char relay_pin) {
         
   if (onOff) {                        // If we want the relay on..
      digitalWrite(relay_pin, LOW);    // Fire it up.
      relayTimer.start();              // Start the timer.
   } else {                            // Else, we want it off..
      digitalWrite(relay_pin, HIGH);   // shut it down.
   }
   
   bool relayState = onOff;                 // The relay state is now what they asked for.
   return relayState;
}


void loop(void) {
   sht.read();
   float temp = sht.getTemperature(); // Values for heat and tempature.
  
   // Serial.print is useful if you are using serial monitor on PC and is not required for this code to work 
   Serial.print("\t");
   Serial.print(temp, 1);
   Serial.print("\t");
   Serial.println(temp, 1);
   delay(100);

   if (loopTimer.ding()) {                               // If the loop timer has expired..
      if (!relay1State && !relay2State) {                 // If the fan's off..
         if (temp > open_door_temp) {                    // And we are past our temp..
            relay1State = switchRelay(true, RELAY1_PIN);                           // On with the fan!
         } else if (temp <= open_door_temp) {
            relay2State = switchRelay(true, RELAY2_PIN);
         }
      } else if (relayTimer.ding()) {                    // Else, if the fan IS running, and the timer has expired..
         if (temp > open_door_temp) {                   // And the room has reached ambiant..
            relay1State = switchRelay(false, RELAY1_PIN);                          // Off with the fan!
         } else if (temp <= open_door_temp) {
            relay2State = switchRelay(false, RELAY2_PIN);
         }
      }
      
      loopTimer.start();                                 // Reset your loop timer.
   }
}
