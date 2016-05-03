///////////////////////////////////////////////////////////
//AUDESIS.PX CODE FOR MVP/////////////////////////////////
//REVISION: 3, switched to GitLAB starting version 3//////
/////////////////////////////////////////////////////////
//FUNCTION:
//1) Mics are functional but finniky
//2) Preliminary gain control over both signals successful
//3) Toggle between features through button clicks, with button debounce
//4) Initial volume ramp at reboot to prevent sudden pop of sound in user ears

//ISSUES:
//1)Pluse of high output at reboot
//2)Headphone mute/unmute doesn't work
//3)(RESOLVED) - Button hold skips through modes rapidly
//4)No augmentation (EQ/effect) to signals to produce accurate modes yet
//5)No control over volume of music source
//6)(RESOLVED) - Delay applied to loop to get serial out, if advanced analysis will
//be done then this will have to change

//NOTE: to comment a whole section add /* before and */ after

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


// GUItool: begin automatically generated code
AudioInputI2SQuad        i2s_quad1;      //xy=377,202
AudioMixer4              mixer1;         //xy=629,160
AudioMixer4              mixer2;         //xy=631,254
AudioOutputI2SQuad       i2s_quad2;      //xy=822,214
AudioConnection          patchCord1(i2s_quad1, 0, mixer1, 0);
AudioConnection          patchCord2(i2s_quad1, 1, mixer2, 0);
AudioConnection          patchCord3(i2s_quad1, 2, mixer1, 1);
AudioConnection          patchCord4(i2s_quad1, 3, mixer2, 1);
AudioConnection          patchCord5(mixer1, 0, i2s_quad2, 0);
AudioConnection          patchCord6(mixer2, 0, i2s_quad2, 1);
//AudioConnection          patchCord7(mixer1, 0, i2s_quad2, 2); // output to lower headphone jack
//AudioConnection          patchCord8(mixer2, 0, i2s_quad2, 3); // output to lower headphone jack
AudioControlSGTL5000     sgtl5000_1;     //xy=162,36
AudioControlSGTL5000     sgtl5000_2;     //xy=492,44
// GUItool: end automatically generated code

const int myInput = AUDIO_INPUT_LINEIN;
const int buttonPin = 2;

int buttonState;
int lastButtonState  =LOW;
int news = 1; //variable used to ensure that only new status updates are sent across serial
int mode = -1;//switch between audesis modes by changing constant below:
float volx = 1; //dac volume max
long lastDebounceTime = 0;
long debounceDelay = 50;
elapsedMillis currentTime; //current clock time elapsed since execution (reboot)

void rampUp(AudioControlSGTL5000 & audioShield){

  //define variables
  float rampTime = 5000; //volume initial ramp time
  float delayTime = 50; //ms
  int x=0;
  unsigned int startTime;
  startTime=currentTime;
    
  //rampStep = 1 / rampTime;
  //delayTime = rampTime / rampStep;
  Serial.println("start");
  while (currentTime < startTime + rampTime) {
    float elapsedTime = currentTime-startTime;
    float vol = elapsedTime/rampTime;
    audioShield.volume(vol*0.75);
    Serial.println(vol,4);
    delay(delayTime);
    }
   audioShield.dacVolume(1); //just in case I skip an iteration
}

void setup() {
  //Begin serial
   Serial.begin(9600);
   
  //Setup button
    pinMode(buttonPin,INPUT_PULLUP);
  
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(50);

  // Enable the first audio shield (Music), select input, and enable output
  sgtl5000_1.setAddress(LOW);
  //sgtl5000_1.dacVolume(0);
  sgtl5000_1.enable();
  sgtl5000_1.muteHeadphone();
  sgtl5000_1.volume(0); //0.75
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.audioPreProcessorEnable();
  

  // Enable the second audio shield (Mics), select input, and enable output
  //sgtl5000_2.setAddress(HIGH);
  //sgtl5000_2.enable();
  //sgtl5000_2.inputSelect(myInput);
  //sgtl5000_2.volume(0.75);
  //sgtl5000_2.audioPreProcessorEnable();

//xxxxxxxxxxxxxxxxxxxthis will change once new filters are added in Rev3
//xxxxxxxxxxNotes: mixer 1 = left ear, mixer 2 = right ear
//xxxxxxxxxxxx mixer1/2 channel 0 is music, channel 1 is Mic

//Initial boot on Deep Focus mode
//music mix
mixer1.gain(0,0.75);
mixer2.gain(0,0.75);
//mic mix
mixer1.gain(1,0);
mixer2.gain(1,0);


Serial.println("Start Ramping Up");
sgtl5000_1.dacVolumeRampDisable();
rampUp(sgtl5000_1);
Serial.println("done ramping");
//sgtl5000_1.muteHeadphone();
}

void loop() {
///////first ramp in volume at deep focus mode////
//you want this function to freeze everything below anyway so using
//delay() is fine
//sgtl5000_1.dacVolumeRampDisable()

int reading = digitalRead(buttonPin);

//initially ramp volume up to avoid any mic feedback or so

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

 if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
          mode=mode+1;
          Serial.print("Button released, mode: ");
          Serial.println(mode);
          Serial.print("RampStep = ");
          //Serial.println(rampStep,5);
          news=1;
      }
    }
  }


if (mode == 0 && news == 1) {
//Deep focus mode; mode 0
  //if (news==1) {
Serial.println("Deep focus activated");
news=0;
  //}
//music mix
mixer1.gain(0,0.75);
mixer2.gain(0,0.75);
//mic mix
mixer1.gain(1,0); //might shut this off to save battery (using disable() or mutelinein())
mixer2.gain(1,0);

sgtl5000_1.autoVolumeEnable();
sgtl5000_1.autoVolumeControl(0,0,0,-18,10,15);
}

if (mode ==1 && news == 1) {
//conversation; mode1
//notes: get mics to percieve conversation range with EQ, apply inv.vocal boost on music

Serial.println("Conversation/temp DF mode activated");
sgtl5000_1.autoVolumeDisable();
news=0;

/*
//music mix
mixer1.gain(0,0.05);
mixer2.gain(0,0.05);
//mic mix
mixer1.gain(1,0.065);
mixer2.gain(1,0.065);
*/

//music mix
mixer1.gain(0,0.75);
mixer2.gain(0,0.75);
//mic mix
mixer1.gain(1,0);
mixer2.gain(1,0);

}

if (mode ==2 && news == 1) {
//Awareness mode; mode 2
//notes: get mics to percieve what's going on around, no EQ on music,
//add EQ on mic for most natural surround sound
    
Serial.println("Awareness mode activated");
news=0;

//music mix
mixer1.gain(0,0.2);
mixer2.gain(0,0.2);
//mic mix
mixer1.gain(1,0.05);
mixer2.gain(1,0.05);  
}
//Serial.println("working");
//delay(100);
if (mode>2) {
mode=0;
}

// save the reading.  Next time through the loop,
// it'll be the lastButtonState:
lastButtonState = reading;
}
