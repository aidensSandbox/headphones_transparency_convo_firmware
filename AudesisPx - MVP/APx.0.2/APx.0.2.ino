///////////////////////////////////////////////////////////
//AUDESIS.PX CODE FOR MVP/////////////////////////////////
//REVISION: 2.5, switched to GitLAB starting version 2.5//////
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
AudioInputI2SQuad        i2s_quad1;      //xy=269,392
AudioFilterBiquad        biquad1;        //xy=450,309
AudioFilterBiquad        biquad2;        //xy=467,491
AudioMixer4              mixer1;         //xy=718,351
AudioMixer4              mixer2;         //xy=720,445
AudioOutputI2SQuad       i2s_quad2;      //xy=911,405
AudioConnection          patchCord1(i2s_quad1, 0, mixer1, 0);
AudioConnection          patchCord2(i2s_quad1, 0, biquad1, 0);
AudioConnection          patchCord3(i2s_quad1, 1, mixer2, 0);
AudioConnection          patchCord4(i2s_quad1, 1, biquad2, 0);
AudioConnection          patchCord5(i2s_quad1, 2, mixer1, 1);
AudioConnection          patchCord6(i2s_quad1, 3, mixer2, 1);
AudioConnection          patchCord7(biquad1, 0, mixer1, 2);
AudioConnection          patchCord8(biquad2, 0, mixer2, 2);
AudioConnection          patchCord9(mixer1, 0, i2s_quad2, 0);
AudioConnection          patchCord10(mixer1, 0, i2s_quad2, 2);
AudioConnection          patchCord11(mixer2, 0, i2s_quad2, 1);
AudioConnection          patchCord12(mixer2, 0, i2s_quad2, 3);
AudioControlSGTL5000     sgtl5000_1;     //xy=251,227
AudioControlSGTL5000     sgtl5000_2;     //xy=581,235
// GUItool: end automatically generated code


const int myInput = AUDIO_INPUT_LINEIN;
const int buttonPin = 26;

int buttonState;
int lastButtonState  = LOW;
int news = 1; //variable used to ensure that only new status updates are sent across serial
int mode = -1;//switch between audesis modes by changing constant below:
long lastDebounceTime = 0;
long debounceDelay = 50;
float maxmusicvol=0.75;
elapsedMillis currentTime; //current clock time elapsed since execution (reboot)
//float currentTime = millis();

void rampUp(AudioControlSGTL5000 & audioShield, float maxrampvol) {

  //define variables
  float rampTime = 2000; //volume initial ramp time
  float delayTime = 50; //ms
  int x = 0;
  unsigned int startTime;
  startTime = currentTime;

  //rampStep = 1 / rampTime;
  //delayTime = rampTime / rampStep;
  Serial.println("start");
  while (currentTime < startTime + rampTime) {
    float elapsedTime = currentTime - startTime;
    float vol = elapsedTime / rampTime;
    audioShield.volume(vol * maxrampvol);
    Serial.println(vol, 4);
    delay(delayTime);
  }
  audioShield.volume(maxrampvol); //just in case I skip an iteration
}

void setupGain(float rw, float mc, float bq) {
 //Inputs (Master Raw gain, Master Mic gain, bi-quad gain, unused mixer channel [set to zero])
  int freq1=500;//300, 100
  int freq2=1500;
  int freq3=5000;//3400

  //turn off unused mixer channel
  mixer1.gain(3,0);
  mixer2.gain(3,0);
  
  //Filter parameter setup
  biquad1.setNotch (0,freq2,0.05); //works great and does the trick w one filter 
  biquad2.setNotch (0,freq2,0.05); //0.02
  //biquad2.setLowpass (0,300,0.2); 
  //biquad1.setLowpass (0,300,0.2); 
  
  //Ear 1
  mixer1.gain(0,rw); //Raw music gain
  mixer1.gain(1,mc); //Mic gain
  mixer1.gain(2,bq); //bi-quad filtered signal gain
  
  //Ear 2
  mixer2.gain(0,rw); //Raw music gain
  mixer2.gain(1,mc); //Mic gain
  mixer2.gain(2,bq); //bi-quad filtered signal gain
}

