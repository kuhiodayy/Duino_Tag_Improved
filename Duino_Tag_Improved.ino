// Duino Tag release V1.01
// Laser Tag for the arduino based on the Miles Tag Protocol.
// By J44industries: www.J44industries.blogspot.com
// For information on building your own Duino Tagger go to: http://www.instructables.com/member/j44/
//
// Much credit deserves to go to Duane O'Brien if it had not been for the excellent Duino Tag tutorials he wrote I would have never been able to write this code.
// Duane's tutorials are highly recommended reading in order to gain a better understanding of the arduino and IR communication. See his site http://aterribleidea.com/duino-tag-resources/
//
// This code sets out the basics for arduino based laser tag system and tries to stick to the miles tag protocol where possible.
// Miles Tag details: http://www.lasertagparts.com/mtdesign.htm
// There is much scope for expanding the capabilities of this system, and hopefully the game will continue to evolve for some time to come.
// Licence: Attribution Share Alike: Give credit where credit is due, but you can do what you like with the code.
// If you have code improvements or additions please go to http://duinotag.blogspot.com
//

// Digital IO's
int triggerPin = 4; // Push button for primary fire. Low = pressed
int trigger2Pin = 3; // Push button for secondary fire. Low = pressed
int speakerPin = 12; // Direct output to piezo sounder/speaker
int audioPin = 9; // Audio Trigger. Can be used to set off sounds recorded in the kind of electronics you can get in greetings card that play a custom message.
int lifePin = 6; // An analogue output (PWM) level corresponds to remaining life. Use PWM pin: 3,5,6,9,10 or 11. Can be used to drive LED bar graphs. eg LM3914N
int ammoPin = 5; // An analogue output (PWM) level corresponds to remaining ammunition. Use PWM pin: 3,5,6,9,10 or 11.
int hitPin = 13; // LED output pin used to indicate when the player has been hit.
int IRtransmitPin = 8; // Primary fire mode IR transmitter pin: Use pins 2,4,7,8,12 or 13. DO NOT USE PWM pins!! More info: http://j44industries.blogspot.com/2009/09/arduino-frequency-generation.html#more
int IRtransmit2Pin = 9; // Secondary fire mode IR transmitter pin: Use pins 2,4,7,8,12 or 13. DO NOT USE PWM pins!!
int IRreceivePin = 2; // The pin that incoming IR signals are read from
int IRreceive2Pin = 11; // Allows for checking external sensors are attached as well as distinguishing between sensor locations (eg spotting head shots)
// Minimum gun requirements: trigger, receiver, IR led, hit LED.

// Player and Game details
int myTeamID = 1; // 1-7 (0 = system message)
int myPlayerID = 5; // Player ID
int myGameID = 0; // Interprited by configureGane subroutine; allows for quick change of game types.
int myWeaponID = 0; // Deffined by gameType and configureGame subroutine.
int myWeaponHP = 0; // Deffined by gameType and configureGame subroutine.
int maxAmmo = 0; // Deffined by gameType and configureGame subroutine.
int maxLife = 0; // Deffined by gameType and configureGame subroutine.
int automatic = 0; // Deffined by gameType and configureGame subroutine. Automatic fire 0 = Semi Auto, 1 = Fully Auto.
int automatic2 = 0; // Deffined by gameType and configureGame subroutine. Secondary fire auto?

//Incoming signal Details
int received[18]; // Received data: received[0] = which sensor, received[1] - [17] byte1 byte2 parity (Miles Tag structure)
int check = 0; // Variable used in parity checking

// Stats
int ammo = 0; // Current ammunition
int life = 0; // Current life

// Code Variables
int timeOut = 0; // Deffined in frequencyCalculations (IRpulse + 50)
int FIRE = 0; // 0 = don't fire, 1 = Primary Fire, 2 = Secondary Fire
int TR = 0; // Trigger Reading
int LTR = 0; // Last Trigger Reading
int T2R = 0; // Trigger 2 Reading (For secondary fire)
int LT2R = 0; // Last Trigger 2 Reading (For secondary fire)

