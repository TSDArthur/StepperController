/* Stepper.ino
 *
 * Created:   Thu March 16 2018
 * Processor: ATmega2560
 * Compiler:  Arduino AVR
 * Author:  Zhang Muhua
 *
 */
#include<FlexiTimer2.h>
#include<TimerThree.h>
#define MAX_SPEED 15
#define NOR_SPEED 15
#define NOR_DIR FWD
#define ALL_KEY_UP 'N'
#define KEY_WAIT_TIME 4
#define TONE_WAIT_TIME 500
#define DIGDIS_DELAY_TIME 5
#define PIN_BUZZER 35
#define NONE_DISPLAY 15
#define DISPLAY_REFRESH_TIME 30000
#define DISPLAY_OFF -1
#define MAX_PROG_NUM 9
#define NOR_STEPSET 64
#define KEY_SPEED 'A'
#define KEY_STEPCOUNT 'B'
#define KEY_DIRECTION 'C'
#define KEY_NEXT 'D'
#define KEY_START 'E'
#define KEY_RECOGNIZE 'F'
#define KEY_MODENORMAL 'G'
#define KEY_MODEPROG 'H'
#define KEY_STOP 'I'
#define KEY_TEST 'J'
#define KEY_DELAY_TIME 250
enum Direction {FWD, BWD};
enum RunState {ON, OFF};
enum ControlState {READY, RUNNING, PROG, NORMAL, TEST, SET_NORMAL, SET_PROG};
enum SetState {HOME, SPEED, STEPCOUNT, DIRECTION};
class Stepper
{
private :
  //properties
  const u8 stepperBeats[4][5] = {
    {1, 1, 0, 1, 0},
    {1, 0, 1, 1, 0},
    {1, 0, 1, 0, 1},
    {1, 1, 0, 0, 1}
  };
  const u32 speedTable[15] = {3400, 3200, 3000, 2800, 2600, 2400, 2200, 2000, 1800, 1600, 1400, 1200, 1000, 800, 600};
  u8 stepperPin[5];
public :
  enum RunState stepperOn;
  enum Direction stepperDirection;
  u32 stepCount;
  u8 stepperSpeed;
  int stepBeatNum;
  u32 stepperSpeedCoverted;
  enum Direction lastDirection;
  bool stepDirModify;
  Stepper(u8 pin0, u8 pin1, u8 pin2, u8 pin3, u8 pin4, enum Direction stepDir, u8 stepSpd)
  {
    //set pins
    pinMode(stepperPin[0] = pin0, OUTPUT);
    pinMode(stepperPin[1] = pin1, OUTPUT);
    pinMode(stepperPin[2] = pin2, OUTPUT);
    pinMode(stepperPin[3] = pin3, OUTPUT);
    pinMode(stepperPin[4] = pin4, OUTPUT);
    //set Dirction
    stepperDirection = stepDir;
    lastDirection = stepDir;
    //set Speed
    stepperSpeed = stepSpd > 0 ? min(MAX_SPEED, stepSpd) : NOR_SPEED;
    //cover speed value
    stepperSpeedCoverted = SpeedCovert(stepperSpeed);
    //reset stepper step count
    stepCount = 0;
    stepBeatNum = stepperDirection == FWD ? 0 : 3;
    stepDirModify = false;
    //set timer
    FlexiTimer2::set(stepperSpeedCoverted, StepperBeating);
  }
public:
  void Start()
  {
    //set state flag
    stepperOn = ON;
    //set timer
    FlexiTimer2::start();
  }

  void Stop()
  {
    //set state flag
    stepperOn = OFF;
    FlexiTimer2::stop();
  }

  void SetSpeed(u8 stepSpd)
  {
    //stop stepper
    Stop();
    //set Speed
    stepperSpeed = stepSpd > 0 ? min(MAX_SPEED, stepSpd) : NOR_SPEED;
    //cover speed value
    stepperSpeedCoverted = SpeedCovert(stepperSpeed);
    //set timer
    FlexiTimer2::set(stepperSpeedCoverted, StepperBeating);
  }

  void SetDirection(enum Direction stepDir)
  {
    //stop stepper
    Stop();
    //set Dirction
    lastDirection = stepperDirection;
    stepperDirection = stepDir;
    if (lastDirection != stepperDirection)stepDirModify = true;
    else stepDirModify = false;
  }

