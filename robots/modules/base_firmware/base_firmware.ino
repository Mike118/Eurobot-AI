//#include "BrushlessMotor.h" 
#include "SerialMotor.h" 
#include "I2CMotor.h" 
#include <Wire.h>
//#define MODE_I2C
#ifndef MODE_I2C
  #define MODE_SERIAL
  //#define SERIAL_DEBUG
#endif
#include "comunication.h"

#define LED_PIN 13
bool ledValue = true;

const double wheelPerimeter = 186.5d;//188.495559215;//mm = pi*d = pi*60
const double reductionFactor = 3.75d;//3.75d
#ifdef TEENSYDUINO
const double wheelDistanceA = 125;//mm
const double wheelDistanceB = 125;//mm
const double wheelDistanceC = 125;//mm
#else
const double wheelDistanceA = 120;//mm
const double wheelDistanceB = 125.4;//mm
const double wheelDistanceC = 123;//mm
#endif
//BrushlessMotor motorA(4,2,3, 39, A22, A21, wheelPerimeter, false);
//BrushlessMotor motorB(7,5,6, 36, A19, A18, wheelPerimeter, false);
//BrushlessMotor motorC(10,8,9,33, A16, A15, wheelPerimeter, false);

//SerialMotor motorA(&Serial3, wheelPerimeter/reductionFactor, true, true);
//SerialMotor motorB(&Serial4, wheelPerimeter/reductionFactor, true);
//SerialMotor motorC(&Serial2, wheelPerimeter/reductionFactor, true);

I2CMotor motorA(0x10, wheelPerimeter/reductionFactor, true);
I2CMotor motorB(0x11, wheelPerimeter/reductionFactor, true);
I2CMotor motorC(0x12, wheelPerimeter/reductionFactor, true);


const double motorA_angle = -60;//°
const double motorB_angle = 180;//°
const double motorC_angle = 60;//°

//--FRONT-
//-A-----C
//---\-/--
//----|---
//----B---
//--BACK--

// Servos
#include <Servo.h>
#include <Ramp.h>  
const int MIN_PULSE = 500;
const int MAX_PULSE = 2500;

const int SERVO_AC_A = 29;//29
const int SERVO_AC_C = 2;//
const int SERVO_AB_A = 6;//
const int SERVO_AB_B = 5;//
const int SERVO_BC_B = 4;//
const int SERVO_BC_C = 3;//
const int SERVO_FLAG = 20;
Servo servoAC_A, servoAC_C;
Servo servoAB_A, servoAB_B;
Servo servoBC_B, servoBC_C;
Servo servoFlag;
ramp servoRampAC_A, servoRampAC_C;
ramp servoRampAB_A, servoRampAB_B;
ramp servoRampBC_B, servoRampBC_C;
ramp servoRampFlag;

const int NB_SERVO_ARM_M = 5;
const int pinNumberServo_M[NB_SERVO_ARM_M] = {30,23,22,35,21};
Servo servos_M[NB_SERVO_ARM_M];
ramp servosRamp_M[NB_SERVO_ARM_M];
const int pinNumberPump_L = 37;
const int pinNumberPump_R = 36;
int positionServo_M_DefaultOut[NB_SERVO_ARM_M] = {90,90,90,90,90};//Z 40 90 90 90 90 140 0
int positionServo_M_Default[NB_SERVO_ARM_M] = {50,90,140,50,90};//Z 40 10 150 60 90 140 0

typedef struct {
  float x=0;
  float y=0;
  float angle=0;
  PathSegment(float _x, float _y, float _angle){
    this->x = _x;
    this->y = _y;
    this->angle = _angle;
  }
} PathSegment;

PathSegment targetPath[50];
int targetPathIndex = 0;
int targetPathSize = 0;
bool runTargetPath = false;

