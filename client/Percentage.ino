// Rotary encoder 1
#define DT_PIN_1 19   // <- interrupt pin
#define CLK_PIN_1 4
#define SW_PIN_1 3

// Rotary encoder 2
#define DT_PIN_2 2    // <- interrupt pin
#define CLK_PIN_2 5
#define SW_PIN_2 18

// Mute LED
#define MUTE_PIN 17

// I2C Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

// Includes
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>

// Percentage Bar Settings
int BAR_STARTX = 16;
int BAR_STARTY = 24;
int BAR_WIDTH = SCREEN_WIDTH - 2 * BAR_STARTX;
int BAR_HEIGHT = 16;
int BAR_RADIUS = 3;
int lastProcButton = LOW;
int lastVolButton = LOW;

// Label Settings
int LABEL_STARTX = 4;
int LABEL_STARTY = 4;

// Button Settings
unsigned long BUTTON_PRESS_COOLDOWN = 500;
unsigned long lastVolButtonPress = 0;
unsigned long lastProcButtonPress = 0;

// Waiting Screen Settings
#define VOLUME_MIXER_NAME "Volume Mixer"
#define WAITING_TEXT "\n\nWaiting for \nconnection.."
int WS_STARTX = 16;

// Navigation Bar Settings
int NAV_CIRCLE_RADIUS = 2;
int NAV_DIST = 6;
int NAV_STARTY = 58;

// Rotary Encoder Settings (Volume / Processes)
int VOL_MAX = 100;
int VOL_MIN = 0;
int VOL_STEP = 5;
int oldVolPos = -999;
int PROC_MAX = 100;
int PROC_MIN = 0;
int PROC_STEP = 1;
int oldProcPos = -999;

// Serial communication Settings
#define READING_NAME  1
#define READING_VALUE 0
#define BAUD_RATE 115200
bool fetchNewProcessData = false;
bool notifyNewMute = false;


// Semantic values
#define MAX_NUM_OF_PROCESSES 16
#define MAX_PROCESS_NAME_LEN 16
int  currentProcess = 0;
char processNames[MAX_NUM_OF_PROCESSES][MAX_PROCESS_NAME_LEN + 1];
int  processValues[MAX_NUM_OF_PROCESSES];
bool processMuted[MAX_NUM_OF_PROCESSES];
int  N = 3;
int  currentValue = 0;


Encoder volumeEncoder(DT_PIN_1, CLK_PIN_1);
Encoder processEncoder(DT_PIN_2, CLK_PIN_2);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup(){
  // Start serial connection
  Serial.begin(BAUD_RATE);
  while (!Serial);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }

  waitingForConnectionDisplay();

  // Wait for H (Hello) from Computer
  while (Serial.available() == 0);
  Serial.read();

  pinMode(MUTE_PIN, OUTPUT);

  pinMode(SW_PIN_1, INPUT);
  attachInterrupt(digitalPinToInterrupt(SW_PIN_1), volButtonPressed, CHANGE);
  pinMode(SW_PIN_2, INPUT);
  attachInterrupt(digitalPinToInterrupt(SW_PIN_2), procButtonPressed, CHANGE);

  for (int i = 0; i < MAX_NUM_OF_PROCESSES; i++) processMuted[i] = false;
  getProcessData();

  updateDisplay();
}

void loop(){
  if (fetchNewProcessData){ getProcessData(); updateDisplay(); fetchNewProcessData = false; }
  if (notifyNewMute){ setProcessMute(); notifyNewMute = false; }
  
  // Read new values from the process rotary encoder
  long newProcPos = (processEncoder.read() / 4) * PROC_STEP;
 
  if (newProcPos < PROC_MIN){
    processEncoder.write((PROC_MIN * 4) / PROC_STEP);
  } else if (newProcPos > N - 1){
    processEncoder.write(((N - 1) * 4) / PROC_STEP);
  } else if (newProcPos != oldProcPos){
    oldProcPos = newProcPos;
    changeProcess(newProcPos);
    updateDisplay();
  }

  
  // Read new values from the volume rotary encoder
  long newVolPos = (volumeEncoder.read() / 4) * VOL_STEP;

  if (newVolPos < VOL_MIN){
    volumeEncoder.write((VOL_MIN * 4) / VOL_STEP);
  } else if (newVolPos > VOL_MAX){
    volumeEncoder.write((VOL_MAX * 4) / VOL_STEP);
  } else if (newVolPos != oldVolPos){
    oldVolPos = newVolPos;
    currentValue = newVolPos;
    setProcessVolume();
    updateDisplay();
  }
}

