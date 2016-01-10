// Luigi Findanno 2016
// Web Radio receiver - DLNA audio renderer
//
// Send commands to OpenWrt and receive the string
// to view to the display by the serial port

#include <Wire.h>
#include <PCF8574_HD44780_I2C.h>
#include <RotaryEncoder.h>

#define PB_POWER 4
#define POWER_RELE 5
#define PB_MUTE 7

PCF8574_HD44780_I2C lcd(0x27,16,2);
RotaryEncoder volume(10,9);
RotaryEncoder tuner(12,11);


String inputString = "";
boolean stringComplete = false;
String row1;
String row2;
String oldrow1;
String oldrow2;
int pbMute=1;
int posVolume=0;
int posTuner=0;
int oldVolume=0;
int oldTuner=0;
long tdebounce=0;
int debounce=0;
long tsync=0;
int sync=0;

void setup() {
  pinMode(PB_POWER, INPUT);
  pinMode(PB_MUTE, INPUT);
  pinMode(POWER_RELE, OUTPUT);
  digitalWrite(POWER_RELE, 1);
  lcd.init();
  lcd.backlight();
  lcd.clear(); 
  Serial.begin(9600);
  inputString.reserve(70);
  row1.reserve(16);
  row2.reserve(16);
  oldrow1.reserve(16);
  oldrow2.reserve(16);
  row1="Web Radio - DLNA";
  row2="Starting...";
  oldrow1="";
  oldrow2="";
  writeLCD();
  delay(1000);
}

void loop() {
  serialEvent();
  if (stringComplete) {
    doCommand();
    inputString = "";
    stringComplete = false;
  }
  writeLCD();
  checkTimers();
  checkPowerButton();
  checkVolume();
  checkTuner();
  checkMute();
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }else{
      inputString += inChar;  
    }
  }
}

void doCommand(){
  String cmd = "";
  String par = "";
  cmd.reserve(4);
  cmd.reserve(64);
  int f;
  
  if (inputString.length() > 20){
    f = 24;
  }else{
    f = inputString.length();
  }
  
  for (int i = 0; i < f; i++) {
    if (i < 4){
      cmd += inputString.charAt(i);  
    }
    if (i > 3){
      par += inputString.charAt(i);  
    }
  }
  
  if (cmd.equals("ECHO")) {
    Serial.println("ECHO");
  }
  if (cmd.equals("RIG1")) {
    row1 = par;
  }
  if (cmd.equals("RIG2")) {
    row2 = par;
  }  
  if (cmd.equals("CLS ")){
    row1="";
    row2="";
  }
}

void writeLCD(){
  if ((!(row1.equals(oldrow1))) || (!(row2.equals(oldrow2)))){
    oldrow1=row1;
    oldrow2=row2;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(row1);
    lcd.setCursor(0,1);
    lcd.print(row2);
  }
}

void checkPowerButton(){ 
  if (digitalRead(PB_POWER)==1){ // power button pressed
    if (digitalRead(POWER_RELE)==1){ 
      Serial.println("SHTD");
      row1="Shutting down...";
      row2="Goodbye";
      writeLCD();
      delay(10000);
      digitalWrite(POWER_RELE, 0);
    }
  }
}

void checkVolume(){
  volume.tick();
  posVolume = volume.getPosition();
  if (posVolume > oldVolume){
    Serial.println("VOL+");
    oldVolume = posVolume;
    sync=1;
    tsync=millis();
  }
  if (posVolume < oldVolume){
    Serial.println("VOL-");
    oldVolume = posVolume;
    sync=1;
    tsync=millis();
  }
}


void checkTuner(){
  tuner.tick();
  posTuner = tuner.getPosition();
  if (posTuner > oldTuner){
    Serial.println("TUN+");
    oldTuner = posTuner;
  }
  if (posTuner < oldTuner){
    Serial.println("TUN-");
    oldTuner = posTuner;
  }
}

void checkMute() {
  if (debounce==0) {
     if ((pbMute==1)&&(digitalRead(PB_MUTE)==0)){ // mute button pressed
        tdebounce=millis();
        debounce=1;
        Serial.println("MUTE");
     }
     pbMute=digitalRead(PB_MUTE);
  }
}

void checkTimers() {
  // mute debounce timer (300ms)
  if (((millis() - tdebounce) > 300) && (debounce == 1)){ // debounce mute button
    debounce=0;
  }
  // resync display signal (3 seconds)
  if (((millis() - tsync) > 3000) && (sync == 1)){ // debounce mute button
    sync=0;
    Serial.println("SYNC");
  }  
}