void setArmPose(int* pose, int duration)
{
  for(int i=0;i<NB_SERVO_ARM_M;i++){
    servosRamp_M[i].go(pose[i], duration);
  }
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 1);
  delay(2500);
  servoAC_A.attach(SERVO_AC_A, MIN_PULSE, MAX_PULSE);//SERVO_AC_A
  servoAC_C.attach(SERVO_AC_C, MIN_PULSE, MAX_PULSE);
  servoAB_A.attach(SERVO_AB_A, MIN_PULSE, MAX_PULSE);
  servoAB_B.attach(SERVO_AB_B, MIN_PULSE, MAX_PULSE);
  servoBC_B.attach(SERVO_BC_B, MIN_PULSE, MAX_PULSE);
  servoBC_C.attach(SERVO_BC_C, MIN_PULSE, MAX_PULSE);
  servoFlag.attach(SERVO_FLAG, MIN_PULSE, MAX_PULSE);
  servoRampAC_A.go(70,0);
  servoRampAC_C.go(110,0);
  servoRampAB_A.go(110,0);
  servoRampAB_B.go(70,0);
  servoRampBC_B.go(110,0);
  servoRampBC_C.go(70,0);
  servoRampFlag.go(27,0);
  
  // Setup Arm M
  for(int i=0; i<NB_SERVO_ARM_M; i++){
    servos_M[i].attach(pinNumberServo_M[i], MIN_PULSE, MAX_PULSE);
    setArmPose(positionServo_M_DefaultOut, 0);
  }
  updateServos();

  //Setup pumps
  pinMode(pinNumberPump_L, OUTPUT);
  pinMode(pinNumberPump_R, OUTPUT);
  digitalWrite(pinNumberPump_L, LOW);
  digitalWrite(pinNumberPump_R, LOW);
  
  //Init motor
  Wire.begin();
  motorA.begin();  
  motorB.begin();
  motorC.begin();

  //Init communication
  comunication_begin(7);//I2C address 7

  /*motorA.setSpeed(1);
  motorB.setSpeed(1);
  motorC.setSpeed(1);*/
}

//In meters, degrees, m/s and °/s
double xStart = 0, yStart = 0, angleStart = 0;
double xPos = 0, yPos = 0, anglePos = 0;
double xTarget = 0, yTarget = 0, angleTarget = 0, speedTarget = 3.5, angleSpeedTarget = 50;

void updatePosition(){
  double distA = motorA.getAndResetDistanceDone();
  double distB = motorB.getAndResetDistanceDone();
  double distC = motorC.getAndResetDistanceDone();
  
  double xA = sin((motorA_angle+anglePos+90.0d)*DEG_TO_RAD)*distA;
  double xB = sin((motorB_angle+anglePos+90.0d)*DEG_TO_RAD)*distB;
  double xC = sin((motorC_angle+anglePos+90.0d)*DEG_TO_RAD)*distC;

  double yA = cos((motorA_angle+anglePos+90.0d)*DEG_TO_RAD)*distA; //motorA_angle+90 is the wheel angle
  double yB = cos((motorB_angle+anglePos+90.0d)*DEG_TO_RAD)*distB;
  double yC = cos((motorC_angle+anglePos+90.0d)*DEG_TO_RAD)*distC;

  xPos += (xA+xB+xC)*0.635;
  yPos += (yA+yB+yC)*0.635;

  double angleDoneA = (distA*360)/(2.0d*PI*(wheelDistanceA/1000.0d));
  double angleDoneB = (distB*360)/(2.0d*PI*(wheelDistanceB/1000.0d));
  double angleDoneC = (distC*360)/(2.0d*PI*(wheelDistanceC/1000.0d));

  anglePos += (angleDoneA+angleDoneB+angleDoneC)/3.0d;
  if(anglePos>180.0d) anglePos += -360.0d;
  if(anglePos<-180.0d) anglePos += 360.0d;
}


double custom_mod(double a, double n){
  return a - floor(a/n) * n;
}

double angleDiff(double a, double b){
  return custom_mod((a-b)+180, 360)-180;
}

double targetMovmentAngle = 0;
double targetSpeed_mps = 0.0;// m/s
double targetAngleSpeed_dps = 0;// °/s

double targetAngleError = 1.0; //° 1.0
double targetPosError = 0.005; //meters
bool targetReached = true;
int targetReachedCountTarget = 10;
int targetReachedCount = 0;

