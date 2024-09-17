#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/interrupt.h>


const unsigned long R1_Value = 1655000ul;
const unsigned long R2_Value = 218000ul;
const unsigned long R3_Value = 18070ul;
const unsigned long R4_Value = 467ul;
const unsigned long R_Values[] = {R1_Value, R2_Value, R3_Value, R4_Value};

const int R1_PIN = 3;
const int R2_PIN = 4;
const int R3_PIN = 5;
const int R4_PIN = 8;
const int R_PINS[] = {R1_PIN, R2_PIN, R3_PIN, R4_PIN};

const int Mode1_PIN = 9;
const int Mode2_PIN = 2;

const int MeasPos_PIN = A1;
const int MeasNeg_PIN = A2;

const int Comp1_PIN = 6;
const int Comp2_PIN = 7;

int selected_R =0;


//define Custom Characters
LiquidCrystal_I2C lcd(0x27,20,4);
uint8_t ohmCahr[8] = {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0A, 0x1B};
uint8_t susCahr[8] = {0x00, 0x00, 0x0E, 0x18, 0x1E, 0x0E, 0x0A, 0x00};
uint8_t capCahr[8] = {0x04, 0x04, 0x04, 0x1F, 0x00, 0x1F, 0x04, 0x04};
uint8_t resCahr[8] = {0x04, 0x04, 0x0E, 0x0A, 0x0A, 0x0E, 0x04, 0x04};
uint8_t volCahr[8] = {0x00, 0x00, 0x11, 0x11, 0x1B, 0x0A, 0x0E, 0x04};
uint8_t Load1Cahr[8] = {0x01, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x01};
uint8_t Load2Cahr[8] = {0x10, 0x18, 0x08, 0x08, 0x08, 0x08, 0x18, 0x10};
uint8_t Load3Cahr[8] = {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
uint8_t Load4Cahr[8] = {0x1F, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x1F};
uint8_t Load5Cahr[8] = {0x1F, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x1F};


//Timer 
volatile int ovfCount= 0;
volatile unsigned long count = 0;
volatile bool inSetup  = true;
volatile bool CaptFlag = false;
ISR(TIMER1_CAPT_vect){
  // for some reason interrupt triggers after starting timer
  if (inSetup) return;
  if (!digitalRead(MeasPos_PIN)) return;

  
  count = (((unsigned long)ovfCount)*65536ul) + ((unsigned int)ICR1);
  ovfCount = 0;
  CaptFlag = true;
  
  DisableCount();
  
}

ISR(TIMER1_OVF_vect){
  ovfCount++;
}

unsigned long getCurrentCount(){
  return (((unsigned long)ovfCount)*65536ul) + ((unsigned int)TCNT1);
}

void Init_Comparator_Timer(){
   TCCR1A = 0;
   TCCR1B = (1<<ICNC1);//|(1<<ICES1); //prescaler of 8
   PORTB &=~((1<<PD6)|(1<<PD7));
   DDRB  &=~((1<<PD6)|(1<<PD7));
   ACSR  |= (1<<ACIC);

   EnableCount();
   delay(1);
   DisableCount();
   inSetup = false;
}

void EnableCount(){
  cli();
  ovfCount= 0;
  TCNT1= 0;
  TIFR1  =0;
  TCCR1B |= (1<<CS11);
  TIMSK1 |= (1<<ICIE1) |(1<<TOIE1);
  TIFR1  =0;
  sei();  
}

void DisableCount(){
  TIMSK1 &= ~(1<<ICIE1);
  TCCR1B &= ~(1<<CS11);  
}

void setup(){
  //setup inputs
  pinMode(MeasPos_PIN, INPUT);
  pinMode(MeasNeg_PIN, INPUT);

  
  pinMode(Mode1_PIN, INPUT_PULLUP);
  pinMode(Mode2_PIN, INPUT_PULLUP);

  
  pinMode(R1_PIN, INPUT);
  pinMode(R2_PIN, INPUT);
  pinMode(R3_PIN, INPUT);
  pinMode(R4_PIN, INPUT);
 
  Init_Comparator_Timer();
  
  Serial.begin(9600);
  while (!Serial) {
    ; 
  }

  //setup LCD
  lcd.init();                      
  lcd.backlight();
  
  lcd.createChar(0, ohmCahr);
  lcd.createChar(1, susCahr);
  lcd.createChar(2, capCahr);
  //lcd.createChar(2, resCahr);
  lcd.createChar(3, Load1Cahr);
  lcd.createChar(4, Load2Cahr);
  lcd.createChar(5, Load3Cahr);
  lcd.createChar(6, Load4Cahr);

  lcd.home();
 
  lcd.setCursor(0, 1);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(14, 1);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(0, 0);
  lcd.write(2);
  lcd.setCursor(15, 0);
  lcd.write(2);
  
}


bool firstPass = true;
void loop(){
  lcd.setCursor(14, 0);
  lcd.print((char)(selected_R+49));
  
  if (!digitalRead(Mode1_PIN)){
    lcd.createChar(2, capCahr);

    pinMode(MeasNeg_PIN, OUTPUT);
    digitalWrite(MeasNeg_PIN, LOW);
    pinMode(MeasPos_PIN, INPUT);
     
    pinMode(R1_PIN, INPUT);
    pinMode(R2_PIN, INPUT);
    pinMode(R3_PIN, INPUT);
    pinMode(R4_PIN, OUTPUT);
    digitalWrite(R4_PIN, LOW);

    //Wait until the cap is discharged
    int inVolt =0;
    while((inVolt = analogRead(MeasPos_PIN)) > 1){
      displayILoadingBar((1024-inVolt));
      
    }
    displayILoadingBar(1024);
    delay(1);
    pinMode(R4_PIN, INPUT);


    //Load cap and time loading time
    EnableCount();
    pinMode(R_PINS[selected_R], OUTPUT);
    digitalWrite(R_PINS[selected_R], HIGH);

    while(!CaptFlag){
      int aread = analogRead(MeasPos_PIN);
      displayLoadingBar(aread*3/2);

      // change resister if too big and restart measurement
      if(firstPass && (getCurrentCount()>800000ul)){
        firstPass = false;
        selected_R = 3;
        DisableCount();
        return; 
      }
    }
    CaptFlag = false;
    displayLoadingBar(1024);

    double cval = (count/ (1.0986122886681096913952452369225*2000000ul))/R_Values[selected_R];
    printValue(cval);
    lcd.print("F");
    Serial.println("F");

    if (count < 2000000ul){//change resister if too small
      if (selected_R > 0) 
        selected_R--;
      else 
        firstPass = true;
    }

  }
  else if (!digitalRead(Mode2_PIN)){
    firstPass = true;
    lcd.createChar(2, resCahr);
    
    pinMode(R1_PIN, INPUT);
    pinMode(R2_PIN, INPUT);
    pinMode(R3_PIN, INPUT);
    pinMode(R4_PIN, INPUT);
      
    pinMode(MeasNeg_PIN, OUTPUT);
    digitalWrite(MeasNeg_PIN, LOW);
    pinMode(MeasPos_PIN, INPUT);

    
    pinMode(R_PINS[selected_R], OUTPUT);
    digitalWrite(R_PINS[selected_R], HIGH);
    
    delay(1);
    
    int sum = 0;
    for(int i = 0; i<8; i++){
      sum+= analogRead(MeasPos_PIN);
    }

    displayLoadingBar(sum/8);
    double rVal = R_Values[selected_R]*(1023*8.0/(1023*8-sum)-1);
    
    printValue(rVal);
    lcd.write(0);
    Serial.println("Ohm");
    
    //Adjust resister
    if (sum > 1024ul*8*9/10){
      if (selected_R>0) selected_R--;
    }
    else if (sum < 1024ul*8/10){
      if (selected_R<3) selected_R++;
    }
    delay(100);

    
  }
  else {
    firstPass = true;
    lcd.createChar(2, volCahr);

    pinMode(R1_PIN, INPUT);
    pinMode(R2_PIN, INPUT);
    pinMode(R3_PIN, INPUT);
    pinMode(R4_PIN, INPUT);

    pinMode(MeasNeg_PIN, OUTPUT);
    digitalWrite(MeasNeg_PIN, LOW);
    pinMode(MeasPos_PIN, INPUT);

    delay(1);
    
    int sum = 0;
    for(int i = 0; i<8; i++){
      sum+= analogRead(MeasPos_PIN);
    }

    displayLoadingBar(sum/8);
    double vval = 5.0 * sum /(1023*8);
    
    printValue(vval);
    lcd.write('V');
    Serial.println("V");
    
  }

}


void displayLoadingBar( int adcVal){
  int temp = (adcVal*50ul+512)/1024;
  
  char customload = (0xFE<<4-(temp%5));
  Load5Cahr[2] = customload;
  Load5Cahr[3] = customload;
  Load5Cahr[4] = customload;
  Load5Cahr[5] = customload;
  lcd.createChar(7, Load5Cahr);
  
  
  lcd.setCursor(2, 1);
  lcd.write(3);
  adcVal -= 51;
  for(int i=0; i<10; i++){
    
    if (temp>= 5)
      lcd.write(6);
    else if(temp>0)
      lcd.write(7);
    else
      lcd.write(5);

    temp-=5;
  }
  lcd.write(4);
}

void displayILoadingBar( int adcVal){
  int temp = (adcVal*50ul+512)/1024;
  
  char customload = ~(0xFE<<4-(temp%5));
  Load5Cahr[2] = customload;
  Load5Cahr[3] = customload;
  Load5Cahr[4] = customload;
  Load5Cahr[5] = customload;
  lcd.createChar(7, Load5Cahr);
  
  
  lcd.setCursor(2, 1);
  lcd.write(3);
  adcVal -= 51;
  for(int i=0; i<10; i++){
    
    if (temp>= 5)
      lcd.write(5);
    else if(temp>0)
      lcd.write(7);
    else
      lcd.write(6);

    temp-=5;
  }
  lcd.write(4);
}


//not fast in any way
void printValue(double d){
  
  char unit = 0;
  if(d > 1000){
    d/=1000;
    if(d > 1000){
      d/=1000;
      if(d > 1000){
        d/=1000;//Giga
        unit= 'G';
      }
      else{//Mega
        unit= 'M';
      }
    }
    else{//kilo
      unit= 'k';
    }
  }
  else if (d<1){
    d*= 1000;
    if(d>=1){
      unit = 'm';
    }
    else{
      d*= 1000;
      if(d>=1){
        unit = 'u';
      }
      else{
        d*= 1000;
        if(d>=1){
          unit = 'n';
        }
        else{
          d*= 1000;
          if(d>=1){
            unit = 'p';
          }
          else{
            d*= 1000;
            unit = 'f';
          }
        }
      }
    }
  }
  
  
  lcd.setCursor(2, 0);
  if (unit==0) lcd.print(" ");
  
  char sbuff[] = {0,0,0,0,0,0,0,0,0,0,0,0};
  dtostrf(d,7,3,sbuff);
  lcd.print(sbuff);
  Serial.print(sbuff);
  
  sprintf(sbuff," %c",unit);
  lcd.print(sbuff);
  Serial.print(sbuff);
  
  
  
}
