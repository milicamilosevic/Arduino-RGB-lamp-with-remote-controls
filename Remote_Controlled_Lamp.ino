#include <dht.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#include <ir_Lego_PF_BitStreamEncoder.h>

#include "Key_CODES.cpp"

//Analog pins
#define POT 2
#define PHOTO_RES 1

//Digital pins
#define IR_RCV_PIN 2
#define RED_PIN 9
#define DHT_PIN 4
#define GREEN_PIN 5
#define BLUE_PIN 6

//LCD
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);
bool displayChange = true;

//IR reciever
IRrecv irrecv(IR_RCV_PIN);
decode_results results;


//DHT11
dht DHT;
long const delayDHT = 10000;
long timePreviousDHT = 0;

int pot_value;  //Vrijednost ocitana sa potencimetra (opseg: 0 - 1023)
int color_value;  //Vrijednost sa potenciometra prilagodjena za RGB model (opseg: 0 - 767)
int R_val, G_val, B_val;  //Vrijednosti pojedinacnih komponenti boje (opseg: 0-255)
int R_val_M, G_val_M, B_val_M;
int lightness_value;  //Vrijednost ocitana sa fotorezistora (opseg: 0 - 1023)

String state = "sensor";

void setup() {
  Serial.begin(9600);
  
  pinMode(POT, INPUT);
  pinMode(PHOTO_RES, INPUT);
  
  pinMode(IR_RCV_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  irrecv.enableIRIn();

  R_val_M=128;
  G_val_M=128;
  B_val_M=128;
  
  delay(500);
}



void pot_to_RGB(){
  color_value=map(pot_value, 0, 1023, 0, 767);
  if(color_value<256){
    R_val=255-color_value;
    G_val=color_value;
    B_val=0;  
  }else if(color_value<512){
    color_value-=256;
    R_val=0;
    G_val=255-color_value;
    B_val=color_value;
  }else{
    color_value-=512;
    R_val=color_value;
    G_val=0;
    B_val=255-color_value;
  }
}



void dim_color(){
  float scale_factor;
  if(lightness_value<256)
    scale_factor=1;       //Ukoliko je osvjetljenost ispid 1/4, uzima se puni intenzitet boje
  else if(lightness_value>=768)
    scale_factor=0;       //Ukoliko je osvijetljenost iznad 3/4 lampica se ne pali.
  else{
    lightness_value=map(lightness_value, 0, 1023, 0, 512);
    lightness_value=512-lightness_value;
    scale_factor=lightness_value/512.0;  
  }
  R_val=R_val*scale_factor;
  G_val=G_val*scale_factor;
  B_val=B_val*scale_factor;
}



void display(){
  lcd.clear();
  if(state=="red"){
    lcd.setCursor(0, 0);
    lcd.print("Crvena:");
    printProgressBar(R_val_M);
    displayChange=false;
    return; 
  }else if(state=="green"){
    lcd.setCursor(0, 0);
    lcd.print("Zelena:");
    printProgressBar(G_val_M);
    displayChange=false;
    return; 
  }else if(state=="blue"){
    lcd.setCursor(0, 0);
    lcd.print("Plava:");
    printProgressBar(B_val_M);
    displayChange=false;
    return;  
  }else if(state=="sensor"){
    lcd.setCursor(0, 0);
    lcd.print("1-Senzor mod");
  }else if(state=="manual"){
    lcd.setCursor(0, 0);
    lcd.print("2-Manuelni mod");
  } 
  
  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print((int)DHT.temperature);
  lcd.print((char)223);
  lcd.print("C   V: ");
  lcd.print((int)DHT.humidity);
  lcd.print("%");

  displayChange=false;
}

void printProgressBar(int value){
  lcd.setCursor(0, 1);
  for(int i=0; i<value; i+=16)
    lcd.print((char)255);
}



void loop() {
  if (irrecv.decode(&results)){
        if(results.value == Key_1_CODE){
          state="sensor";
        }
        
        if(results.value == Key_2_CODE){
          if(state=="manual")
            state="red";
          else if(state=="red")
            state="green";
          else if(state=="green")
            state="blue";
          else
            state="manual";
        }

        if(results.value == Minus_CODE){
          if(state=="red")
            if(R_val_M>0)
              R_val_M-=16;
          if(state=="green")
            if(G_val_M>0)
              G_val_M-=16;
          if(state=="blue")
            if(B_val_M>0)
              B_val_M-=16;
        }

        if(results.value == Plus_CODE){
          if(state=="red")
            if(R_val_M<256)
              R_val_M+=16;
          if(state=="green"){
            if(G_val_M<256)
              G_val_M+=16;
          }
          if(state=="blue")
            if(B_val_M<256)
              B_val_M+=16;
        }
        
        displayChange = true;
        irrecv.resume();
  }

  if(timePreviousDHT==0 || millis()>timePreviousDHT+delayDHT){
    delay(100);
    int chk = DHT.read11(DHT_PIN);
    displayChange = true;
    timePreviousDHT=millis();  
  }
  
  if(state=="sensor"){
    pot_value=analogRead(POT);
    lightness_value=analogRead(PHOTO_RES);
    pot_to_RGB();
    dim_color();
    analogWrite(RED_PIN, R_val);
    analogWrite(GREEN_PIN, G_val);
    analogWrite(BLUE_PIN, B_val);
  }else{
    if(R_val_M==256) analogWrite(RED_PIN,255);
    else analogWrite(RED_PIN,R_val_M);
    if(G_val_M==256) analogWrite(GREEN_PIN,255);
    else analogWrite(GREEN_PIN,G_val_M);
    if(B_val_M==256) analogWrite(BLUE_PIN,255);
    else analogWrite(BLUE_PIN,B_val_M);
  }

  if(displayChange)
    display();
}