  void ResetBeatCounts()
  {
    //stop stepper
    Stop();
    //reset stepper step count
    stepCount = 0;
  }

  void Reset()
  {
    //stop stepper
    Stop();
    //set values
    SetSpeed(NOR_SPEED);
    //SetDirection(NOR_DIR);
    ResetBeatCounts();
  }
public:
  void Beats()
  {
    if (stepperDirection != lastDirection && stepDirModify)
    {
      if (stepperDirection == FWD)
      {
        stepBeatNum++;
        stepBeatNum = stepBeatNum > 3 ? 0 : stepBeatNum;
        stepBeatNum++;
        stepBeatNum = stepBeatNum > 3 ? 0 : stepBeatNum;
      }
      else
      {
        stepBeatNum--;
        stepBeatNum = stepBeatNum < 0 ? 3 : stepBeatNum;
        stepBeatNum--;
        stepBeatNum = stepBeatNum < 0 ? 3 : stepBeatNum;
      }
      stepDirModify = false;
    }
    //set pins
    for (int i = 0; i < 5; i++)
      digitalWrite(stepperPin[i], stepperBeats[stepBeatNum][i]);
    //add beat num
    stepCount++;
    if (stepperDirection == FWD)stepBeatNum++;
    else stepBeatNum--;
    stepBeatNum = stepBeatNum < 0 ? 3 : (stepBeatNum > 3 ? 0 : stepBeatNum);
  }

  u32 SpeedCovert(u8 stepSpd)
  {
    //linear
    return speedTable[stepSpd - 1];
  }
};

class Keypad
{
private:
  u8 colPin[5], rowPin[4];
  const u8 keyMap[5][4] = {{1, 2, 3, 'A'}, {4, 5, 6, 'B'},
    {7, 8, 9, 'C'}, {'D', 0, 'E', 'F'},
    {'G', 'H', 'I', 'J'}
  };
public:
  Keypad(u8 pin_col_0, u8 pin_col_1, u8 pin_col_2, u8 pin_col_3, u8 pin_col_4, u8 pin_row_0, u8 pin_row_1, u8 pin_row_2, u8 pin_row_3)
  {
    //set col pins
    pinMode(colPin[0] = pin_col_0, OUTPUT);
    pinMode(colPin[1] = pin_col_1, OUTPUT);
    pinMode(colPin[2] = pin_col_2, OUTPUT);
    pinMode(colPin[3] = pin_col_3, OUTPUT);
    pinMode(colPin[4] = pin_col_4, OUTPUT);
    //set row pins
    pinMode(rowPin[0] = pin_row_0, OUTPUT);
    pinMode(rowPin[1] = pin_row_1, OUTPUT);
    pinMode(rowPin[2] = pin_row_2, OUTPUT);
    pinMode(rowPin[3] = pin_row_3, OUTPUT);
    //prepration
    KeypadReset();
  }
private:
  void KeypadReset()
  {
    //set col pins
    pinMode(colPin[0], OUTPUT);
    pinMode(colPin[1], OUTPUT);
    pinMode(colPin[2], OUTPUT);
    pinMode(colPin[3], OUTPUT);
    pinMode(colPin[4], OUTPUT);
    //set col pins output
    digitalWrite(colPin[0], HIGH);
    digitalWrite(colPin[1], HIGH);
    digitalWrite(colPin[2], HIGH);
    digitalWrite(colPin[3], HIGH);
    digitalWrite(colPin[4], HIGH);
    //set row pins
    pinMode(rowPin[0], INPUT_PULLUP);
    pinMode(rowPin[1], INPUT_PULLUP);
    pinMode(rowPin[2], INPUT_PULLUP);
    pinMode(rowPin[3], INPUT_PULLUP);
  }
public:
  u8 GetKey()
  {
    KeypadReset();
    for (int i = 0; i < 5; i++)
    {
      digitalWrite(colPin[i], LOW);
      for (int j = 0; j < 4; j++)
      {
        if (digitalRead(rowPin[j]) == LOW)
        {
          delay(KEY_WAIT_TIME);
          if (digitalRead(rowPin[j]) == LOW)
          {
            //Key press
            //Tone start
            tone(PIN_BUZZER, 5000);
            delay(KEY_WAIT_TIME);
            //Tone end
            noTone(PIN_BUZZER);
            return keyMap[i][j];
          }
        }
      }
      digitalWrite(colPin[i], HIGH);
    }
    return ALL_KEY_UP;
  }
};