// Signal Properties
int IRpulse = 600; // Basic pulse duration of 600uS MilesTag standard 4*IRpulse for header bit, 2*IRpulse for 1, 1*IRpulse for 0.
int IRfrequency = 38; // Frequency in kHz Standard values are: 38kHz, 40kHz. Choose dependant on your receiver characteristics
int IRt = 0; // LED on time to give correct transmission frequency, calculated in setup.
int IRpulses = 0; // Number of oscillations needed to make a full IRpulse, calculated in setup.
int header = 4; // Header lenght in pulses. 4 = Miles tag standard
int maxSPS = 10; // Maximum Shots Per Seconds. Not yet used.
int TBS = 0; // Time between shots. Not yet used.

// Transmission data
int byte1[8]; // String for storing byte1 of the data which gets transmitted when the player fires.
int byte2[8]; // String for storing byte1 of the data which gets transmitted when the player fires.
int myParity = 0; // String for storing parity of the data which gets transmitted when the player fires.

// Received data
int memory = 10; // Number of signals to be recorded: Allows for the game data to be reviewed after the game, no provision for transmitting / accessing it yet though.
int hitNo = 0; // Hit number
// Byte1
int player[10]; // Array must be as large as memory
int team[10]; // Array must be as large as memory
// Byte2
int weapon[10]; // Array must be as large as memory
int hp[10]; // Array must be as large as memory
int parity[10]; // Array must be as large as memory


void setup() {
// Serial coms set up to help with debugging.
Serial.begin(9600); 
Serial.println("Startup...");
// Pin declarations
pinMode(triggerPin, INPUT);
pinMode(trigger2Pin, INPUT);
pinMode(speakerPin, OUTPUT);
pinMode(audioPin, OUTPUT);
pinMode(lifePin, OUTPUT);
pinMode(ammoPin, OUTPUT);
pinMode(hitPin, OUTPUT);
pinMode(IRtransmitPin, OUTPUT);
pinMode(IRtransmit2Pin, OUTPUT);
pinMode(IRreceivePin, INPUT);
pinMode(IRreceive2Pin, INPUT);

frequencyCalculations(); // Calculates pulse lengths etc for desired frequency
configureGame(); // Look up and configure game details
tagCode(); // Based on game details etc works out the data that will be transmitted when a shot is fired


digitalWrite(triggerPin, HIGH); // Not really needed if your circuit has the correct pull up resistors already but doesn't harm
digitalWrite(trigger2Pin, HIGH); // Not really needed if your circuit has the correct pull up resistors already but doesn't harm

for (int i = 1;i < 254;i++) { // Loop plays start up noise
analogWrite(ammoPin, i);
playTone((3000-9*i), 2);
} 

// Next 4 lines initialise the display LEDs
analogWrite(ammoPin, ((int) ammo));
analogWrite(lifePin, ((int) life));
lifeDisplay();
ammoDisplay();

Serial.println("Ready....");
}


// Main loop most of the code is in the sub routines
void loop(){
receiveIR();
if(FIRE != 0){
shoot();
ammoDisplay();
}
triggers();
}


// SUB ROUTINES


void ammoDisplay() { // Updates Ammo LED output
float ammoF;
ammoF = (260/maxAmmo) * ammo;
if(ammoF <= 0){ammoF = 0;}
if(ammoF > 255){ammoF = 255;}
analogWrite(ammoPin, ((int) ammoF));
}


void lifeDisplay() { // Updates Ammo LED output
float lifeF;
lifeF = (260/maxLife) * life;
if(lifeF <= 0){lifeF = 0;}
if(lifeF > 255){lifeF = 255;}
analogWrite(lifePin, ((int) lifeF));
} 


