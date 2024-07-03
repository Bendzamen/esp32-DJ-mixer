/*
    ESP32 DJ Mixer
    This code emulates a BLE MIDI device on an ESP32 board.
    It has been tested using open-source Mixxx DJ software.
    Using a MAC PC, the response was fast and reliable, enough for basic DJ mixing.

    Created by Benjamin Lapos 2023
*/

#include <Arduino.h>
#include <BLEMidi.h>
#include <SimpleDebouncer.h>
#include <RunningAverage.h>
#include <Rotary.h>

const int status_led = 2;
bool led_state = false;
int last_change = 0;

//setup of rotary encoders
Rotary RotR = Rotary(25, 26);   // right  dj wheel
Rotary RotL = Rotary(27, 14);   // left dj wheel

//2 potentiometers without multiplexer setup
RunningAverage RA_volR(10);     // right volume
RunningAverage RA_volL(10);     // left volume

const int vol_R = 35;   // right volume pin
const int vol_L = 34;   // left volume pin

// prev values of volume knobs (for running avg)
int prev_volR = 0;
int prev_volL = 0;

//setup of multiplexer potentiometers
const int s0 = 21;
const int s1 = 19;
const int s2 = 18;
const int s3 = 5;

const int SIG_pin = 39;
//const int en_pin = 36;

int i = 0;
unsigned long lastMeasure = 0;

int prev[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 

RunningAverage RApot[16] = {
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10), 
  RunningAverage(10)
  }; 

//button defining
SimpleDebouncer PlayR(22);  // on / off right side play
SimpleDebouncer QueR(23);   // on / off right side que
SimpleDebouncer RevR(17);   // on / off right side reverse

SimpleDebouncer PlayL(12);  // on / off left side play
SimpleDebouncer QueL(13);   // on / off left side que
SimpleDebouncer RevL(32);   // on / off left side reverse

SimpleDebouncer EffectR(16);    // on / off left side effect
SimpleDebouncer EffectL(4);     // on / off left side effect

SimpleDebouncer HeadphonesR(33);    // on / off right headphones
SimpleDebouncer HeadphonesL(15);    // on / off left headphones

//define midi channels
const int channel_PlayR = 0;
const int channel_QueR = 1;
const int channel_RevR = 2;

const int channel_PlayL = 3;
const int channel_QueL = 4;
const int channel_RevL = 5;

const int channel_EffectR = 6;
const int channel_EffectL = 7;

const int channel_HeadphonesR = 8;
const int channel_HeadphonesL = 9;

const int channel_volR = 10; 
const int channel_volL = 11; 

const int channel_RotR_CW = 12;
const int channel_RotR_CCW = 13;
const int channel_RotL_CW = 14;
const int channel_RotL_CCW = 15;

// esp acts as 2 controllers, one for multiplexer connected devices
// and one for the rest

const uint8_t controller_number_multix = 0;
const uint8_t controller_else = 1;

void setup() {
  Serial.begin(115200);
  //start BLE midi server
  BLEMidiServer.begin("MIDI DJ Mix");

  pinMode(status_led, OUTPUT);
  digitalWrite(status_led, LOW);

  //multiplexer setup
  pinMode(s0, OUTPUT); 
  pinMode(s1, OUTPUT); 
  pinMode(s2, OUTPUT); 
  pinMode(s3, OUTPUT); 
  //pinMode(en_pin, OUTPUT);
  pinMode(SIG_pin, INPUT);

  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);
  //digitalWrite(en_pin, HIGH);
  
  for (int i = 0; i<16; i++){
    RApot[i].clear(); // explicitly start clean
  }

  //button setup
  PlayR.setup();
  QueR.setup();
  RevR.setup();

  PlayL.setup();
  QueL.setup();
  RevL.setup();

  EffectR.setup();
  EffectL.setup();

  HeadphonesR.setup();
  HeadphonesL.setup();

  //2 potentiometers without multiplexer setup
  pinMode(vol_R, INPUT);
  pinMode(vol_L, INPUT);

  //rotary encoders setup
  RotR.begin(true);
  RotL.begin(true);

  lastMeasure = millis();
  last_change = millis();

}


int readMux(int channel){
  int controlPin[4] = {s3, s2, s1, s0};

  int muxChannel[16][4]={
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
  };

  //loop through the 4 sig
  for(int j = 0; j < 4; j ++){
    digitalWrite(controlPin[j], muxChannel[channel][j]);
  }
  //read the value at the SIG pin
  int val = analogRead(SIG_pin);
  //return the value
  return val;
}


