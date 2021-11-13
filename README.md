<h1># Temp-Controlled-Linear-Actuator<br>
 Arduino SHT31d Temperature Controlled Linear Actuator</h1>
 
<h3>An Arduino Pro Mini uses an L9110 to control a micro linear actuator at 12v. I set a 5 sec off delay to save the actuator between temp changes. Temp in code is 30 degrees celcius open/ 28 degrees celcius close. I use this in conjunction with a fan to circulate air in my home communications cupboard, the actuator opens/closes the cupboard door a few centimetres when it gets too hot.</h3>

<b>Required Parts:</b><br>
<ol>
 <li>1x Arduino Pro Mini 3v3 [https://www.aliexpress.com/item/1005001621723982.html]</li>
 <li>1x SHT31d Sensor [https://www.aliexpress.com/item/32678741657.html]</li>
 <li>1x L9110S 12v H-bridge Stepper Motor Dual DC Driver Controller [https://www.aliexpress.com/item/32896620313.html]</li>
 <li>1x 12v to 5v converter (DSN-Mini-360 DC-DC 4.75V-23V to 1V-17V Buck Converter Step Down Module) [https://www.aliexpress.com/item/32952187058.html]</li>
 <li>1x 12V Micro Linear Actuator 30mm 7MMS 128N [https://www.aliexpress.com/item/1005001427208855.html]</li>
 <li>1x 12v 1amp power supply</li>
 </ol>
 
You will also need an Arduino USB programmer of your choice to upload code.<br>

<b>Assembly:</b><br>
<ol>
<li>I used Pin 6 and 7 on the Pro Mini to Control the L9110. Wire Pro Mini Pin 6 to A-1A on L9110, Wire Pin 7 to A-1B on L9110</li>
<li>Use I2C for SHT31d temp sensor (VCC, GND, Pin A4, and Pin A5 on Pro Mini)</li>
<li>Wire 12v to DSN-Mini-360 input and adjust output to 5v. Connect output to Pro Mini (Pin RAW and GND)</li>
<li>Wire 12v to L9110S VCC, GND Pins</li>
<li>Wire Actuator to L9110 output 1A (you may need to experiment with wire direction based on actuator)</li>
</ol>
