/*
 cv_v4_3.ino
 
 Version: 1.0.1
 
 Measures current while cycling the anode potential at a set rate
   - Start with both off for 30 minutes, then run CV using initial potential
 as starting & ending point (per cycle)
 
 
 ==================================================
 ***ONLY WORKS ON CIRCUIT V4.3********
 ==================================================
 
 Last modified: March 14, 2015
  
 1.0.1 Changes:
  - Refactored vol_nor to anodePotential
 

 
 Adam Burns - burns7@illinois.edu
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;
int power1state = LOW;
int power2state = LOW;
boolean offState = true;
boolean setPotential = false;

int StablePotential = 0;
const int Power1 = 6; // power control for MCP4161 #1 to D6
const int Power2 = 5; // power control for MCP4161 #2 to D5
unsigned long offDuration = 300000; // off mode for 30 mins (1,800,000ms)
int numOfDigits = 5;
int netCycles=2; // target number of cycles
int delayInterval = 1000; // rate of sweep (1000=1s -> 1mv/1s)
int csPin1 = 7; //Chip select Digital Pin 7 for digital pot #1
int csPin2 = 3; //Chip select D3 for digital pot #2
/*
int vol_before_resistor; //Analog Pin 2
 int vol_after_resistor;  //Analog Pin 3
 
 int vol_reference;       //Analog Pin 4 *///no longer used
int trans_sig = 0;
int cnt = 0;
int resistor = 98.2; // R2 resistance in Ohms
int lsv_finished = 0;

double target_value;
double startingPotential=0;
//double tol = 0.001;
#define DEBUG 0 // set to 1 to print debug data to serial monitor


void setup()
{
  Serial.begin(9600);
  SPI.begin(); //Init SPI
  ads.begin();

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  //

  pinMode(csPin1, OUTPUT);
  pinMode(csPin2, OUTPUT);

  pinMode(Power1, OUTPUT);
  pinMode(Power2, OUTPUT);
  digitalWrite(Power1,HIGH);
  delay(200);
  digitalWrite(Power2, LOW);

  // Reset digipot to 0
  digitalWrite(csPin1, LOW);
  SPI.transfer(0);
  SPI.transfer(0);
  digitalWrite(csPin1, HIGH);
  delay(200);

  // Setting digipot to 0 again in case 1st attempt failed
  digitalWrite(csPin1, LOW);
  SPI.transfer(0);
  SPI.transfer(0);
  digitalWrite(csPin1, HIGH);
  delay(200);
  digitalWrite(Power1, LOW);

}