void loop() {
  if(BLEMidiServer.isConnected()){
    digitalWrite(status_led, HIGH);

    //take care of multiplexer potentiometer
    if (millis()-lastMeasure>=1){
      if (i>=16){
        i = 0;
      }
      //digitalWrite(en_pin, LOW);
      int val = readMux(i);
      //digitalWrite(en_pin, HIGH);

      
      RApot[i].addValue(val);
      int fin_val = map(round(RApot[i].getAverage()), 0, 4095, 0, 138);
      if (((i == 7 || i==9)  || (i==14 || i==10)) || ((i==12 || i==13) || ((i==11 || i==8) || i==0))){
        //Serial.println("ok");
        fin_val = map(round(RApot[i].getAverage()), 0, 4095, 127, -11);
      }
      
      //Serial.println(fin_val);
      
      if (abs(prev[i]-fin_val)>1){
        Serial.print(i);
        Serial.print(" ");
        Serial.println(fin_val);
        prev[i] = fin_val;
        BLEMidiServer.controlChange(i, controller_number_multix, fin_val);
      }

      //take care of remaining 2 potentiometers
      int valR = analogRead(vol_R);
      RA_volR.addValue(valR);
      int fin_valR = map(round(RA_volR.getAverage()), 0, 4095, 0, 138);
      if (abs(prev_volR-fin_valR)>1){
        Serial.print("R");
        Serial.print(" ");
        Serial.println(fin_valR);
        prev_volR = fin_valR;
        BLEMidiServer.controlChange(channel_volR, controller_else, fin_valR);
      }

      int valL = analogRead(vol_L);
      RA_volL.addValue(valL);
      int fin_valL = map(round(RA_volL.getAverage()), 0, 4095, 0, 138);
      if (abs(prev_volL-fin_valL)>1){
        Serial.print("L");
        Serial.print(" ");
        Serial.println(fin_valL);
        prev_volL = fin_valL;
        BLEMidiServer.controlChange(channel_volL, controller_else, fin_valL);
      }

      lastMeasure = millis();
      i++;
    }

    //update buttons
    //right play rev que
    PlayR.process();
    QueR.process();
    RevR.process();

    if (RevR.pressed()){
      BLEMidiServer.controlChange(channel_RevR, controller_else, 1);
      Serial.println("RevR pressed");
    }
    if (RevR.released()){
      BLEMidiServer.controlChange(channel_RevR, controller_else, 0);
      Serial.println("RevR released");
    }

    if (PlayR.pressed()){
      BLEMidiServer.controlChange(channel_PlayR, controller_else, 1);
      Serial.println("PlayR pressed");
    }

    if (QueR.pressed()){
      BLEMidiServer.controlChange(channel_QueR, controller_else, 1);
      Serial.println("QueR pressed");
    }

    //left play rev que
    PlayL.process();
    QueL.process();
    RevL.process();

    if (RevL.pressed()){
      BLEMidiServer.controlChange(channel_RevL, controller_else, 1);
      Serial.println("RevL pressed");
    }
    if (RevL.released()){
      BLEMidiServer.controlChange(channel_RevL, controller_else, 0);
      Serial.println("RevL released");
    }

    if (PlayL.pressed()){
      BLEMidiServer.controlChange(channel_PlayL, controller_else, 1);
      Serial.println("PlayL pressed");
    }

    if (QueL.pressed()){
      BLEMidiServer.controlChange(channel_QueL, controller_else, 1);
      Serial.println("QueL pressed");
    }
    
    //effects buttons
    EffectR.process();
    EffectL.process();

    if (EffectR.pressed()){
      BLEMidiServer.controlChange(channel_EffectR, controller_else, 1);
      Serial.println("EffectR pressed");
    }
    if (EffectL.pressed()){
      BLEMidiServer.controlChange(channel_EffectL, controller_else, 1);
      Serial.println("EffectL pressed");
    }

    //headphones buttons
    HeadphonesR.process();
    HeadphonesL.process();

    if (HeadphonesR.pressed()){
      BLEMidiServer.controlChange(channel_HeadphonesR, controller_else, 1);
      Serial.println("HeadphonesR pressed");
    }
    if (HeadphonesL.pressed()){
      BLEMidiServer.controlChange(channel_HeadphonesL, controller_else, 1);
      Serial.println("HeadphonesL pressed");
    }

    //process rotary encoders
    unsigned char resultR = RotR.process();
    if (resultR==DIR_CW) {
      BLEMidiServer.controlChange(channel_RotR_CW, controller_else, 1);
      Serial.println("R Right");
    }
    else if (resultR==DIR_CCW) {
      BLEMidiServer.controlChange(channel_RotR_CCW, controller_else, 1);
      Serial.println("R Left");
    }

    unsigned char resultL = RotL.process();
    if (resultL==DIR_CW) {
      BLEMidiServer.controlChange(channel_RotL_CW, controller_else, 1);
      Serial.println("L Right");
    }
    else if (resultL==DIR_CCW) {
      BLEMidiServer.controlChange(channel_RotL_CCW, controller_else, 1);
      Serial.println("L Left");
    }
  }
  
  else{
    if (millis()-last_change>500){
      Serial.println("*");
      if (led_state){
        digitalWrite(status_led, HIGH);
        led_state = false;
      }
      else{
        digitalWrite(status_led, LOW);
        led_state = true;
      }
      last_change = millis();
    }
  }
}