void updateDisplay(){
  display.clearDisplay();
  drawPercentage((int) currentValue);
  char* processName = processNames[currentProcess];
  drawProcessName(processName);
  drawNavigationBar(currentProcess);
  display.display();
}

void drawProcessName(const char* processName){
  int16_t cursorX, cursorY;
  uint16_t stringWidth, stringHeight;
  display.getTextBounds(processName, LABEL_STARTX, LABEL_STARTY, &cursorX, &cursorY, &stringWidth, &stringHeight);

  int centerStartX = (SCREEN_WIDTH - stringWidth) / 2;
  display.setCursor(centerStartX, LABEL_STARTY);
  display.setTextColor(1);
  display.print(processName);
}

void drawPercentage(int value){
  int BORDER_STARTX = BAR_STARTX - 2;
  int BORDER_STARTY = BAR_STARTY - 2;
  int BORDER_WIDTH  = BAR_WIDTH + 4;
  int BORDER_HEIGHT = BAR_HEIGHT + 4;
  display.drawRoundRect(BORDER_STARTX, BORDER_STARTY, BORDER_WIDTH, BORDER_HEIGHT, BAR_RADIUS, 1);
  int width = (BAR_WIDTH * value) / 100;
  display.fillRoundRect(BAR_STARTX, BAR_STARTY, width, BAR_HEIGHT, BAR_RADIUS, 1);
}

void drawNavigationBar(int selected){
  
  int triangleWidth = 2 * NAV_CIRCLE_RADIUS;
  int triangleHeight = NAV_CIRCLE_RADIUS;
  int circleDistance = NAV_DIST;
  int triangleDistance = 1.5 * NAV_DIST;
  int navBarWidth  = N * 2 * NAV_CIRCLE_RADIUS + (N - 1) * circleDistance + 2 * triangleDistance + 2 * triangleWidth;
  int navBarHeight = 2 * NAV_CIRCLE_RADIUS;
  int navBarX = (SCREEN_WIDTH - navBarWidth) / 2;
  int navBarY = NAV_STARTY;

  // Left Triangle
  int ltX0 = navBarX; int ltY0 = navBarY + navBarHeight / 2;
  int ltX1 = navBarX + triangleWidth; int ltY1 = navBarY;
  int ltX2 = navBarX + triangleWidth; int ltY2 = navBarY + navBarHeight;
  display.drawTriangle(ltX0, ltY0, ltX1, ltY1, ltX2, ltY2, 1);

  // Draw Circles
  for (int i = 0; i < N; i++){
    // i-th Circle
    int circleX = navBarX + triangleWidth + triangleDistance + NAV_CIRCLE_RADIUS + i * (2 * NAV_CIRCLE_RADIUS + circleDistance);
    int circleY = navBarY + navBarHeight / 2;
    if (selected == i){
      display.fillCircle(circleX, circleY, NAV_CIRCLE_RADIUS, 1);
    } else{
      display.drawCircle(circleX, circleY, NAV_CIRCLE_RADIUS, 1);
    }
  }

  // Right Triangle
  int rtX0 = navBarX + navBarWidth; int rtY0 = navBarY + navBarHeight / 2;
  int rtX1 = navBarX + navBarWidth - triangleWidth; int rtY1 = navBarY;
  int rtX2 = navBarX + navBarWidth - triangleWidth; int rtY2 = navBarY + navBarHeight;
  display.drawTriangle(rtX0, rtY0, rtX1, rtY1, rtX2, rtY2, 1);
  
}