void receiveIR() { // Void checks for an incoming signal and decodes it if it sees one.

if(digitalRead(IRreceivePin) == LOW){ // If the receive pin is low a signal is being received.
digitalWrite(hitPin,HIGH);
hit();
}
else if(digitalRead(IRreceive2Pin) == LOW){
  digitalWrite(hitPin,HIGH);
  hit();
  Serial.println("headshot!");
}
}


void shoot() {
if(FIRE == 1){ // Has the trigger been pressed?
Serial.println("FIRE 1");
sendPulse(IRtransmitPin, 4); // Transmit Header pulse, send pulse subroutine deals with the details
delayMicroseconds(IRpulse);

for(int i = 0; i < 8; i++) { // Transmit Byte1
if(byte1[i] == 1){
sendPulse(IRtransmitPin, 1);
//Serial.print("1 ");
}
//else{Serial.print("0 ");}
sendPulse(IRtransmitPin, 1);
delayMicroseconds(IRpulse);
}

for(int i = 0; i < 8; i++) { // Transmit Byte2
if(byte2[i] == 1){
sendPulse(IRtransmitPin, 1);
// Serial.print("1 ");
}
//else{Serial.print("0 ");}
sendPulse(IRtransmitPin, 1);
delayMicroseconds(IRpulse);
}

if(myParity == 1){ // Parity
sendPulse(IRtransmitPin, 1);
}
sendPulse(IRtransmitPin, 1);
delayMicroseconds(IRpulse);
Serial.println("");
Serial.println("DONE 1");
playTone(100, 5);
}


if(FIRE == 2){ // Where a secondary fire mode would be added
Serial.println("FIRE 2");
sendPulse(IRtransmitPin, 4); // Header
Serial.println("DONE 2");
}
FIRE = 0;
ammo = ammo - 1;
}


void sendPulse(int pin, int length){ // importing variables like this allows for secondary fire modes etc.
// This void genertates the carrier frequency for the information to be transmitted
int i = 0;
int o = 0;
while( i < length ){
i++;
while( o < IRpulses ){
o++;
digitalWrite(pin, HIGH);
delayMicroseconds(IRt);
digitalWrite(pin, LOW);
delayMicroseconds(IRt);
}
}
}


void triggers() { // Checks to see if the triggers have been presses
LTR = TR; // Records previous state. Primary fire
LT2R = T2R; // Records previous state. Secondary fire
TR = digitalRead(triggerPin); // Looks up current trigger button state
T2R = digitalRead(trigger2Pin); // Looks up current trigger button state
// Code looks for changes in trigger state to give it a semi automatic shooting behaviour
if(TR != LTR && TR == LOW){
FIRE = 1;
}
if(T2R != LT2R && T2R == LOW){
FIRE = 2;
}
if(TR == LOW && automatic == 1){
FIRE = 1;
}
if(T2R == LOW && automatic2 == 1){
FIRE = 2;
}
if(FIRE == 1 || FIRE == 2){
if(ammo < 1){FIRE = 0; noAmmo();}
if(life < 1){FIRE = 0; dead();}
// Fire rate code to be added here 
}

}


void configureGame() { // Where the game characteristics are stored, allows several game types to be recorded and you only have to change one variable (myGameID) to pick the game.
if(myGameID == 0){
myWeaponID = 1;
maxAmmo = 30;
ammo = 30;
maxLife = 3;
life = 3;
myWeaponHP = 1;
}
if(myGameID == 1){
myWeaponID = 1;
maxAmmo = 100;
ammo = 100;
maxLife = 10;
life = 10;
myWeaponHP = 2;
}
}