void loop()
{
  float multiplier = 0.125F; // ADS1115  1x gain   +/- 4.096V (16-bit results) 0.125mV
  double anondePotential;
  double last_anode;
  double current;
  double cell_vol;
  
  
  if(offState==true){    
    unsigned long currentMillis = millis();
    if((currentMillis>offDuration) && (offState==true))
    {
      power1state=HIGH;
      power2state=HIGH;
      offState=false;
      setPotential=true;
      digitalWrite(Power1,power1state);
      digitalWrite(Power2,power2state);
      delay(150); //allow current to stabilize
    }
  }


  if((offState==false) && (setPotential==true) && (StablePotential<=5))
  {


    double vol=ads.readADC_Differential_0_1();
    vol=vol * multiplier;

    current = ((vol)/(98.2));

    anondePotential = ads.readADC_Differential_2_3();
    anondePotential= (anondePotential * multiplier)/1000;

    cell_vol = ads.readADC_SingleEnded(1);
    cell_vol=(cell_vol * multiplier)/1000; 
    
    if (anondePotential < -0.352) trans_sig ++;
    else if (anondePotential > -0.342 && trans_sig >0) trans_sig --;

    digitalWrite(csPin1, LOW);
    SPI.transfer(0);
    SPI.transfer(trans_sig);
    digitalWrite(csPin1, HIGH);
    delay(500);
    
    if((anondePotential>=-0.36) && (anondePotential<= -0.33)){
      StablePotential++;
    }

  }

  if (StablePotential>5){
    setPotential=false;
  }


  double vol=ads.readADC_Differential_0_1();
  vol=vol * multiplier;

  current = ((vol)/(98.2));

  anondePotential = ads.readADC_Differential_2_3();
  anondePotential= (anondePotential * multiplier)/1000;

  cell_vol = ads.readADC_SingleEnded(1);
  cell_vol=(cell_vol * multiplier)/1000; 

#if DEBUG
  Serial.println();
  Serial.print("trans_sig: ");
  Serial.print(trans_sig);
  Serial.print(",  current: ");
  Serial.print(current, numOfDigits);
  Serial.print(",  annode potential:");
  Serial.print(anondePotential, numOfDigits);
  Serial.print(",  Cell vol:");
  Serial.print(cell_vol, numOfDigits);
  Serial.print(", Cnt: ");
  Serial.println(cnt);
  Serial.print(", Target: ");
  Serial.println(target_value, numOfDigits); 
#endif

  if (cnt==0){
    target_value=anondePotential;
    startingPotential=anondePotential;
  }

  //========== begin new code ============

  if((offState==false)&&(setPotential==false)){
    startingPotential=anondePotential;

    for(int i=0; i<netCycles; i++){

      // increase until target > 0
      while(target_value<=0){
        cnt ++;
        if (cnt % 2 == 0 && cnt != 0) 
        {
          target_value += 0.002;
        }
        if (cnt < 10) target_value = anondePotential; 
        else {
          if (cnt % 1 ==0)
          {
            if (anondePotential < target_value)
            {
              trans_sig ++;
              if(trans_sig > 255){ 
                trans_sig = 255; 
              }
              digitalWrite(csPin1, LOW);
              SPI.transfer(0);
              SPI.transfer(trans_sig);
              digitalWrite(csPin1, HIGH);
              delay(100); // allow voltage to stabilize before reading
            }
          }
        }
        double vol=ads.readADC_Differential_0_1();
        vol=vol * multiplier;

        current = ((vol)/(98.2));

        anondePotential = ads.readADC_Differential_2_3();
        anondePotential= (anondePotential * multiplier)/1000;

        cell_vol = ads.readADC_SingleEnded(1);
        cell_vol=(cell_vol * multiplier)/1000; 

        Serial.print(trans_sig);
        Serial.print("      ");
        Serial.print(current, 10);
        Serial.print("  ");
        Serial.print(anondePotential, 10);
        Serial.print("  ");
        Serial.print(cell_vol, 10);
        Serial.println("  ");


        delay(delayInterval);
      }



      // decrease until trans_sig = 0
      while(trans_sig>0){
        cnt++;

        if (cnt % 2 == 0 && cnt != 0) 
        {
          target_value -= 0.002;
        }
        if (cnt % 1 ==0)
        {
          if (anondePotential > target_value)
          {
            trans_sig --;
            if(trans_sig <=0 ){ 
              trans_sig = 0; 
            }
            digitalWrite(csPin1, LOW);
            SPI.transfer(0);
            SPI.transfer(trans_sig);
            digitalWrite(csPin1, HIGH);
            delay(100); // allow voltage to stabilize
          }
        }

        double vol=ads.readADC_Differential_0_1();
        vol=vol * multiplier;

        current = ((vol)/(98.2));

        anondePotential = ads.readADC_Differential_2_3();
        anondePotential= (anondePotential * multiplier)/1000;

        cell_vol = ads.readADC_SingleEnded(1);
        cell_vol=(cell_vol * multiplier)/1000; 

        Serial.print(trans_sig);
        Serial.print("      ");
        Serial.print(current, 10);
        Serial.print("  ");
        Serial.print(anondePotential, 10);
        Serial.print("  ");
        Serial.print(cell_vol, 10);
        Serial.println("  ");


        delay(delayInterval);
      }


      // increase back up to startingPotential
      while(target_value<startingPotential){

        cnt ++;
        if (cnt % 2 == 0 && cnt != 0) 
        {
          target_value += 0.002;
        }
        if (cnt < 10) target_value = anondePotential; 
        else {
          if (cnt % 1 ==0)
          {
            if (anondePotential < target_value)
            {
              trans_sig ++;
              if(trans_sig > 255){ 
                trans_sig = 255; 
              }
              digitalWrite(csPin1, LOW);
              SPI.transfer(0);
              SPI.transfer(trans_sig);
              digitalWrite(csPin1, HIGH);
              delay(100); // allow voltage to stabilize
            }
          }
        }

        double vol=ads.readADC_Differential_0_1();
        vol=vol * multiplier;

        current = ((vol)/(98.2));

        anondePotential = ads.readADC_Differential_2_3();
        anondePotential= (anondePotential * multiplier)/1000;

        cell_vol = ads.readADC_SingleEnded(1);
        cell_vol=(cell_vol * multiplier)/1000; 

        Serial.print(trans_sig);
        Serial.print("      ");
        Serial.print(current, 10);
        Serial.print("  ");
        Serial.print(anondePotential, 10);
        Serial.print("  ");
        Serial.print(cell_vol, 10);
        Serial.println("  ");


        delay(delayInterval);
      }

    }
    Serial.println("  ");
    Serial.println(" CV completed ");

    //halt the code
    while(true);
  }


  delay(1000);

}