void waitingForConnectionDisplay(){
  display.clearDisplay();
  display.setCursor(0, 16);
  display.setTextColor(1);
  display.setTextSize(1);
  display.print(VOLUME_MIXER_NAME);
  display.setTextSize(1);
  display.print(WAITING_TEXT);
  display.display();
}

void changeProcess(int i){
  
  if (i >= N) return;
  // Store previous process value
  processValues[currentProcess] = currentValue;

  // Load values for new process
  currentProcess = i;
  currentValue = processValues[i];
  volumeEncoder.write((currentValue * 4) / VOL_STEP);
  if (processMuted[currentProcess]){
    digitalWrite(MUTE_PIN, HIGH);
  } else {
    digitalWrite(MUTE_PIN, LOW);
  }
}

void volButtonPressed(){
  if (millis() - lastVolButtonPress < BUTTON_PRESS_COOLDOWN) return;
  lastVolButtonPress = millis();
  notifyNewMute = true;
}

void procButtonPressed(){
  if (millis() - lastProcButtonPress < BUTTON_PRESS_COOLDOWN) return;
  lastProcButtonPress = millis();
  fetchNewProcessData = true;
}

void getProcessData(){
  /*
   * 'GET_PD' triggers a response containing all current process names and their corresponding values.
   * The response is formatted as follows: PROCESS_NAME1%PROCESS_VALUE1&PROCESS_NAME2%PROCESS_VALUE2&$
   */
  Serial.println("GET_PD");
  char nameBuf[MAX_PROCESS_NAME_LEN + 1];
  char valueBuf[3 + 1];                       // Maximum number of chars for the volume value (Range from 0 (1) to 100 (3))
  char bufPointer = 0;                        // Points to the next free char byte in the buffer
  char processPointer = 0;                    // Points to the next free process slot in processNames and processValues array
  char currentlyReading = READING_NAME;       // Either READING_NAME or READING_VALUE
  while (true) {
    while (Serial.available() == 0);
    char c = Serial.read();
    
    if (c == '&'){ // End of process value transmission
      valueBuf[bufPointer] = '\0';
      int processValue = atoi(valueBuf);
      processValues[processPointer] = processValue;
      strcpy(processNames[processPointer], nameBuf);

      processPointer++;
      bufPointer = 0;
      currentlyReading = READING_NAME;
      
    } else if (c == '%'){ // End of process name transmission
      nameBuf[bufPointer] = '\0';

      bufPointer = 0;
      currentlyReading = READING_VALUE;

    } else if (c == '$'){ // End of transmission
      break;
    
    } else {
      if (currentlyReading == READING_NAME){
        if (bufPointer >= MAX_PROCESS_NAME_LEN){
          Serial.print("Buffer overflow while receiving name for process no.");
          Serial.println(processPointer);
          return;
        }
        nameBuf[bufPointer] = c;
        bufPointer++;
      } else {
        if (bufPointer >= 3){
          Serial.print("Buffer overflow while receiving values for process no.");
          Serial.print(processPointer);
          Serial.print(" with name: ");
          Serial.println(nameBuf);
          return;
        }
        valueBuf[bufPointer] = c;
        bufPointer++;
      }
    }
  }
  N = processPointer;

  // Update the current values
  currentProcess = 0;
  currentValue = processValues[0];
}

void setProcessVolume(){
  Serial.print("SET_PV&");
  Serial.print(processNames[currentProcess]);
  Serial.print("%");
  Serial.println(currentValue);
}

void setProcessMute(){
  processMuted[currentProcess] = !processMuted[currentProcess];
  if (processMuted[currentProcess]){
    digitalWrite(MUTE_PIN, HIGH);
  } else {
    digitalWrite(MUTE_PIN, LOW);
  }
  Serial.print("MUTE_PROC&");
  Serial.print(processNames[currentProcess]);
  Serial.print("%");
  Serial.println(processMuted[currentProcess]);
}