void setup() {
  //Begin serial
  Serial.begin(9600);

  //Setup button
  pinMode(buttonPin, INPUT_PULLUP);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(150);

  // Enable the first audio shield (Music), select input, and enable output
  sgtl5000_1.setAddress(LOW);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0); //0.75
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.audioPreProcessorEnable();


  // Enable the second audio shield (Mics), select input, and enable output
  sgtl5000_2.setAddress(HIGH);
  sgtl5000_2.enable();
  sgtl5000_2.volume(0.75);
  sgtl5000_2.inputSelect(myInput);
  sgtl5000_2.audioPreProcessorEnable();

  //xxxxxxxxxxxxxxxxxxxthis will change once new filters are added in Rev3
  //xxxxxxxxxxNotes: mixer 1 = left ear, mixer 2 = right ear
  //xxxxxxxxxxxx mixer1/2 channel 0 is music, channel 1 is Mic

  //Initial boot on Deep Focus mode
  //EQ for Music
  sgtl5000_1.eqSelect(3);
  sgtl5000_1.eqBands(0,-0.4,-0.5,-0.4,-0.2); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz
  //sgtl5000_1.eqBands(0,0,0,0,0); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz
  
  //setup gain
setupGain(maxmusicvol,0,0);

  Serial.println("Start Ramping Up");
  //sgtl5000_1.dacVolumeRampDisable();
  rampUp(sgtl5000_1, maxmusicvol);
  rampUp(sgtl5000_2, maxmusicvol);
  Serial.println("done ramping");
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

      // only toggle the mode if the new button state is HIGH
      if (buttonState == HIGH) {
        mode = mode + 1;
        Serial.print("Button released, mode: ");
        Serial.println(mode);
        news = 1;
      }
    }
  }


  if (mode == 0 && news == 1) {
    
    //EQ for Music
    sgtl5000_1.eqSelect(3);
    sgtl5000_1.eqBands(0,-0.4,-0.5,-0.4,-0.2); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz
    //sgtl5000_1.eqBands(0,0,0,0,0); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz

    //EQ for Mic
    sgtl5000_2.eqSelect(3);
    sgtl5000_2.eqBands(0,0,0,0,0);
    
    //Deep focus mode; mode 0
    //if (news==1) {
    Serial.println("Deep focus activated");
    news = 0;
    
    //Adjust gain mix
    setupGain(maxmusicvol,0,0);
  }
/*
  if (mode == 1 && news == 1) {
    //conversation; mode1
    //notes: get mics to percieve conversation range with EQ, apply inv.vocal boost on music

      //EQ for Music
      sgtl5000_1.eqSelect(3);
      sgtl5000_1.eqBands(1,-1,-1,-1,0.7); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz
  
      //EQ for Mic
      sgtl5000_2.eqSelect(3);
      sgtl5000_2.eqBands(0,0,0,0,0);
     
      Serial.println("Conversation activated - normal");
      news = 0;

     //set gain mix
     setupGain(0.05,0.065,0);
      

  }
*/

  
  if (mode == 1 && news == 1) {
    //conversation; mode1
    //notes: get mics to percieve conversation range with EQ, apply inv.vocal boost on music

      //EQ for Music
      sgtl5000_1.eqSelect(3);
      sgtl5000_1.eqBands(0,-0.4,-0.5,-0.4,-0.2); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz
  
      //EQ for Mic
      sgtl5000_2.eqSelect(3);
      sgtl5000_2.eqBands(-0.8,0.5,0,-0.2,-0.4);
     
      Serial.println("Conversation activated - with bi-quad");
      news = 0;

     //set gain mix
     setupGain(0,0.09,0.25);//0,0.75,0.3

  }

  if (mode == 2 && news == 1) {
    //Awareness mode; mode 2
    //notes: get mics to percieve what's going on around, no EQ on music,
    //add EQ on mic for most natural surround sound

    //EQ for Music
    sgtl5000_1.eqSelect(3);
    sgtl5000_1.eqBands(0,-0.4,-0.5,-0.4,-0.2); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz
    //sgtl5000_1.eqBands(0,0,0,0,0); //115Hz, 330Hz, 990Hz, 3kHz, 8.8kHz

    //EQ for Mic
    sgtl5000_2.eqSelect(3);
    sgtl5000_2.eqBands(0,0.5,0,-0.2,-0.2);

    Serial.println("Awareness mode activated");
    news = 0;

    //set gain mix
    setupGain(0.4,0.08,0);//0,.075
  }

  //Serial.println("working");
  //delay(100);
  if (mode > 2) {
    mode = 0;
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
}