double applySpeedRamp(double distFromStart, double distFromEnd, double rampDist, double speedTarget, double minSpeed){
  double speed = speedTarget;
  double errorSign = distFromEnd>0?1:-1;
  if(abs(distFromStart)<rampDist){ //speed up ramp
    float factor = 1.0/rampDist*abs(distFromStart);
    speed = speedTarget*factor;
  }
  else if(abs(distFromEnd)<rampDist){ //speed down ramp
    float factor = 1.0/rampDist*abs(distFromEnd);
    speed = speedTarget*factor;
  }
  else { //cruise speed
    speed = speedTarget;
  }
  //Bound speed
  if(abs(speed)>speedTarget) speed = speedTarget;
  if(abs(speed)<minSpeed) speed = minSpeed;
  speed *= errorSign;
  return speed;
}

//Y is forward, X is right side (motor C side)
void updateAsserv(){
  //Translation
  double xDiff = xTarget - xPos;
  double yDiff = yTarget - yPos;
  double translationError = sqrt(pow(xPos - xTarget,2) + pow(yPos - yTarget,2)); // meters
  double translationAngle = 0;
  if(xDiff!=0.f)
    translationAngle = atan2(xDiff, yDiff)*RAD_TO_DEG;
  else if(yDiff<0.f)
    translationAngle = -180;

  targetMovmentAngle = angleDiff(translationAngle, anglePos);

  //Translation Speed
  double minSpeed = 0.05;
  if(runTargetPath && targetPathIndex>0 && targetPathIndex<targetPathSize-1)
    minSpeed = 0.1;
  double slowDownDistance = 0.20;//m
  double distFromStart = sqrt(pow(xPos - xStart,2) + pow(yPos - yStart,2)); // meters
  double distFromEnd = translationError;
  targetSpeed_mps = applySpeedRamp(distFromStart, distFromEnd, slowDownDistance, speedTarget, minSpeed);
  

  //Rotation
  double angleMinSpeed = 10;//deg/s
  double slowDownAngle = 25;//deg
  double rotationError = angleDiff(angleTarget,anglePos);
  double rotationFromStart = angleDiff(angleStart,anglePos);
  
  targetAngleSpeed_dps = applySpeedRamp(rotationFromStart, rotationError, slowDownAngle, angleSpeedTarget, angleMinSpeed);
  
  if(translationError>targetPosError || fabs(rotationError)>targetAngleError){
    targetReached = false;
    targetReachedCount=0;
  }
  else{
    if(targetReachedCount >= targetReachedCountTarget)
      targetReached = true;  
    targetReachedCount++;
    targetAngleSpeed_dps=0;
    targetSpeed_mps=0;
  }
}

void printCharts(){
  //Position
  Serial.print(xPos*1000);Serial.print(" ");
  Serial.print(yPos*1000);Serial.print(" ");
  Serial.print(xTarget*1000);Serial.print(" ");
  Serial.print(yTarget*1000);Serial.print(" ");

  //Angle
  Serial.print(anglePos);Serial.print(" ");
  Serial.print(angleTarget);Serial.print(" ");
  Serial.print(targetMovmentAngle);Serial.print(" ");

  //Angle Speed
  Serial.print(targetAngleSpeed_dps);Serial.print(" ");
  Serial.print(angleSpeedTarget);Serial.print(" ");

  //Position Speed
  Serial.print(targetSpeed_mps*100.0);Serial.print(" ");
  Serial.print(speedTarget*100.0);Serial.print(" ");

  //Motor Speed
  Serial.print(motorA.getSpeed());Serial.print(" ");
  Serial.print(motorB.getSpeed());Serial.print(" ");
  //Serial.print(motorB.getSpeed()*1000);Serial.print(" ");
  //Serial.print(motorC.getSpeed()*1000);Serial.print(" ");
  Serial.print("\r\n");
}

