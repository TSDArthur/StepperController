/* Stepper.c
 *
 * Created:   Sat March 17 2018
 * Processor: STC12C5A60S2
 * Compiler:  C51
 * Author:  Zhang Muhua
 *
 */
#include"stc.h"
#define MAX_SPEED 7
#define NOR_SPEED 7
#define NOR_DIR FWD
#define ALL_KEY_UP 'N'
#define KEY_WAIT_TIME 4
#define TONE_WAIT_TIME 500
#define DIGDIS_DELAY_TIME 5
#define PIN_BUZZER P3^6
#define NONE_DISPLAY 15
#define DISPLAY_REFRESH_TIME 10
#define DISPLAY_OFF -1
#define MAX_PROG_NUM 10
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
#define KEY_DELAY_TIME 100
#define pinDisplay P0
#define STEPPER_TIMER_K 50
#define STEP_ANGEL 0.5
#define u8 unsigned char
#define u32 unsigned int
enum Direction {FWD, BWD};
enum RunState {ON, OFF};
enum ControlState {READY, RUNNING, PROG, NORMAL, TEST, SET_NORMAL, SET_PROG, GOBACK};
enum SetState {HOME, SPEED, STEPCOUNT, DIRECTION};

sbit stepperPin0 = P2 ^ 5;
sbit stepperPin1 = P2 ^ 6;
sbit stepperPin2 = P2 ^ 7;
sbit stepperPin3 = P3 ^ 0;
sbit stepperPin4 = P3 ^ 1;

enum RunState stepperOn;
enum Direction stepperDirection;
u32 stepCount;
u8 stepperSpeed;
int stepBeatNum;
u32 stepperSpeedCoverted;
enum Direction lastDirection;
u8 stepDirModify;
u32 timer0Inter = 0;
u32 timer0Cnt = 0;
sbit buzzer = P3 ^ 6;

u32 historySteps = 0;
int historyAngel = 0;

u32 min(u32 a, u32 b)
{
  if (a > b)return b;
  return a;
}

void delay(u32 t)
{
  u32 i;
  u8 j;
  for (i = 0; i < t; i++)
    for (j = 0; j < 123; j++);
}

void ResetTimer0()
{
  TH0 = 0x0ff;
  TL0 = 0x9c;
}

void ResetTimer1()
{
  TH1 = 0x0d8;
  TL1 = 0x0ff;
}

u32 SpeedTable(u8 stepSpd)
{
  if (stepSpd == 1)return 29;
  else if (stepSpd == 2)return 28;
  else if (stepSpd == 3)return 27;
  else if (stepSpd == 4)return 26;
  else if (stepSpd == 5)return 25;
  else if (stepSpd == 6)return 24;
  else return 23;
}

void StepperConf()
{
  stepperDirection = NOR_DIR;
  lastDirection = NOR_DIR;
  stepperSpeed = NOR_SPEED;
  stepperSpeedCoverted = SpeedTable(stepperSpeed);
  stepCount = 0;
  stepBeatNum = stepperDirection == FWD ? 0 : 3;
  stepDirModify = 0;
  stepperOn = OFF;
}

void StepperStart()
{
  stepperOn = ON;
  timer0Inter = stepperSpeedCoverted;
  timer0Cnt = 0;
  ResetTimer0();
  TR0 = 1;
}

void StepperStop()
{
  stepperOn = OFF;
  TR0 = 0;
}

void SetSpeed(u8 stepSpd)
{
  StepperStop();
  stepperSpeed = stepSpd > 0 ? min(MAX_SPEED, stepSpd) : NOR_SPEED;
  stepperSpeedCoverted = SpeedTable(stepperSpeed);
}

void StepperSetDirection(enum Direction stepDir)
{
  StepperStop();
  lastDirection = stepperDirection;
  stepperDirection = stepDir;
  if (lastDirection != stepperDirection)stepDirModify = 1;
  else stepDirModify = 0;
}

void StepperResetBeatCounts()
{
  StepperStop();
  stepCount = 0;
}