void frequencyCalculations() { // Works out all the timings needed to give the correct carrier frequency for the IR signal
IRt = (int) (500/IRfrequency); 
IRpulses = (int) (IRpulse / (2*IRt));
IRt = IRt - 4;
// Why -4 I hear you cry. It allows for the time taken for commands to be executed.
// More info: http://j44industries.blogspot.com/2009/09/arduino-frequency-generation.html#more

Serial.print("Oscilation time period /2: ");
Serial.println(IRt);
Serial.print("Pulses: ");
Serial.println(IRpulses);
timeOut = IRpulse + 50; // Adding 50 to the expected pulse time gives a little margin for error on the pulse read time out value
}


void tagCode() { // Works out what the players tagger code (the code that is transmitted when they shoot) is
byte1[0] = myTeamID >> 2 & B1;
byte1[1] = myTeamID >> 1 & B1;
byte1[2] = myTeamID >> 0 & B1;

byte1[3] = myPlayerID >> 4 & B1;
byte1[4] = myPlayerID >> 3 & B1;
byte1[5] = myPlayerID >> 2 & B1;
byte1[6] = myPlayerID >> 1 & B1;
byte1[7] = myPlayerID >> 0 & B1;


byte2[0] = myWeaponID >> 2 & B1;
byte2[1] = myWeaponID >> 1 & B1;
byte2[2] = myWeaponID >> 0 & B1;

byte2[3] = myWeaponHP >> 4 & B1;
byte2[4] = myWeaponHP >> 3 & B1;
byte2[5] = myWeaponHP >> 2 & B1;
byte2[6] = myWeaponHP >> 1 & B1;
byte2[7] = myWeaponHP >> 0 & B1;

myParity = 0;
for (int i=0; i<8; i++) {
if(byte1[i] == 1){myParity = myParity + 1;}
if(byte2[i] == 1){myParity = myParity + 1;}
myParity = myParity >> 0 & B1;
}

// Next few lines print the full tagger code.
Serial.print("Byte1: ");
Serial.print(byte1[0]);
Serial.print(byte1[1]);
Serial.print(byte1[2]);
Serial.print(byte1[3]);
Serial.print(byte1[4]);
Serial.print(byte1[5]);
Serial.print(byte1[6]);
Serial.print(byte1[7]);
Serial.println();

Serial.print("Byte2: ");
Serial.print(byte2[0]);
Serial.print(byte2[1]);
Serial.print(byte2[2]);
Serial.print(byte2[3]);
Serial.print(byte2[4]);
Serial.print(byte2[5]);
Serial.print(byte2[6]);
Serial.print(byte2[7]);
Serial.println();

Serial.print("Parity: ");
Serial.print(myParity);
Serial.println();
}


void playTone(int atone, int duration) { // A sub routine for playing tones like the standard arduino melody example
for (long i = 0; i < duration * 1000L; i += atone * 2) {
digitalWrite(speakerPin, HIGH);
delayMicroseconds(atone);
digitalWrite(speakerPin, LOW);
delayMicroseconds(atone);
}
}


void dead() { // void determines what the tagger does when it is out of lives
// Makes a few noises and flashes some lights
for (int i = 1;i < 254;i++) {
analogWrite(ammoPin, i);
playTone((1000+9*i), 2);
} 
analogWrite(ammoPin, ((int) ammo));
analogWrite(lifePin, ((int) life));
Serial.println("DEAD");

for (int i=0; i<10; i++) {
analogWrite(ammoPin, 255);
digitalWrite(hitPin,HIGH);
delay (500);
analogWrite(ammoPin, 0);
digitalWrite(hitPin,LOW);
delay (500);
}
}


void noAmmo() { // Make some noise and flash some lights when out of ammo
digitalWrite(hitPin,HIGH);
playTone(500, 100);
playTone(1000, 100);
digitalWrite(hitPin,LOW);
}


void hit() { // Make some noise and flash some lights when you get shot
digitalWrite(hitPin,HIGH);
life = life - 1;
Serial.print("Life: ");
Serial.println(life);
playTone(500, 500);
if(life <= 0){dead();}
digitalWrite(hitPin,LOW);
lifeDisplay();
}