float nextPathTranslationError = 0.03;//meters
float nextPathRotationError = 5;//degrees
void updatePath(){
  if(!runTargetPath || targetReached) return;
  //Set current target
  xTarget = targetPath[targetPathIndex].x;
  yTarget = targetPath[targetPathIndex].y;
  angleTarget = targetPath[targetPathIndex].angle;

  //Compute error to switch to next target
  double translationError = sqrt(pow(xPos - xTarget,2) + pow(yPos - yTarget,2)); // meters
  double rotationError = angleDiff(angleTarget,anglePos);
  if(targetPathIndex<targetPathSize-1
    && translationError < nextPathTranslationError
    && rotationError < nextPathRotationError){
      targetPathIndex++;
      //Reset startPos for accel ramp
      xStart = xPos;
      yStart = yPos;
      angleStart = anglePos;
  } 
  
  //Set new target
  xTarget = targetPath[targetPathIndex].x;
  yTarget = targetPath[targetPathIndex].y;
  angleTarget = targetPath[targetPathIndex].angle;
}

bool movementEnabled = false;
bool emergencyStop = false;
bool manualMode = false;
void control(){
  //Update position
  //updatePosition();
  //Serial.print("Position\t");Serial.print(xPos);Serial.print("\t");Serial.print(yPos);Serial.print("\t");Serial.print(anglePos);Serial.print("\n");

  //double daTargetDistance = sqrt(pow(xpos - xTarget,2) + pow(ypos - yTarget,2)); // meters
  //double daTargetAngle = angleDiff(angleTarget,angle);

  //Update target from path
  if(runTargetPath) updatePath();
  
  /* Compute speeds with asserv:
   *  targetMovmentAngle
   *  targetSpeed_mps
   *  targetAngleSpeed_dps
   */
  if(!manualMode) updateAsserv();
  #ifdef SERIAL_DEBUG
    printCharts();
  #endif
  
  
  //Compute translation
  double speedA = targetSpeed_mps * sin((targetMovmentAngle-motorA_angle)*DEG_TO_RAD);
  double speedB = targetSpeed_mps * sin((targetMovmentAngle-motorB_angle)*DEG_TO_RAD);
  double speedC = targetSpeed_mps * sin((targetMovmentAngle-motorC_angle)*DEG_TO_RAD);

  //Compute Rotation
  double arcLengthA = 2.0d*PI*(wheelDistanceA/1000.d)*(targetAngleSpeed_dps/360.0d); // arcLength in meters.
  double arcLengthB = 2.0d*PI*(wheelDistanceB/1000.d)*(targetAngleSpeed_dps/360.0d); // arcLength in meters.
  double arcLengthC = 2.0d*PI*(wheelDistanceC/1000.d)*(targetAngleSpeed_dps/360.0d); // arcLength in meters.
  double speedAngleA = arcLengthA; 
  double speedAngleB = arcLengthB;
  double speedAngleC = arcLengthC;
  
  //Serial.print(speedA*1000.0d);Serial.print("\t");Serial.print(arcLength*1000.0d);Serial.print("\n");
  
  speedA += speedAngleA*1.25;
  speedB += speedAngleB*1.25;
  speedC += speedAngleC*1.25;


  //Serial.print(speedA*1000000.f);Serial.print("\t");Serial.print(speedB*1000000.f);Serial.print("\t");Serial.print(speedC*1000000.f);Serial.print("\n");
  //Serial.print(motorA.m_currSleep);Serial.print("\t");Serial.print(motorB.m_currSleep==0);Serial.print("\t");Serial.print(motorC.m_currSleep);Serial.print("\n");

  //Compute Sync Factor -> so motors slows their Acceleration based on the max-accelerating one
  double syncFactorA=1, syncFactorB=1, syncFactorC=1;
  double speedDiffA = abs(speedA - motorA.getSpeed());
  double speedDiffB = abs(speedB - motorB.getSpeed());
  double speedDiffC = abs(speedC - motorC.getSpeed());
  double speedDiffMax = max(speedDiffA,max(speedDiffB,speedDiffC));
  syncFactorA = 1;//speedDiffA/speedDiffMax;
  syncFactorB = 1;//speedDiffB/speedDiffMax;
  syncFactorC = 1;//speedDiffC/speedDiffMax;
  
  //Drive motors
  if(!emergencyStop && movementEnabled){
    motorA.setSpeed(speedA, syncFactorA);
    motorB.setSpeed(speedB, syncFactorB);
    motorC.setSpeed(speedC, syncFactorC);
  }
  else{
    motorA.setSpeed(0);
    motorB.setSpeed(0);
    motorC.setSpeed(0);
  }
}

