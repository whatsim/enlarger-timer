// turn off display
// delay on start time on run

#include <Wire.h>
#include <MD_REncoder.h>

#define ROTARY_PIN1 2
#define ROTARY_PIN2 3

#define RUN_BUTTON_PIN 6
#define TIMER_BUTTON_PIN 5
#define PREVIEW_BUTTON_PIN 4
#define POWER_PIN 8

#define DISPLAY_ADDRESS   0x70

#define HT16K33_CMD_BRIGHTNESS 0xE0 ///< I2C register for BRIGHTNESS setting
#define HT16K33_BLINK_CMD 0x80
#define HT16K33_BLINK_DISPLAYON 0x01 ///< I2C value for steady on
#define HT16K33_BLINK_OFF 0          ///< I2C value for steady off
#define HT16K33_BLINK_2HZ 1          ///< I2C value for 2 Hz blink
#define HT16K33_BLINK_1HZ 2          ///< I2C value for 1 Hz blink
#define HT16K33_BLINK_HALFHZ 3       ///< I2C value for 0.5 Hz blink



static const uint8_t numbertable[] = {
    0x3F, /* 0 */
    0x06, /* 1 */
    0x5B, /* 2 */
    0x4F, /* 3 */
    0x66, /* 4 */
    0x6D, /* 5 */
    0x7D, /* 6 */
    0x07, /* 7 */
    0x7F, /* 8 */
    0x6F, /* 9 */
    0x77, /* a */
    0x7C, /* b */
    0x39, /* C */
    0x5E, /* d */
    0x79, /* E */
    0x71, /* F */
};

uint16_t displaybuffer[8];
bool hasSetPowerToHigh = false;
bool hasToggledDisplayThisPress = false;
bool displayOn = true;

struct ButtonState {
   unsigned long changeTime;
   int state = LOW;
   int readState = LOW;
   int pin;
};

long debounceDelay = 50;
long preRunDuration = 2;

struct ButtonState runButton;
struct ButtonState previewButton;
struct ButtonState timerButton;

enum mode {
  setting,
  running,
  preview,
  timerRunning
};

MD_REncoder encoder = MD_REncoder(ROTARY_PIN1, ROTARY_PIN2);
volatile uint8_t encoderDelta = 0;

unsigned long startTime = 0;
float duration = 5;

mode currentMode = setting;

void setup() {
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN1), rotInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN2), rotInterrupt, CHANGE);

  runButton.pin = RUN_BUTTON_PIN;
  previewButton.pin = PREVIEW_BUTTON_PIN;
  timerButton.pin = TIMER_BUTTON_PIN;
  pinMode(previewButton.pin, INPUT_PULLUP);
  pinMode(runButton.pin, INPUT_PULLUP);
  pinMode(timerButton.pin, INPUT_PULLUP);
  timerButton.changeTime = 0;
  
 
  pinMode(POWER_PIN, OUTPUT);

  Serial.begin(9600);
  Wire.begin();

  changeMode(currentMode);
  encoder.begin();

  initDisplay();
}


void loop() {

  switch(currentMode){
    case setting:
    case preview:
      setFunction();
    break;
    case timerRunning:
    case running:
      runFunction();
    break;
    
  }

  unsigned long now = millis();
  unsigned long timerButtonAge = now - timerButton.changeTime;
  
  if(timerButtonAge > 1000 && timerButton.state == LOW && !hasToggledDisplayThisPress){
    toggleDisplay();
    hasToggledDisplayThisPress = true;
    changeMode(setting);
  }
  
  writeDisplay();

}

mode changeMode(mode m){
  if(currentMode != m){
    switch(m){
      case setting:
        digitalWrite(POWER_PIN,LOW);
      break;
      case preview:
        digitalWrite(POWER_PIN,HIGH);
      break;
      case timerRunning:
        digitalWrite(POWER_PIN,LOW);
        startTime = millis();
      break;
      case running:
        digitalWrite(POWER_PIN,LOW);
        hasSetPowerToHigh = false;
        startTime = millis();
      break;
      
    }
    currentMode = m;
  }
}


int updateButton(ButtonState *state){
  int readState = digitalRead(state->pin);
  unsigned long now = millis();
  if (readState != state->readState) {
    state->changeTime = now;
  }
  state->readState = readState;
  if(now - state->changeTime > debounceDelay) {
    if(readState != state->state){
      state->state = readState;
      return readState;
    }
  }
  return -1;
}