class DigitalDisplay
{
private:
  u8 pinDisplay[12];
  u8 displayMap[16][8] = {{LOW, LOW, LOW, LOW, LOW, LOW, HIGH, HIGH}, {HIGH, LOW, LOW, HIGH, HIGH, HIGH, HIGH, HIGH},
    {LOW, LOW, HIGH, LOW, LOW, HIGH, LOW, HIGH}, {LOW, LOW, LOW, LOW, HIGH, HIGH, LOW, HIGH},
    {HIGH, LOW, LOW, HIGH, HIGH, LOW, LOW, HIGH}, {LOW, HIGH, LOW, LOW, HIGH, LOW, LOW, HIGH},
    {LOW, HIGH, LOW, LOW, LOW, LOW, LOW, HIGH}, {LOW, LOW, LOW, HIGH, HIGH, HIGH, HIGH, HIGH},
    {LOW, LOW, LOW, LOW, LOW, LOW, LOW, HIGH}, {LOW, LOW, LOW, LOW, HIGH, LOW, LOW, HIGH},
    {LOW, HIGH, LOW, LOW, HIGH, LOW, LOW, HIGH}, {LOW, LOW, HIGH, HIGH, LOW, LOW, LOW, HIGH},
    {LOW, LOW, LOW, HIGH, LOW, LOW, LOW, HIGH}, {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW, HIGH},
    {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW, HIGH}, {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}
  };
  //0-0 1-1 2-2 3-3 4-4 5-5 6-6 7-7 8-8 9-9 10-S 11-P 12-A 13-E 14--
public:
  u8 disContent[4];
  u8 nowDisplayPos;
public:
  DigitalDisplay(u8 pin0, u8 pin1, u8 pin2, u8 pin3, u8 pin4, u8 pin5, u8 pin6, u8 pin7, u8 pin8, u8 pin9, u8 pin10, u8 pin11)
  {
    //set pins
    pinMode(pinDisplay[0] = pin0, OUTPUT);
    pinMode(pinDisplay[1] = pin1, OUTPUT);
    pinMode(pinDisplay[2] = pin2, OUTPUT);
    pinMode(pinDisplay[3] = pin3, OUTPUT);
    pinMode(pinDisplay[4] = pin4, OUTPUT);
    pinMode(pinDisplay[5] = pin5, OUTPUT);
    pinMode(pinDisplay[6] = pin6, OUTPUT);
    pinMode(pinDisplay[7] = pin7, OUTPUT);
    pinMode(pinDisplay[8] = pin8, OUTPUT);
    pinMode(pinDisplay[9] = pin9, OUTPUT);
    pinMode(pinDisplay[10] = pin10, OUTPUT);
    pinMode(pinDisplay[11] = pin11, OUTPUT);
    //reset pos
    nowDisplayPos = 0;
  }

  void Display()
  {
    nowDisplayPos = nowDisplayPos == 4 ? 0 : nowDisplayPos;
    //Clear();
    for (int i = 0; i < 4; i++)
    {
      if (i != nowDisplayPos)digitalWrite(pinDisplay[i], LOW);
      else digitalWrite(pinDisplay[i], HIGH);
    }
    for (int i = 4; i < 12; i++)
      digitalWrite(pinDisplay[i], displayMap[ContentCovert(disContent[nowDisplayPos])][i - 4]);
    nowDisplayPos++;
  }

  void Clear()
  {
    for (int i = 4; i < 12; i++)
      digitalWrite(pinDisplay[i], HIGH);
  }
private:
  u8 ContentCovert(u8 disContent)
  {
    if (disContent == 'S')return 10;
    else if (disContent == 'P')return 11;
    else if (disContent == 'A')return 12;
    else if (disContent == 'E')return 13;
    else if (disContent == '-')return 14;
    else return disContent;
  }
};

//Main Struct
struct Control
{
  enum Direction stepperDirection;
  u8 stepperSpeed;
  u32 stepperSetSteps;
  bool setDir;
  bool setSpd;
  bool setSteps;
};