int positionFrequency = 100; //Hz
int controlFrequency = 100; //Hz

void executeOrder(){
  comunication_read();
  if(comunication_msgAvailable()){
    if(comunication_InBuffer[0] == '#' && comunication_InBuffer[1] != '\0'){
      //ignore
    }
    else if(strstr(comunication_InBuffer, "id")){
      sprintf(comunication_OutBuffer, "MovingBaseAlexandreV4");//max 29 Bytes
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "move enable")){
      movementEnabled = true;
      emergencyStop = false;
      sprintf(comunication_OutBuffer, "move OK");//max 29 Bytes
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "move disable")){
      movementEnabled = false;
      sprintf(comunication_OutBuffer, "move OK");//max 29 Bytes
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "pos set ")){
      sprintf(comunication_OutBuffer, "pos OK");//max 29 Bytes
      comunication_write();//async
      int x_pos=0, y_pos=0, angle_pos=0;
      int res = sscanf(comunication_InBuffer, "pos set %i %i %i", &y_pos, &x_pos, &angle_pos);
      xPos = (float)(x_pos)/1000.0f;
      yPos = (float)(y_pos)/1000.0f;
      anglePos = angle_pos;
      xTarget = xPos;
      yTarget = yPos;
      angleTarget = anglePos;
      updateAsserv();
    }
    else if(strstr(comunication_InBuffer, "pos getXY")){
      sprintf(comunication_OutBuffer,"pos %i %i %i %i",(int)(yPos*1000.0f), (int)(xPos*1000.0f), (int)(anglePos), (int)(targetSpeed_mps*10));
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "pos getDA")){
      sprintf(comunication_OutBuffer, "ERROR");//max 29 Bytes
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "move XY ")){
      sprintf(comunication_OutBuffer,"move OK");
      comunication_write();//async
      int i_x_pos=0, i_y_pos=0, i_angle=0, i_speed_pos=1;
      sscanf(comunication_InBuffer, "move XY %i %i %i %i", &i_y_pos, &i_x_pos, &i_angle, &i_speed_pos);
      xStart = xPos;
      yStart = yPos;
      angleStart = anglePos;
      xTarget = (float)(i_x_pos)/1000.0f;
      yTarget = (float)(i_y_pos)/1000.0f;
      angleTarget = (float)(i_angle);
      speedTarget = (float)(i_speed_pos)/10.0f;
      angleSpeedTarget = speedTarget * 180.f;
      targetReached = false;
      emergencyStop = false;
      targetReached = false;
      runTargetPath = false;
      movementEnabled = true;
      updateAsserv();
    }
    else if(strstr(comunication_InBuffer, "path set ")){
      sprintf(comunication_OutBuffer,"path OK");
      comunication_write();//async
      int action=-1, i_x_pos=0, i_y_pos=0, i_angle=0, i_speed_pos=1;
      sscanf(comunication_InBuffer, "path set %i %i %i %i %i", &action, &i_y_pos, &i_x_pos, &i_angle, &i_speed_pos);
      //0 reset and add 1 point
      //1 add 1 point
      //2 add 1 point and run
      //3 reset, add 1 point and run
      if(action==0 || action==3){//reset
        targetReached = false;
        targetPathIndex = 0;
        targetPathSize = 0;
        runTargetPath = false;
      }
      if(action==0 || action==1 || action==2 || action==3){//add
        int idx = targetPathSize;
        targetPath[idx].x = (float)(i_x_pos)/1000.0f;
        targetPath[idx].y = (float)(i_y_pos)/1000.0f;
        targetPath[idx].angle = (float)(i_angle);
        speedTarget = (float)(i_speed_pos)/10.0f;
        angleSpeedTarget = speedTarget * 180.f;
        targetPathSize++;
      }
      if((action==2 || action==3) && targetPathSize){//run
        xStart = xPos;
        yStart = yPos;
        angleStart = anglePos;
        targetReached = false;
        runTargetPath = true;
        emergencyStop = false;
        movementEnabled = true;
        updatePath();
        updateAsserv();
      }
    }
    else if(strstr(comunication_InBuffer, "move DA")){
      sprintf(comunication_OutBuffer, "ERROR");//max 29 Bytes
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "status get")){
      sprintf(comunication_OutBuffer,"%s %i %i %i %i %i", (targetReached?"end":"run"), (int)(yPos*1000.0f), (int)(xPos*1000.0f), (int)(anglePos), (int)(targetSpeed_mps*10), targetPathIndex);
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "speed get")){
      sprintf(comunication_OutBuffer,"speed %.1f",targetSpeed_mps);
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "move break")){
      sprintf(comunication_OutBuffer,"move OK");
      comunication_write();//async
      emergencyStop = true;
      targetSpeed_mps = 0;
      targetAngleSpeed_dps = 0;
    }
    else if(strstr(comunication_InBuffer, "move RM")){
      int i_distance=0, i_vitesse=4;
      sscanf(comunication_InBuffer, "move RM %i %i", &i_distance, &i_vitesse);
      sprintf(comunication_OutBuffer,"ERROR");
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "support XY")){
      sprintf(comunication_OutBuffer,"support 1");
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "support Path")){
      sprintf(comunication_OutBuffer,"support 1");
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "manual enable")){
      manualMode=true;
      sprintf(comunication_OutBuffer,"manual OK");
      comunication_write();//async
    }
    else if(strstr(comunication_InBuffer, "manual disable")){
      sprintf(comunication_OutBuffer,"manual OK");
      comunication_write();//async
      manualMode=false;
      targetMovmentAngle = 0;
      targetSpeed_mps = 0;
      targetAngleSpeed_dps = 0;
      xTarget = xPos;
      yTarget = yPos;
      angleTarget = anglePos;
    }
    else if(strstr(comunication_InBuffer, "manual set ")){
      sprintf(comunication_OutBuffer,"manual OK");
      comunication_write();//async
      int i_move_angle=0, i_move_speed=0, i_angle_speed=0;
      sscanf(comunication_InBuffer, "manual set %i %i %i", &i_move_angle, &i_move_speed, &i_angle_speed);
      targetMovmentAngle = i_move_angle;
      targetSpeed_mps = (float)(i_move_speed)/10.0f;
      targetAngleSpeed_dps = i_angle_speed;
      emergencyStop = false;
      runTargetPath = false;
    }
    else if(strstr(comunication_InBuffer, "servo set ")){
      sprintf(comunication_OutBuffer, "OK");//max 29 Bytes
      comunication_write();//async
      char c1, c2, c3;
      int angle=90, duration=0;
      sscanf(comunication_InBuffer, "servo set %c%c%c %i %i", &c1, &c2, &c3, &angle, &duration);
      if(c1=='A' && c2=='C' && c3=='A') servoRampAC_A.go(angle, duration);
      if(c1=='A' && c2=='C' && c3=='C') servoRampAC_C.go(180-angle, duration);
      if(c1=='A' && c2=='B' && c3=='A') servoRampAB_A.go(180-angle, duration);
      if(c1=='A' && c2=='B' && c3=='B') servoRampAB_B.go(angle, duration);
      if(c1=='B' && c2=='C' && c3=='B') servoRampBC_B.go(180-angle, duration);
      if(c1=='B' && c2=='C' && c3=='C') servoRampBC_C.go(angle, duration);
      if(c1=='F' && c2=='L' && c3=='A') servoRampFlag.go(angle, duration);
    }
    else if(strstr(comunication_InBuffer, "Z ")){
      int a1=90,a2=90,a3=90,a4=90,a5=90,t=0;
      sscanf(comunication_InBuffer, "Z %i %i %i %i %i %i",&a1,&a2,&a3,&a4,&a5,&t);
      int pose[NB_SERVO_ARM_M];
      pose[0]=a1;
      pose[1]=a2;
      pose[2]=a3;
      pose[3]=a4;
      pose[4]=a5;
      sprintf(comunication_OutBuffer, "OK");//max 29 Bytes
      comunication_write();//async
      setArmPose(pose, t);
    }
    else if(strstr(comunication_InBuffer, "pump set ")){
      sprintf(comunication_OutBuffer, "OK");//max 29 Bytes
      comunication_write();//async
      char c1, c2, c3;
      int value=0;
      sscanf(comunication_InBuffer, "pump set %c%c%c %i", &c1, &c2, &c3, &value);
      value = value?1:0;
      if(c1=='L' && c2=='E' && c3=='F') digitalWrite(pinNumberPump_L, value);
      if(c1=='R' && c2=='I' && c3=='G') digitalWrite(pinNumberPump_R, value);
    }
    else{
      sprintf(comunication_OutBuffer,"ERROR");
      comunication_write();//async
    }
    comunication_cleanInputs();
  }
}