void StepperReset()
{
  StepperStop();
  SetSpeed(NOR_SPEED);
  StepperResetBeatCounts();
}

void StepperBeats()
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
    stepDirModify = 0;
  }

  if (stepBeatNum == 0)
  {
    stepperPin0 = 1;
    stepperPin1 = 1;
    stepperPin2 = 0;
    stepperPin3 = 1;
    stepperPin4 = 0;
  }
  else if (stepBeatNum == 1)
  {
    stepperPin0 = 1;
    stepperPin1 = 0;
    stepperPin2 = 1;
    stepperPin3 = 1;
    stepperPin4 = 0;
  }
  else if (stepBeatNum == 2)
  {
    stepperPin0 = 1;
    stepperPin1 = 0;
    stepperPin2 = 1;
    stepperPin3 = 0;
    stepperPin4 = 1;
  }
  else if (stepBeatNum == 3)
  {
    stepperPin0 = 1;
    stepperPin1 = 1;
    stepperPin2 = 0;
    stepperPin3 = 0;
    stepperPin4 = 1;
  }

  stepCount++;
  if (stepperDirection == FWD)stepBeatNum++;
  else stepBeatNum--;
  stepBeatNum = stepBeatNum < 0 ? 3 : (stepBeatNum > 3 ? 0 : stepBeatNum);
  historySteps++;
  historyAngel = stepperDirection == FWD ? historyAngel + 1 : historyAngel - 1;
}

sbit colPin0 = P2 ^ 0;
sbit colPin1 = P2 ^ 1;
sbit colPin2 = P2 ^ 2;
sbit colPin3 = P2 ^ 3;
sbit colPin4 = P2 ^ 4;

sbit rowPin0 = P1 ^ 5;
sbit rowPin1 = P1 ^ 6;
sbit rowPin2 = P1 ^ 7;
sbit rowPin3 = P3 ^ 7;

const u8 keyMap[5][4] =
{
  {1, 2, 3, 'A'},
  {4, 5, 6, 'B'},
  {7, 8, 9, 'C'},
  {'D', 0, 'E', 'F'},
  {'G', 'H', 'I', 'J'}
};

void KeypadStepperReset()
{
  colPin0 = 1;
  colPin1 = 1;
  colPin2 = 1;
  colPin3 = 1;
  colPin4 = 1;

  rowPin0 = 1;
  rowPin1 = 1;
  rowPin2 = 1;
  rowPin3 = 1;
}

u8 state = 1;

u8 KeypadGetKey()
{
  u8 i;
  u8 j;
  KeypadStepperReset();
  for (i = 0; i < 5; i++)
  {
    if (i == 0)colPin0 = 0;
    else if (i == 1)colPin1 = 0;
    else if (i == 2)colPin2 = 0;
    else if (i == 3)colPin3 = 0;
    else if (i == 4)colPin4 = 0;
    state = 1;
    for (j = 0; j < 4; j++)
    {
      if (j == 0)state = rowPin0;
      else if (j == 1)state = rowPin1;
      else if (j == 2)state = rowPin2;
      else if (j == 3)state = rowPin3;
      if (state == 0)
      {
        delay(KEY_WAIT_TIME);
        if (j == 0)state = rowPin0;
        else if (j == 1)state = rowPin1;
        else if (j == 2)state = rowPin2;
        else if (j == 3)state = rowPin3;
        if (state == 0)
        {
          return keyMap[i][j];
        }
      }
    }
    if (i == 0)colPin0 = 1;
    else if (i == 1)colPin1 = 1;
    else if (i == 2)colPin2 = 1;
    else if (i == 3)colPin3 = 1;
    else if (i == 4)colPin4 = 1;
  }
  return ALL_KEY_UP;
}

