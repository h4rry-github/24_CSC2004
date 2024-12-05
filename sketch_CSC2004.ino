#include "RX9QR.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>      

#define EMF_pin 0
#define THER_pin 1
#define MQ3_pin 16
#define ADCvolt 5
#define ADCResol 1024
#define Base_line 432
#define meti 60  
#define mein 120
#define servo_pin 9

float cal_A = 376.3;
float cal_B = 50.55; 

#define cr1  700    
#define cr2  1000     
#define cr3  2000     
#define cr4  4000  
#define C1 0.00230088
#define C2 0.000224
#define C3 0.00000002113323296
float Resist_0 = 15;

unsigned int time_s = 0;
unsigned int time_s_prev = 0;
unsigned int time_s_set = 1;
extern volatile unsigned long timer0_millis;

int status_sensor = 0;
unsigned int co2_ppm = 0;
unsigned int co2_step = 0;
unsigned int c2h5oh_value = 0;
float EMF = 0;
float THER = 0;

LiquidCrystal_I2C lcd(0x27,16,2); 
RX9QR RX9(cal_A, cal_B, Base_line, meti, mein, cr1, cr2, cr3, cr4);
Servo servo1;

int time_record = 0, co2_mean = 0, c2h5oh_mean = 0;
int isBlowed = 0, isDetected = 0;
void setup() {
  Serial.begin(9600);
  lcd.init();            
  lcd.backlight(); 
  lcd.setCursor(0,0);
  pinMode(servo_pin, OUTPUT);
  servo1.attach(servo_pin);
  servo1.write(180);  
}

int co2_measure(){
 EMF = analogRead(EMF_pin);
    delay(1);
    EMF = EMF / (ADCResol - 1);
    EMF = EMF * ADCvolt;
    EMF = EMF / 6;
    EMF = EMF * 1000; 
    THER = analogRead(THER_pin);
    delay(1);
    THER = 1/(C1+C2*log((Resist_0*THER)/(ADCResol-THER))+C3*pow(log((Resist_0*THER)/(ADCResol-THER)),3))-273.15; EMF = analogRead(EMF_pin) / (ADCResol - 1) * ADCvolt / 6 * 1000;
  delay(1);
  THER = analogRead(THER_pin);
  THER = 1/(C1+C2*log((Resist_0*THER)/(ADCResol-THER))+C3*pow(log((Resist_0*THER)/(ADCResol-THER)),3))-273.15;
  delay(1);
  return RX9.cal_co2(EMF,THER);
}

int c2h5oh_measure(){
  return analogRead(MQ3_pin);
}

void show_info1(){
  lcd.setCursor(0,0);
  lcd.print("Please blow on the sensor.");
}

void show_info2(){
  lcd.setCursor(0,0);
  lcd.print("Verification has been completed.");
}
 
void show_error1(){
  lcd.setCursor(0,0);
  lcd.print("No breathing detected. Please try again.");
}

void show_error2(){
  lcd.setCursor(0,0);
  lcd.print("Ethanol detected, restricted use.");
}

void operate_servo(){
  int pos;
  for(pos = 180; pos >= 0; pos--){
    servo1.write(pos);                      
    delay(10);          
  }
  delay(15000);
  for (pos = 0; pos <= 180; pos++){
    servo1.write(pos);                      
    delay(10);          
  }
}

void loop() {
  time_s = millis()/1000;
  if(time_s - time_s_prev >= time_s_set){
    time_s_prev = time_s;
    EMF = analogRead(EMF_pin);
    delay(1);
    EMF = EMF / (ADCResol - 1);
    EMF = EMF * ADCvolt;
    EMF = EMF / 6;
    EMF = EMF * 1000;
    THER = analogRead(THER_pin);
    delay(1);
    THER = 1/(C1+C2*log((Resist_0*THER)/(ADCResol-THER))+C3*pow(log((Resist_0*THER)/(ADCResol-THER)),3))-273.15;
    status_sensor = RX9.status_co2(); 
    co2_ppm = RX9.cal_co2(EMF,THER);   
    co2_step = RX9.step_co2(); 
    if(time_record < 11){
      c2h5oh_value = c2h5oh_measure();
      if(status_sensor){ 
        if(time_record == 0){
          show_info1();
        }
        time_record++;
        co2_mean += co2_ppm;
        c2h5oh_mean += c2h5oh_value;
      }else{
        lcd.setCursor(0,0);
        lcd.print("Please wait...");
      }
    }else{
      co2_mean /= 10;
      c2h5oh_mean /= 10;
      if(co2_mean > 900){
        isBlowed = 1;
      }
      if(c2h5oh_mean > 600){
        isDetected = 1;
      }
      if(isBlowed && !isDetected){
        show_info2();
        operate_servo();
        isBlowed = 0;
        isDetected = 0;
      }else if(!isBlowed){
        show_error1();
        isBlowed = 0;
        isDetected = 0;
      }else{
        show_error2();
        isBlowed = 0;
        isDetected = 0;
      }
      time_record = 0;
      co2_mean = 0;
      c2h5oh_mean = 0;
      delay(10000);
    }
  }
}