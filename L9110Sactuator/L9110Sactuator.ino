#include <Wire.h>
#include <SHT31.h> // https://github.com/RobTillaart/SHT31

#define a1a_PIN 6  //a1a pin to L9110
#define a2a_PIN 7  //a2a pin to L9110
#define SHT31_ADDRESS   0x44

uint32_t start;
uint32_t stop;

SHT31 sht;

int open_door_temp = 30;
int close_door_temp = 28;
bool        a1aState;
bool        a2aState;
float        tempReached;
unsigned long tempReachedMillis; // when temp was reached
unsigned long actuatorTurnedOnAt; // when actuator was turned on
unsigned long turnOffDelay = 5000; // turn off actuator after this. Based on a 7mm/s actuator it should take 4.3secs to open/close.

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

   // L9110 setup and shut off..
   pinMode(a1a_PIN, OUTPUT);
   pinMode(a2a_PIN, OUTPUT);   
   digitalWrite(a1a_PIN, HIGH);
   digitalWrite(a2a_PIN, HIGH);  
   a1aState = true;              // L9110, actuator closed
   a2aState = false;              // L9110, actuator open
}


// A function that turns the actuator on and off.
bool switchL9110(bool onOff, char L9110_pin) {
         
   if (onOff) {                        // If we want the L9110 on..
      digitalWrite(L9110_pin, LOW);    // Fire it up.
   } else {                            // Else, we want it off..
      digitalWrite(L9110_pin, HIGH);   // shut it down.
   }
   
   bool pinState = onOff;                 // The L9110 state is now what they asked for.
   return pinState;
}

void loop(void) {
	// get the time at the start of this loop()
	unsigned long currentMillis = millis(); 
	
	sht.read();
	float temp = sht.getTemperature(); // Values for heat and tempature.
  
	// Serial.print is useful if you are using serial monitor on PC and is not required for this code to work 
	// Serial.print("\t");
	// Serial.print(temp, 1);
	// Serial.print("\t");
	// Serial.print(a1aState, 1);
	// Serial.print("\t");
	// Serial.print(tempReached, 1);
	// Serial.print("\t");
	// Serial.println(a2aState, 1);

	if (tempReached <= open_door_temp && temp > open_door_temp) {
		tempReachedMillis = currentMillis;
		a1aState = switchL9110(true, a1a_PIN);   // Open the actuator!
		a2aState = switchL9110(false, a2a_PIN);
		tempReached = temp;
		if (a1aState) {
			actuatorTurnedOnAt = currentMillis;    // start the timer.
		}
	}
	
	if (tempReached > close_door_temp && temp <= close_door_temp) {
		tempReachedMillis = currentMillis;
		a1aState = switchL9110(false, a1a_PIN);			 
		a2aState = switchL9110(true, a2a_PIN);   // Close the actuator!
		tempReached = temp;
		if (a2aState) {
			actuatorTurnedOnAt = currentMillis;    // start the timer.
		}
	}
	
	if (a1aState) {
		// okay, actuator on, check for how long
		if ((unsigned long)(currentMillis - actuatorTurnedOnAt) >= turnOffDelay) {
			a1aState = false;
			digitalWrite(a1a_PIN, HIGH);   // shut it down.
		}
	}
	
	if (a2aState) {
		// okay, actuator on, check for how long
		if ((unsigned long)(currentMillis - actuatorTurnedOnAt) >= turnOffDelay) {
			a2aState = false;
			digitalWrite(a2a_PIN, HIGH);   // shut it down.
		}
	}
}