//setup stepper
Stepper Stepper_motor(36, 8, 9, 10, 11, FWD, NOR_SPEED);
//setup keypad
Keypad Matrix_keypad(26, 27, 28, 29, 30, 31, 32, 33, 34);
//setup digitalDisplay
DigitalDisplay Digital_display(6, 7, 8, 9, 10, 11, 12, 13, 22, 23, 24, 25);
//set display mode
int lastShowNumber = -1;
u8 displayMode = 2;
u32 displayNumber = 0;
enum ControlState CurrentState;
Control progList[MAX_PROG_NUM];
u8 progNum = 0;
bool inProc = false;
u8 nowProg = 0;
enum SetState CurrentSetState;
int nowEnterValue = -1;

void Display_Number(u32 numIn)
{
  if (numIn == lastShowNumber)return;
  if (numIn < 10)
  {
    Digital_display.disContent[0] = NONE_DISPLAY;
    Digital_display.disContent[1] = NONE_DISPLAY;
    Digital_display.disContent[2] = NONE_DISPLAY;
    Digital_display.disContent[3] = numIn;
  }
  else if (numIn < 100)
  {
    Digital_display.disContent[0] = NONE_DISPLAY;
    Digital_display.disContent[1] = NONE_DISPLAY;
    Digital_display.disContent[2] = numIn / 10;
    Digital_display.disContent[3] = numIn % 10;
  }
  else if (numIn < 1000)
  {
    Digital_display.disContent[0] = NONE_DISPLAY;
    Digital_display.disContent[1] = numIn / 100;
    Digital_display.disContent[2] = (numIn / 10) % 10;
    Digital_display.disContent[3] = numIn % 10;
  }
  else if (numIn < 10000)
  {
    Digital_display.disContent[0] = numIn / 1000;
    Digital_display.disContent[1] = (numIn / 100) % 10;
    Digital_display.disContent[2] = (numIn / 10) % 10;
    Digital_display.disContent[3] = numIn % 10;
  }
  else
  {
    Digital_display.disContent[0] = 9;
    Digital_display.disContent[1] = 9;
    Digital_display.disContent[2] = 9;
    Digital_display.disContent[3] = 9;
  }
  lastShowNumber = numIn;
}

void StepperBeating()
{
  Stepper_motor.Beats();
  //displayMode = 0;
  //displayNumber = Stepper_motor.stepCount;
  if (Stepper_motor.stepCount == 4096)
    Stepper_motor.Stop();
}

void DisplayShowing()
{
  if (displayMode == 0)Display_Number(displayNumber);
  else if (displayMode == 1)lastShowNumber = DISPLAY_OFF;
  if (displayMode != 2)Digital_display.Display();
  else Digital_display.Clear();
}

void setup()
{
  //setup buzzer
  pinMode(PIN_BUZZER, OUTPUT);
  //test#1
  //Stepper_motor.Start();
  //set display
  Timer3.initialize(DISPLAY_REFRESH_TIME);
  Timer3.attachInterrupt(DisplayShowing);
  CurrentState = READY;
  nowProg = 10;
  Stepper_motor.Stop();
  Stepper_motor.Reset();
  Stepper_motor.SetSpeed(NOR_SPEED);
  Stepper_motor.SetDirection(BWD);
  Stepper_motor.Start();
}