void updateServos(){
  servoAC_A.write(servoRampAC_A.update());
  servoAC_C.write(servoRampAC_C.update());
  servoAB_A.write(servoRampAB_A.update());
  servoAB_B.write(servoRampAB_B.update());
  servoBC_B.write(servoRampBC_B.update());
  servoBC_C.write(servoRampBC_C.update());
  servoFlag.write(servoRampFlag.update());
  for(int i=0;i<NB_SERVO_ARM_M;i++){
    servos_M[i].write(servosRamp_M[i].update());
  }
}

unsigned long lastControlMillis = 0;
unsigned long lastPositionMillis = 0;
unsigned long lastLedTime = 0;
float sinCounter = 0;
/*void loop()
{
  //Read commands
  executeOrder();

  //Spin motors
  motorA.spin();
  //motorB.spin();
  //motorC.spin();

  unsigned long currMillis = millis();
  
  //Run position loop
  int positionMillis = 1000/positionFrequency;
  if(currMillis - lastPositionMillis >= positionMillis){
    lastPositionMillis = currMillis;
    updatePosition();
  }
  
  //Run control loop
  int controlMillis = 1000/controlFrequency;
  if(currMillis - lastControlMillis >= controlMillis){
    lastControlMillis = currMillis;
    control();
  }
  
  updateServos();

  // blink led
  if(currMillis - lastLedTime >= 500){
    ledValue=!ledValue;
    digitalWrite(LED_PIN, ledValue);
    lastLedTime = currMillis;
  }
  
}*/