u8 displayMap[16] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x92, 0x8c, 0x88, 0x86, 0xbf, 0xff};
//0-0 1-1 2-2 3-3 4-4 5-5 6-6 7-7 8-8 9-9 10-S 11-P 12-A 13-E 14--
u8 disContent[4];
u8 nowDisplayPos = 0;
sbit pinDisplay0 = P1 ^ 1;
sbit pinDisplay1 = P1 ^ 2;
sbit pinDisplay2 = P1 ^ 3;
sbit pinDisplay3 = P1 ^ 4;

void DigClear()
{
  pinDisplay = displayMap[15];
}

u8 DigContentCovert(u8 disContent)
{
  if (disContent == 'S')return 10;
  else if (disContent == 'P')return 11;
  else if (disContent == 'A')return 12;
  else if (disContent == 'E')return 13;
  else if (disContent == '-')return 14;
  else return disContent;
}

void DigDisplay()
{
  u8 i;
  nowDisplayPos = nowDisplayPos == 4 ? 0 : nowDisplayPos;
  DigClear();
  for (i = 0; i < 4; i++)
  {
    if (i != nowDisplayPos)
    {
      if (i == 0)pinDisplay0 = 0;
      else if (i == 1)pinDisplay1 = 0;
      else if (i == 2)pinDisplay2 = 0;
      else if (i == 3)pinDisplay3 = 0;
    }
    else
    {
      if (i == 0)pinDisplay0 = 1;
      else if (i == 1)pinDisplay1 = 1;
      else if (i == 2)pinDisplay2 = 1;
      else if (i == 3)pinDisplay3 = 1;
    }
  }
  pinDisplay = displayMap[DigContentCovert(disContent[nowDisplayPos])];
  nowDisplayPos++;
}

struct Control
{
  enum Direction stepperDirection;
  u8 stepperSpeed;
  u32 stepperSetSteps;
  u8 setDir;
  u8 setSpd;
  u8 setSteps;
} progList[MAX_PROG_NUM];

int lastShowNumber = -1;
u8 displayMode = 2;
u32 displayNumber = 0;
enum ControlState CurrentState;
u8 progNum = 0;
u8 inProc = 0;
u8 nowProg = 0;
enum SetState CurrentSetState;
int nowEnterValue = -1;
sbit buzeer = P3 ^ 6;

void Display_Number(int numIn)
{
  if (numIn == lastShowNumber)return;
  if (numIn < 0)
  {
    if (-1 * numIn < 10)
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = NONE_DISPLAY;
      disContent[2] = '-';
      disContent[3] = -1 * numIn;
    }
    else if (-1 * numIn < 100)
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = '-';
      disContent[2] = (-1 * numIn) / 10;
      disContent[3] = (-1 * numIn) % 10;
    }
    else if (-1 * numIn < 1000)
    {
      disContent[0] = '-';
      disContent[1] = (-1 * numIn) / 100;
      disContent[2] = (-1 * numIn / 10) % 10;
      disContent[3] = (-1 * numIn) % 10;
    }
    else
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = NONE_DISPLAY;
      disContent[2] = 'E';
      disContent[3] = 0;
    }
  }
  else
  {
    if (numIn < 10)
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = NONE_DISPLAY;
      disContent[2] = NONE_DISPLAY;
      disContent[3] = numIn;
    }
    else if (numIn < 100)
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = NONE_DISPLAY;
      disContent[2] = numIn / 10;
      disContent[3] = numIn % 10;
    }
    else if (numIn < 1000)
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = numIn / 100;
      disContent[2] = (numIn / 10) % 10;
      disContent[3] = numIn % 10;
    }
    else if (numIn < 10000)
    {
      disContent[0] = numIn / 1000;
      disContent[1] = (numIn / 100) % 10;
      disContent[2] = (numIn / 10) % 10;
      disContent[3] = numIn % 10;
    }
    else
    {
      disContent[0] = NONE_DISPLAY;
      disContent[1] = NONE_DISPLAY;
      disContent[2] = 'E';
      disContent[3] = 0;
    }
  }
  lastShowNumber = numIn;
}