void writeTimeToBuffer(long t){
  int tenthSeconds  = floor(t / 100);
  int seconds = floor(t / 1000);
  int tenSeconds = floor(t / 10000);
  int hundredSeconds = floor(t / 100000);

  writeDigitToBuffer(4, tenthSeconds % 10, false);
  writeDigitToBuffer(3, seconds % 10, true);
  displaybuffer[2] = 0;
  if(tenSeconds % 10 != 0 || hundredSeconds % 10 != 0){
    writeDigitToBuffer(1, tenSeconds % 10,false);
  } else {
    displaybuffer[1] = 0;
  }
  if(hundredSeconds % 10 != 0){
    writeDigitToBuffer(0, hundredSeconds % 10,false);
  } else {
    displaybuffer[0] = 0;
  }
  
}


void setFunction(){

  // toggle preview mode
  int previewButtonEdge = updateButton(&previewButton);
  if(previewButtonEdge == LOW){
    if(currentMode == preview){
      changeMode(setting);
    } else {
      changeMode(preview);
    }
  }
  
  int timerButtonEdge = updateButton(&timerButton);
  if(timerButtonEdge == LOW){
    if(currentMode == timerRunning){
      changeMode(setting);
    } else if(currentMode == setting){
      changeMode(timerRunning);
    }
  }
  if(timerButton.state == HIGH){
    hasToggledDisplayThisPress = false;
  }
  

  // change time
  
  if(encoderDelta == DIR_CW){
    duration -= 0.5;
  } else if(encoderDelta == DIR_CCW){
    duration += 0.5;
  }
  if(duration <= 0){
    duration = 0;
  }
  if(duration > 120){
    duration = 120;
  }

  // start running
  int runButtonEdge = updateButton(&runButton);
  if(runButtonEdge == LOW){
    changeMode(running);    
  }

  encoderDelta = 0;
  // display time
//  Serial.println(duration * 1000);
  writeTimeToBuffer(long(duration * 1000));
}

void runFunction(){
  // check it run has been pressed again to cancel
  int runButtonEdge = updateButton(&runButton);
  int timerButtonEdge = updateButton(&timerButton);
  if(runButtonEdge == LOW || timerButtonEdge == LOW){
    changeMode(setting); 
  }

  long modifiedDuration = duration;
  if(currentMode == running){
    modifiedDuration += preRunDuration;
  } else if (currentMode == timerRunning){
    modifiedDuration = 60;
  }

  // end if duration complete
  long currentTime = millis() - startTime;
  // allow 3 second run up
  if(currentTime > preRunDuration * 1000 && !hasSetPowerToHigh && currentMode == running){
    digitalWrite(POWER_PIN,HIGH);
    hasSetPowerToHigh = true;
  }
  if(currentTime > long(modifiedDuration* 1000)){
     changeMode(setting);    
  }

  // display time
  if(currentMode == running && currentTime < preRunDuration * 1000){
    writeTimeToBuffer((preRunDuration * 1000) - currentTime);
  } else {
    writeTimeToBuffer((modifiedDuration * 1000) - currentTime);
  }
}

void writeDigitToBuffer(uint8_t d, uint8_t num, bool dot){
  displaybuffer[d] = numbertable[num] | (dot << 7);
}

void writeDisplay(){
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write((uint8_t)0x00); // start at address $00
  for (uint8_t i = 0; i < 8; i++) {
    Wire.write(displaybuffer[i] & 0xFF);
    Wire.write(displaybuffer[i] >> 8);
  }
  Wire.endTransmission();
}

void setBrightness(int b){
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(HT16K33_CMD_BRIGHTNESS | b);
  Wire.endTransmission();
}

void clearBuffer() {
  for (uint8_t i = 0; i < 8; i++) {
    displaybuffer[i] = 0;
  }
}

void initDisplay(){
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(0x21); // turn on oscillator
  Wire.endTransmission();
  clearBuffer();
  writeDisplay();
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(HT16K33_CMD_BRIGHTNESS | 1);
  Wire.endTransmission();
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON);
  Wire.endTransmission();
  
  
  
}
void toggleDisplay(){
  if(displayOn){
    Wire.beginTransmission(DISPLAY_ADDRESS);
    Wire.write(HT16K33_BLINK_CMD | HT16K33_BLINK_OFF);
    Wire.endTransmission();
  } else {
    Wire.beginTransmission(DISPLAY_ADDRESS);
    Wire.write(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON);
    Wire.endTransmission();
  }
  displayOn = !displayOn;
}
void rotInterrupt(){
  encoderDelta = encoder.read();
}