/*unsigned long lastPrintMillis = 0;
void loop(){
  motorA.setSpeed(0);
  motorB.setSpeed(0);
  motorC.setSpeed(0);
  motorA.spin();
  motorB.spin();
  motorC.spin();
  unsigned long now = millis();
  if(now-lastPrintMillis> 100){
    lastPrintMillis = now;
    Serial.print(motorA.getAndResetDistanceDone()*1000.0d);
    Serial.print(" ");
    Serial.print(motorB.getAndResetDistanceDone()*1000.0d);
    Serial.print(" ");
    Serial.println(motorC.getAndResetDistanceDone()*1000.0d);
  }
}*/

/*void loop(){
  motorB.setSpeed(0.1);
  //motorB.setSpeed(0.4);
  //motorC.setSpeed(0.4);
  while(1){
    motorB.spin();
  }
}*/

void loop()
{
    executeOrder();
    /*updatePosition();
    control();
    updateServos();*/
    unsigned long currMillis = millis();
  
    //Run position loop
    int positionMillis = 1000/positionFrequency;
    if(currMillis - lastPositionMillis >= positionMillis){
      lastPositionMillis = currMillis;
      updatePosition();
    }
    
    //Run control loop
    int controlMillis = 1000/controlFrequency;
    if(currMillis - lastControlMillis >= controlMillis){
      lastControlMillis = currMillis;
      control();
    }
    
    updateServos();
  
    // blink led
    if(currMillis - lastLedTime >= 500){
      ledValue=!ledValue;
      digitalWrite(LED_PIN, ledValue);
      lastLedTime = currMillis;
    }
  
    motorA.spin();
    motorB.spin();
    motorC.spin();
    return;
}
