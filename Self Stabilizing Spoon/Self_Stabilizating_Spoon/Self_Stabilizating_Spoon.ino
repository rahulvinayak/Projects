//Self Stabilizing Spoon for Parkinson's Patients
#include<Wire.h>
#include <ESP32Servo.h>
const int MPU_addr=0x68; // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float delta_t = 0.005;
float pitchAcc,rollAcc, pitch, roll, pitched;
float P_CompCoeff= 0.98;
// ============= INITIAL SETUP ===========================================
Servo myservo1, myservo2;
void setup(){
Wire.begin();//I2c Initialization
Wire.beginTransmission(MPU_addr);
Wire.write(0x6B); // PWR_MGMT_1 register
Wire.write(0); // set to zero (wakes up the MPU-6050)
Wire.endTransmission(true);
Serial.begin(115200);
myservo1.attach(2);
myservo2.attach(4);
}
// ============= MAIN LOOP ===============================================
void loop(){
Wire.beginTransmission(MPU_addr);
Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
Wire.endTransmission(false);
Wire.requestFrom(MPU_addr,14,true); // request a total of 14 registers
AcX=Wire.read()<<8|Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C
AcY=Wire.read()<<8|Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E
AcZ=Wire.read()<<8|Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40
GyX=Wire.read()<<8|Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44
GyY=Wire.read()<<8|Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46
GyZ=Wire.read()<<8|Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48
//Complementary filter
long squaresum_P=((long)GyY*GyY+(long)AcY*AcY);
long squaresum_R=((long)GyX*GyX+(long)AcX*AcX);
pitch+=((-AcY/40.8f)*(delta_t));
roll+=((-AcX/45.8f)*(delta_t)); 
pitchAcc= atan((AcY/sqrt(squaresum_P))*RAD_TO_DEG);
rollAcc =atan((AcX/sqrt(squaresum_R))*RAD_TO_DEG);
pitch =(P_CompCoeff*pitch + (1.0f-P_CompCoeff)*pitchAcc);//pitch
roll =(P_CompCoeff*roll + (1.0f-P_CompCoeff)*rollAcc);
myservo1.write(roll + 40);
myservo2.write(pitch + 120);
}