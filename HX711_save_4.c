#define USE_EEPROM
#ifdef USE_EEPROM
#include <EEPROM.h>
#endif
#include "HX711.h"

#define DOUT 16 //A2:16
#define CLK  17 //A3:17

//#define calibration_factor 1200.0
#define calibration_factor 6000.0
#define RXLED LED_BUILTIN
#define TRIGGER_D_PIN 10

#define ACTIVE_BOUND 10
#define ACTIVE_DEFAULT 50
#define GUARD_MS 250 // don't trigger in this ms
#define TRIGGER_MS 80
#define QS_ACTIVE_LEVEL LOW

struct SaveData {
  signed char qs_dir;
  unsigned char qs_active;
  unsigned char qs_kill_time;
  unsigned char qs_trigger_mode;
};

static const SaveData default_value = {
  -1,
  ACTIVE_DEFAULT,
  TRIGGER_MS,
  QS_ACTIVE_LEVEL,
};

static SaveData eeprom_token;

#define SHOW_SETTING {\
  Serial.print(F("DIR = "));\
  Serial.print(eeprom_token.qs_dir, DEC);\
  Serial.print(F(", ACTIVE = "));\
  Serial.print(eeprom_token.qs_active, DEC);\
  Serial.print(F(", KILL = "));\
  Serial.print(eeprom_token.qs_kill_time, DEC);\
  Serial.print(F(", TRIGGER MODE = "));\
  Serial.println(eeprom_token.qs_trigger_mode, DEC);\
  } while(0);
        
static boolean led_toggle = 0;
static boolean trigger = false, test_trigger = false;
//static int print_reading = true;
static int print_reading = false;
static unsigned long start_blinking_time=0;

HX711 scale(DOUT, CLK);

void setup() {               
  Serial.begin(115200);
  Serial.print(F(__DATE__));
  Serial.print(" ");
  Serial.println(F(__TIME__));
  
  scale.set_scale(calibration_factor);
  scale.tare(); //Reset the scale to 0
    
  pinMode(TRIGGER_D_PIN, OUTPUT);
  digitalWrite(TRIGGER_D_PIN, !eeprom_token.qs_trigger_mode); // Low as not to ground ignition
  trigger=false;
  test_trigger = false;

  pinMode(RXLED, OUTPUT);  // Set RX LED as an output  

#ifdef USE_EEPROM
  EEPROM.get(0, eeprom_token);
  // reset eeprom if new
  if(eeprom_token.qs_kill_time < 10 || eeprom_token.qs_kill_time >130 || eeprom_token.qs_dir>1 || \
    eeprom_token.qs_active<40 || eeprom_token.qs_active>250 || \
    eeprom_token.qs_trigger_mode > 1)
  {
    memcpy(&eeprom_token, &default_value, sizeof(SaveData));
    Serial.println(F("EEPROM data not valid"));
  }
#else
    memcpy(&eeprom_token, &default_value, sizeof(SaveData));
#endif
  SHOW_SETTING;
}

// the loop routine runs over and over again forever:
void loop() {
  int reading = (int)scale.get_units();
  if(print_reading && (reading > 3 || reading < -3))
    Serial.println(reading);
  
  reading=reading*eeprom_token.qs_dir;
  if (test_trigger == true || (reading >= eeprom_token.qs_active && trigger==false))
  //if(true)
  {
      Serial.print("triggered ");
      Serial.println(reading, DEC);
      digitalWrite(TRIGGER_D_PIN, eeprom_token.qs_trigger_mode);
      delay(eeprom_token.qs_kill_time);
      digitalWrite(TRIGGER_D_PIN, !eeprom_token.qs_trigger_mode);
      delay(GUARD_MS);
      trigger=true;   
      test_trigger = false;
      start_blinking_time=millis()+500;
  }
  else if (reading < eeprom_token.qs_active)
  {
    if(trigger == true)
    {
        Serial.print("released ");
        Serial.println(reading, DEC);
    }
    trigger=false;
  }
  
  if(start_blinking_time>millis())
    led_toggle=(millis()/100)&1;
  else
    led_toggle=(millis()/1000)&1;
  digitalWrite(RXLED, led_toggle);



  if (Serial.available()) {
    char inChar = (char)Serial.read();

    Serial.println("");
    switch (inChar)
    {
      case 's':
        break;
      case 'q':
        memcpy(&eeprom_token, &default_value, sizeof(SaveData));
        break;
#ifdef USE_EEPROM        
      case 'w':
        EEPROM.put(0, eeprom_token);
        Serial.println(F("save done"));
        break;
#endif        
      case 'd':
        eeprom_token.qs_dir = -eeprom_token.qs_dir;
        break;
      case '1':
        if(eeprom_token.qs_active<250) eeprom_token.qs_active+=5;
        break;
      case '2':
        eeprom_token.qs_active-=5;
        if(eeprom_token.qs_active<ACTIVE_BOUND) eeprom_token.qs_active=ACTIVE_BOUND;
        break;
      case '3':
        if(eeprom_token.qs_active<250) eeprom_token.qs_active+=10;
        break;
      case '4':
        eeprom_token.qs_active-=10;
        if(eeprom_token.qs_active<ACTIVE_BOUND) eeprom_token.qs_active=ACTIVE_BOUND;
        break;
      case '5':
        if(eeprom_token.qs_kill_time<130) eeprom_token.qs_kill_time+=5;
        break;
      case '6':
        if(eeprom_token.qs_kill_time>50) eeprom_token.qs_kill_time-=5;
        break;
      case 'p':
        print_reading = !print_reading;
        Serial.print(F("print="));
        Serial.println(print_reading, DEC);        
        break;
      case 't':
        test_trigger = true;
        break;
      case 'y':
        eeprom_token.qs_trigger_mode = !eeprom_token.qs_trigger_mode;
        digitalWrite(TRIGGER_D_PIN, !eeprom_token.qs_trigger_mode);
        break;
      case 'h':
        Serial.println(F("1: active+1"));
        Serial.println(F("2: active-1"));
        Serial.println(F("3: active+5"));
        Serial.println(F("4: active-5"));
        Serial.println(F("5: killtime+5"));
        Serial.println(F("6: killtime-5"));
        Serial.println(F("s: show"));
        Serial.println(F("q: reset to factory"));
        Serial.println(F("d: toggle direction"));
        Serial.println(F("p: toggle print reading"));
        Serial.println(F("t: test trigger"));
        Serial.println(F("y: toggle trigger active mode"));
#ifdef USE_EEPROM        
        Serial.println(F("w: write to eeprom"));
#endif        
        Serial.println(F("h: this help"));
        Serial.print(F(__DATE__));
        Serial.print(F(" "));
        Serial.println(F(__TIME__));
        break;
    }
    Serial.print(F("reading="));
    Serial.println(reading);      
    SHOW_SETTING;
  }
  Serial.flush();   
    
}