void StepperBeating() interrupt 1
{
  ResetTimer0();
  timer0Cnt++;
  if (timer0Cnt == timer0Inter)
  {
    StepperBeats();
    if (nowProg != 10 && stepCount == progList[nowProg].stepperSetSteps)
      StepperStop();
    timer0Cnt = 0;
  }
}

void DisplayShowing() interrupt 3
{
  ResetTimer1();
  if (displayMode == 0)Display_Number(displayNumber);
  else if (displayMode == 1)lastShowNumber = DISPLAY_OFF;
  if (displayMode != 2)DigDisplay();
  else DigClear();
}

void BuzzerTone()
{
  buzzer = 1;
  delay(KEY_DELAY_TIME * 2);
  buzzer = 0;
}

u8 keyPress;
enum Direction testDir = FWD;
enum ControlState lastState = READY;
u8 lastKey = ALL_KEY_UP;

void main()
{
  u8 i;
  buzeer = 0;
  CurrentState = READY;
  StepperConf();
  TMOD = 0x11;
  ResetTimer0();
  ResetTimer1();
  EA = 1;
  ET0 = 1;
  ET1 = 1;
  TR1 = 1;
  TR0 = 0;
  P3M1 = 0;
  P3M0 = 0xff;
  P2M1 = 0;
  P2M0 = 0xff;
  while (1)
  {
    keyPress = KeypadGetKey();
    if (lastKey == keyPress && keyPress != ALL_KEY_UP && keyPress != KEY_TEST)continue;
    if (CurrentState == TEST && keyPress != KEY_TEST)
    {
      CurrentState = READY;
      inProc = 0;
      BuzzerTone();
    }
    if (keyPress == KEY_STOP && CurrentState != GOBACK)
    {
      if (CurrentState == READY)
      {
        StepperSetDirection(testDir);
        StepperBeats();
        delay(KEY_DELAY_TIME);
      }
      else
      {
        CurrentState = READY;
        inProc = 0;
      }
      BuzzerTone();
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_TEST && CurrentState != GOBACK)
    {
      if (CurrentState == READY)
      {
        CurrentState = TEST;
        inProc = 0;
        BuzzerTone();
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_MODENORMAL && CurrentState != GOBACK)
    {
      BuzzerTone();
      if (CurrentState == READY)
      {
        CurrentState = SET_NORMAL;
        CurrentSetState = HOME;
        nowEnterValue = -1;
        inProc = 0;
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_MODEPROG && CurrentState != GOBACK)
    {
      BuzzerTone();
      if (CurrentState == READY)
      {
        CurrentState = SET_PROG;
        CurrentSetState = HOME;
        nowEnterValue = -1;
        inProc = 0;
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_SPEED && CurrentState != GOBACK)
    {
      BuzzerTone();
      if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && CurrentSetState != SPEED)
      {
        nowEnterValue = -1;
        CurrentSetState = SPEED;
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_STEPCOUNT && CurrentState != GOBACK)
    {
      BuzzerTone();
      if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && CurrentSetState != STEPCOUNT)
      {
        nowEnterValue = -1;
        CurrentSetState = STEPCOUNT;
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_DIRECTION && CurrentState != GOBACK)
    {
      BuzzerTone();
      if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && CurrentSetState != DIRECTION)
      {
        nowEnterValue = -1;
        CurrentSetState = DIRECTION;
      }
      else if (CurrentState == READY)
      {
        testDir = testDir == FWD ? BWD : FWD;
        displayMode = 0;
        displayNumber = testDir == FWD ? 1 : 0;
        delay(KEY_DELAY_TIME * 20);
        displayMode = 1;
        disContent[0] = '-';
        disContent[1] = '-';
        disContent[2] = '-';
        disContent[3] = '-';
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress >= 0 && keyPress <= 9 && CurrentState != GOBACK)
    {
      BuzzerTone();
      if ((CurrentState == SET_NORMAL || CurrentState == SET_PROG) && (CurrentSetState != HOME))
      {
        if (nowEnterValue == -1)nowEnterValue = keyPress;
        else if (nowEnterValue * 10 + keyPress < 10000)nowEnterValue = nowEnterValue * 10 + keyPress;
      }
    }
    else if (keyPress == KEY_RECOGNIZE && CurrentState != GOBACK)
    {
      BuzzerTone();
      if (CurrentSetState == SPEED)
      {
        nowEnterValue = nowEnterValue > 0 ? min(MAX_SPEED, nowEnterValue) : NOR_SPEED;
        progList[progNum].stepperSpeed = nowEnterValue;
        progList[progNum].setSpd = 1;
      }
      else if (CurrentSetState == DIRECTION)
      {
        nowEnterValue = nowEnterValue == -1 ? NOR_DIR : (nowEnterValue == 0 ? BWD : FWD);
        progList[progNum].stepperDirection = nowEnterValue;
        progList[progNum].setDir = 1;
      }
      else if (CurrentSetState == STEPCOUNT)
      {
        nowEnterValue = nowEnterValue == -1 ? NOR_STEPSET : (nowEnterValue > 9999 ? 9999 : nowEnterValue);
        progList[progNum].stepperSetSteps = nowEnterValue;
        progList[progNum].setSteps = 1;
      }
      else if (CurrentState == READY)
      {
        if (historyAngel)
        {
          CurrentState = GOBACK;
          inProc = 0;
        }
      }
      CurrentSetState = HOME;
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_NEXT && CurrentState != GOBACK)
    {
      BuzzerTone();
      if (CurrentState == SET_PROG && progNum < MAX_PROG_NUM - 2)
      {
        progNum++;
        CurrentSetState = HOME;
        inProc = 0;
      }
      else if (CurrentState == READY)
      {
        displayMode = 0;
        displayNumber = historySteps;
        delay(KEY_DELAY_TIME * 20);
        displayMode = 1;
        disContent[0] = '-';
        disContent[1] = '-';
        disContent[2] = '-';
        disContent[3] = '-';
      }
      delay(KEY_DELAY_TIME);
    }
    else if (keyPress == KEY_START && CurrentState != GOBACK)
    {
      BuzzerTone();
      if (CurrentState == SET_NORMAL || CurrentState == SET_PROG)
      {
        for (i = 0; i <= progNum; i++)
        {
          if (!progList[i].setSpd)progList[i].stepperSpeed = NOR_SPEED;
          if (!progList[i].setDir)progList[i].stepperDirection = NOR_DIR;
          if (!progList[i].setSteps)progList[i].stepperSetSteps = NOR_STEPSET;
        }
        if (CurrentState == SET_NORMAL && CurrentSetState == HOME)
        {
          CurrentState = NORMAL;
          inProc = 0;
        }
        else if (CurrentState == SET_PROG && CurrentSetState == HOME)
        {
          nowProg = 0;
          CurrentState = PROG;
          inProc = 0;
        }
      }
      else if (CurrentState == READY)
      {
        displayMode = 0;
        displayNumber = historyAngel;
        delay(KEY_DELAY_TIME * 20);
        displayMode = 1;
        disContent[0] = '-';
        disContent[1] = '-';
        disContent[2] = '-';
        disContent[3] = '-';
      }
      delay(KEY_DELAY_TIME);
    }

    //record pre data
    lastKey = keyPress;
    lastState = CurrentState;

    //state event
    if (CurrentState == READY)
    {
      if (!inProc)
      {
        StepperStop();
        StepperReset();
        displayMode = 1;
        disContent[0] = '-';
        disContent[1] = '-';
        disContent[2] = '-';
        disContent[3] = '-';
        inProc = 1;
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
        StepperStop();
        StepperReset();
        StepperSetDirection(testDir);
        StepperStart();
        inProc = 1;
      }
      displayMode = 0;
      displayNumber = stepCount;
    }
    else if (CurrentState == GOBACK)
    {
      if (!inProc)
      {
        nowProg = 9;
        StepperStop();
        StepperReset();
        progList[nowProg].stepperSetSteps = historyAngel < 0 ? -1 * historyAngel : historyAngel;
        SetSpeed(NOR_SPEED);
        StepperSetDirection(historyAngel > 0 ? BWD : FWD);
        StepperStart();
        inProc = 1;
      }
      displayMode = 0;
      displayNumber = stepCount;
      if (stepperOn == OFF)
      {
        StepperReset();
        inProc = 0;
        CurrentState = READY;
        BuzzerTone();
      }
    }
    else if (CurrentState == NORMAL)
    {
      if (!inProc)
      {
        StepperStop();
        StepperReset();
        SetSpeed(progList[0].stepperSpeed);
        StepperSetDirection(progList[0].stepperDirection);
        displayMode = 1;
        disContent[0] = NONE_DISPLAY;
        disContent[1] = NONE_DISPLAY;
        disContent[2] = 'P';
        disContent[3] = 1;
        delay(1000);
        StepperStart();
        inProc = 1;
      }
      displayMode = 0;
      displayNumber = progList[0].stepperSetSteps - stepCount;
      if (stepperOn == OFF)
      {
        StepperReset();
        inProc = 0;
        CurrentState = READY;
        BuzzerTone();
      }
    }
    else if (CurrentState == PROG)
    {
      if (!inProc)
      {
        StepperStop();
        StepperReset();
        SetSpeed(progList[nowProg].stepperSpeed);
        StepperSetDirection(progList[nowProg].stepperDirection);
        displayMode = 1;
        disContent[0] = NONE_DISPLAY;
        disContent[1] = NONE_DISPLAY;
        disContent[2] = 'P';
        disContent[3] = nowProg + 1;
        delay(1000);
        StepperStart();
        inProc = 1;
      }
      displayMode = 0;
      displayNumber = progList[nowProg].stepperSetSteps - stepCount;
      if (stepperOn == OFF)
      {
        StepperReset();
        inProc = 0;
        CurrentState = PROG;
        nowProg++;
        if (nowProg > progNum)
        {
          CurrentState = READY;
          BuzzerTone();
        }
      }
    }
    else if (CurrentState == SET_NORMAL)
    {
      if (!inProc)
      {
        progList[0].setSpd = 0;
        progList[0].setDir = 0;
        progList[0].setSteps = 0;
        displayMode = 1;
        disContent[0] = NONE_DISPLAY;
        disContent[1] = NONE_DISPLAY;
        disContent[2] = 'P';
        disContent[3] = 1;
        delay(1000);
        inProc = 1;
      }
      if (CurrentSetState == HOME)
      {
        displayMode = 1;
        disContent[0] = NONE_DISPLAY;
        disContent[1] = NONE_DISPLAY;
        disContent[2] = 'P';
        disContent[3] = 1;
      }
      else
      {
        if (nowEnterValue == -1)
        {
          displayMode = 1;
          disContent[0] = '-';
          disContent[1] = '-';
          disContent[2] = '-';
          disContent[3] = '-';
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
        progList[progNum].setSpd = 0;
        progList[progNum].setDir = 0;
        progList[progNum].setSteps = 0;
        displayMode = 1;
        disContent[0] = NONE_DISPLAY;
        disContent[1] = NONE_DISPLAY;
        disContent[2] = 'P';
        disContent[3] = progNum + 1;
        delay(1000);
        inProc = 1;
      }
      if (CurrentSetState == HOME)
      {
        displayMode = 1;
        disContent[0] = NONE_DISPLAY;
        disContent[1] = NONE_DISPLAY;
        disContent[2] = 'P';
        disContent[3] = progNum + 1;
      }
      else
      {
        if (nowEnterValue == -1)
        {
          displayMode = 1;
          disContent[0] = '-';
          disContent[1] = '-';
          disContent[2] = '-';
          disContent[3] = '-';
        }
        else
        {
          displayMode = 0;
          displayNumber = nowEnterValue;
        }
      }
    }
  }
}