void loop()
{
  u8 keyPress = Matrix_keypad.GetKey();
  //key event
  if (keyPress == KEY_STOP)
  {
    CurrentState = READY;
    inProc = false;
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_TEST)
  {
    if (CurrentState == TEST)
    {
      CurrentState = READY;
      inProc = false;
    }
    else if (CurrentState == READY)
    {
      CurrentState = TEST;
      inProc = false;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_MODENORMAL)
  {
    if (CurrentState == READY)
    {
      CurrentState = SET_NORMAL;
      CurrentSetState = HOME;
      nowEnterValue = -1;
      inProc = false;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_MODEPROG)
  {
    if (CurrentState == READY)
    {
      CurrentState = SET_PROG;
      CurrentSetState = HOME;
      nowEnterValue = -1;
      inProc = false;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_SPEED)
  {
    if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && CurrentSetState != SPEED)
    {
      nowEnterValue = -1;
      CurrentSetState = SPEED;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_STEPCOUNT)
  {
    if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && CurrentSetState != STEPCOUNT)
    {
      nowEnterValue = -1;
      CurrentSetState = STEPCOUNT;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_DIRECTION)
  {
    if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && CurrentSetState != DIRECTION)
    {
      nowEnterValue = -1;
      CurrentSetState = DIRECTION;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress >= 0 && keyPress <= 9)
  {
    if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && (CurrentSetState != HOME))
    {
      if (nowEnterValue == -1)nowEnterValue = keyPress;
      else if (nowEnterValue * 10 + keyPress < 10000)nowEnterValue = nowEnterValue * 10 + keyPress;
      delay(KEY_DELAY_TIME);
    }
  }
  else if (keyPress == KEY_RECOGNIZE)
  {
    if (CurrentSetState == SPEED)
    {
      nowEnterValue = nowEnterValue > 0 ? min(MAX_SPEED, nowEnterValue) : NOR_SPEED;
      progList[progNum].stepperSpeed = nowEnterValue;
      progList[progNum].setSpd = true;
    }
    else if (CurrentSetState == DIRECTION)
    {
      nowEnterValue = nowEnterValue == -1 ? NOR_DIR : (nowEnterValue == 0 ? BWD : FWD);
      progList[progNum].stepperDirection = nowEnterValue;
      progList[progNum].setDir = true;
    }
    else if (CurrentSetState == STEPCOUNT)
    {
      nowEnterValue = nowEnterValue == -1 ? NOR_STEPSET : (nowEnterValue > 9999 ? 9999 : nowEnterValue);
      progList[progNum].stepperSetSteps = nowEnterValue;
      progList[progNum].setSteps = true;
    }
    CurrentSetState = HOME;
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_NEXT)
  {
    if (CurrentState == SET_PROG)
    {
      progNum++;
      CurrentSetState = HOME;
      inProc = false;
    }
    delay(KEY_DELAY_TIME);
  }
  else if (keyPress == KEY_START)
  {
    for (int i = 0; i <= progNum; i++)
    {
      if (!progList[i].setSpd)progList[i].stepperSpeed = NOR_SPEED;
      if (!progList[i].setDir)progList[i].stepperDirection = NOR_DIR;
      if (!progList[i].setSteps)progList[i].stepperSetSteps = NOR_STEPSET;
    }
    if (CurrentState == SET_NORMAL && CurrentSetState == HOME)
    {
      CurrentState = NORMAL;
      inProc = false;
    }
    else if (CurrentState == SET_PROG && CurrentSetState == HOME)
    {
      nowProg = 0;
      CurrentState = PROG;
      inProc = false;
    }
    delay(KEY_DELAY_TIME);
  }
  //state event
  if (CurrentState == READY)
  {
    if (!inProc)
    {
      Stepper_motor.Stop();
      Stepper_motor.Reset();
      displayMode = 1;
      Digital_display.disContent[0] = '-';
      Digital_display.disContent[1] = '-';
      Digital_display.disContent[2] = '-';
      Digital_display.disContent[3] = '-';
      inProc = true;
      nowProg = 0;
      progNum = 0;
      progList[0].stepperSetSteps = 0;
      progList[0].stepperSpeed = NOR_SPEED;
      progList[0].stepperDirection = NOR_DIR;
    }
  }
  else if (CurrentState == TEST)
  {
    if (!inProc)
    {
      nowProg = 10;
      Stepper_motor.Stop();
      Stepper_motor.Reset();
      Stepper_motor.SetDirection(NOR_DIR);
      Stepper_motor.Start();
      inProc = true;
    }
    displayMode = 0;
    displayNumber = Stepper_motor.stepCount;
  }
  else if (CurrentState == NORMAL)
  {
    if (!inProc)
    {
      Stepper_motor.Stop();
      Stepper_motor.Reset();
      Stepper_motor.SetSpeed(progList[0].stepperSpeed);
      Stepper_motor.SetDirection(progList[0].stepperDirection);
      displayMode = 1;
      Digital_display.disContent[0] = NONE_DISPLAY;
      Digital_display.disContent[1] = NONE_DISPLAY;
      Digital_display.disContent[2] = 'P';
      Digital_display.disContent[3] = 1;
      delay(1000);
      Stepper_motor.Start();
      inProc = true;
    }
    displayMode = 0;
    displayNumber = progList[0].stepperSetSteps - Stepper_motor.stepCount;
    if (Stepper_motor.stepperOn == OFF)
    {
      Stepper_motor.Reset();
      tone(PIN_BUZZER, 1000);
      delay(TONE_WAIT_TIME);
      noTone(PIN_BUZZER);
      inProc = false;
      CurrentState = READY;
    }
  }
  else if (CurrentState == PROG)
  {
    if (!inProc)
    {
      Stepper_motor.Stop();
      Stepper_motor.Reset();
      Stepper_motor.SetSpeed(progList[nowProg].stepperSpeed);
      Stepper_motor.SetDirection(progList[nowProg].stepperDirection);
      displayMode = 1;
      Digital_display.disContent[0] = NONE_DISPLAY;
      Digital_display.disContent[1] = NONE_DISPLAY;
      Digital_display.disContent[2] = 'P';
      Digital_display.disContent[3] = nowProg + 1;
      delay(1000);
      Stepper_motor.Start();
      inProc = true;
    }
    displayMode = 0;
    displayNumber = progList[nowProg].stepperSetSteps - Stepper_motor.stepCount;
    if (Stepper_motor.stepperOn == OFF)
    {
      Stepper_motor.Reset();
      tone(PIN_BUZZER, 1000);
      delay(TONE_WAIT_TIME);
      noTone(PIN_BUZZER);
      inProc = false;
      CurrentState = PROG;
      nowProg++;
      if (nowProg > progNum)
      {
        CurrentState = READY;
        tone(PIN_BUZZER, 1000);
        delay(TONE_WAIT_TIME);
        noTone(PIN_BUZZER);
      }
    }
  }
  else if (CurrentState == SET_NORMAL)
  {
    if (!inProc)
    {
      progList[0].setSpd = false;
      progList[0].setDir = false;
      progList[0].setSteps = false;
      displayMode = 1;
      Digital_display.disContent[0] = NONE_DISPLAY;
      Digital_display.disContent[1] = NONE_DISPLAY;
      Digital_display.disContent[2] = 'P';
      Digital_display.disContent[3] = 1;
      delay(1000);
      inProc = true;
    }
    if (CurrentSetState == HOME)
    {
      displayMode = 1;
      Digital_display.disContent[0] = NONE_DISPLAY;
      Digital_display.disContent[1] = NONE_DISPLAY;
      Digital_display.disContent[2] = 'P';
      Digital_display.disContent[3] = 1;
    }
    else
    {
      if (nowEnterValue == -1)
      {
        displayMode = 1;
        Digital_display.disContent[0] = '-';
        Digital_display.disContent[1] = '-';
        Digital_display.disContent[2] = '-';
        Digital_display.disContent[3] = '-';
      }
      else
      {
        displayMode = 0;
        displayNumber = nowEnterValue;
      }
    }
  }
  else if (CurrentState == SET_PROG)
  {
    if (!inProc)
    {
      progList[progNum].setSpd = false;
      progList[progNum].setDir = false;
      progList[progNum].setSteps = false;
      displayMode = 1;
      Digital_display.disContent[0] = NONE_DISPLAY;
      Digital_display.disContent[1] = NONE_DISPLAY;
      Digital_display.disContent[2] = 'P';
      Digital_display.disContent[3] = progNum + 1;
      delay(1000);
      inProc = true;
    }
    if (CurrentSetState == HOME)
    {
      displayMode = 1;
      Digital_display.disContent[0] = NONE_DISPLAY;
      Digital_display.disContent[1] = NONE_DISPLAY;
      Digital_display.disContent[2] = 'P';
      Digital_display.disContent[3] = progNum + 1;
    }
    else
    {
      if (nowEnterValue == -1)
      {
        displayMode = 1;
        Digital_display.disContent[0] = '-';
        Digital_display.disContent[1] = '-';
        Digital_display.disContent[2] = '-';
        Digital_display.disContent[3] = '-';
      }
      else
      {
        displayMode = 0;
        displayNumber = nowEnterValue;
      }
    }
  }
}