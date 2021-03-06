
//#############################################
//
//  Sensor Network Code
//  Richard Hawthorn
//  February 2015
//  
//  Collect data from sensors, send it to
//  be logged, processed and visulaised.
//
//  Sensors
//
//  Temperature
//  Humidity
//  Pressure
//  Sound
//  Light
//  Movement
//  Power
//  Door entry
//  Proximity
//  Air quality
//  Battery Levels
//
//  Visulaise
//
//  7 Segment
//  14 Segment
//  8x8 Matrix
//  8x8 bi-colour matrix
//
//
//#############################################


//#####################################
//  Turn on features and define pins
//#####################################

//Our node id
#define NODEID 12
#define GROUPID 28

//128 x 64 LCD screen
#define LCD128 true

//All sensors on or off
#define SENSORS false

//Led pixels enabled?
#define PIXELS false
#define PIXELS_PIN 8

#define PIR false
#define PIR_PIN 7

#define SOUND false
#define SOUND_PIN 17

#define LIGHT true
#define LIGHT_PIN 16

#define SENSOR2 false
#define SENSOR2_PIN 4

#define SENSOR3 false
#define SENSOR3_PIN 21

//How oftern should we send out readings (in minutes)?
#define PERIOD 1

//low power mode?
#define LOW_POWER false

//Individual Sensors
#define BASIC_HUMID false
#define BASIC_HUMID_TEMP false
#define BASIC_HUMID_PIN 8

#define HUMID true
#define HUMID_TEMP false

#define PRESSURE true
#define PRESSURE_TEMP false

#define TEMP true

#define VOLTAGE false
#define VOLTAGE_PIN A0

//building power
#define POWER false
#define POWER_PIN A1

#define LEDS false
#define LED1 3
#define LED2 5
#define LED3 6
#define LED4 9

#define HUB true
 
//Output serial debug strings
#define DEBUG true

#define MATRIX3208 false
#define MATRIX3208CS1 6
#define MATRIX3208WR 8
#define MATRIX3208DATA 9
#define MATRIX3208SPEED 100

#define SEGMENT14 false
#define SEGMENT7 false
#define MATRIX8X8 false
#define MATRIX8X8BI false

#define BUTTONS true

//uint8_t KEY[] = "ABCDABCDABCDABCD"; 

//#####################################
//  Get some includes sorted
//#####################################

#include <JeeLib.h>  
#include <Time.h>
#include <Wire.h>

#if MATRIX3208
  #include <HT1632.h>
  #include <font_5x4.h>
  #include <images.h>
#endif

#if PIXELS
  #include <Adafruit_NeoPixel.h>
#endif

#if HUMID
  #include "Adafruit_HTU21DF.h"
#endif

#if PRESSURE
  #include <Adafruit_MPL3115A2.h>
#endif

#if BASIC_HUMID
  #include "DHT.h"
#endif

#if POWER
  #include "EmonLib.h"  
#endif

#if TEMP
  #include "Adafruit_MCP9808.h"
#endif

#if LCD128
  #include "U8glib.h"
#endif

#if LOW_POWER
 // #include "LowPower.h"
#endif

#if BUTTONS
  #include "Adafruit_MPR121.h"
#endif

#if SEGMENT14 || SEGMENT7 || MATRIX8X8 || MATRIX8X8BI
  #include "Adafruit_LEDBackpack.h"
  #include "Adafruit_GFX.h"
#endif

//#####################################
//  Packetbuffer class
//#####################################
class PacketBuffer : public Print {
public:
    PacketBuffer () : fill (0) {}
    
    const byte* buffer() { return buf; }
    byte length() { return fill; }
    void reset() { fill = 0; }

#if ARDUINO < 100
    virtual void write(uint8_t ch)
        { if (fill < sizeof buf) buf[fill++] = ch; }
#else
    virtual size_t write(uint8_t ch) {
        if (fill < sizeof buf) {
            buf[fill++] = ch;
            return 1;
        }
        return 0;
    }
#endif
    
private:
    byte fill, buf[RF12_MAXDATA];
};


//#####################################
//  Some pre setup required
//#####################################

//Basic Humidity
#if BASIC_HUMID
  #define DHTTYPE DHT22
  DHT dht(BASIC_HUMID_PIN, DHTTYPE);
#endif

#if PIXELS
  //Neopixels
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIXELS_PIN, NEO_GRB + NEO_KHZ800);
#endif

#if POWER
  EnergyMonitor emon1;  
#endif

#if HUMID
  Adafruit_HTU21DF htu = Adafruit_HTU21DF();
#endif

#if PRESSURE
  Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
#endif

#if TEMP
  Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#endif

#if SEGMENT14
  Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
#endif

#if SEGMENT7
  Adafruit_7segment matrix = Adafruit_7segment();
#endif

#if MATRIX8X8
  Adafruit_8x8matrix matrix = Adafruit_8x8matrix();
#endif

#if MATRIX8X8BI
  Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();
#endif

#if BUTTONS
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t buttoncurrent = 0;
  uint16_t buttonlast = 0;
  
  char buttons[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  
  int btn_up = 0;
  int btn_right = 1;
  int btn_down = 2;
  int btn_left = 3;
  int btn_home = 4;
  int btn_cancel = 5;
  int btn_menu = 6;
  int btn_enter = 7;
  int btn_tl = 8;
  int btn_bl = 9;
  int btn_tr = 10;
  int btn_br = 11;
#endif

#if MATRIX3208
  int matrixLoop = 0;
  int matrixLength;
  char matrixString [50] = "HeyData";
  long millisCheck = 0;
#endif

//Serial strings
String RfIn = "";
boolean RfInComplete = false;
String toRf = "";
boolean toRfComplete = false;

String stringToProcess = "";

String dataPower;
String dataHumid;
String dataTemp;
String dataPressure;
String dataSound;
String dataLight;
String dataPir;
String dataSensor;
String dataVoltage;

//used for sending out sensor readings
int lastMinute = -1;
int lastSecond = -1;

//sound
int soundMax = 0;
int soundMin = 0;

//pir
boolean pir_movement = false;

//power
float power_max = 0;

//sensor
boolean sensor2_triggered = false;

//single leds
#if LEDS
  float ledLoop = 0;
#endif

//lcd
#if LCD128
  #define BACKLIGHT_LED_RED 3
  #define BACKLIGHT_LED_GREEN 5
  #define BACKLIGHT_LED_BLUE 6

  U8GLIB_LM6059 u8g(14, 4, 9, 7, 8);

  String lcd_temp = "0";
  String lcd_humid = "0";
  String lcd_pressure = "0";
  String lcd_power = "0";
  String lcd_light = "0";
  
  String lcd_page = "home";
  
  char* menu[]={
    "Backlight", 
    "Brightness",
    "Sensors", 
    "Buttons",
    "Sleep",
    "Reset"
  };
  
  int menu_pos = 0;
  int menu_selected = -1;
  
#endif

//payload to send over RF
PacketBuffer payload;   // temp buffer to send out rf data

//#####################################
//  Pixels Functions
//#####################################

#if PIXELS
  void colorWipe(uint32_t c) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
    }
  }
#endif

//#####################################
//  Setup
//#####################################

#if (!LOW_POWER)
  ISR(WDT_vect) { Sleepy::watchdogEvent(); }    
#endif

void setup () 
{
  Serial.begin(115200);
  Serial.println("Hey, Data");
  rf12_initialize(NODEID,RF12_433MHZ,GROUPID); // NodeID, Frequency, Group
  //rf12_encrypt(KEY);
  
 // Serial.println(rf12_config());
  
  debugMessage("Start setup");
  
  RfIn.reserve(50);
  toRf.reserve(50);
  
  #if POWER
    dataPower.reserve(50);
  #endif
  
  #if HUMID
  dataHumid.reserve(50);
  #endif
  
  #if TEMP
  dataTemp.reserve(50);
  #endif
  
  #if PRESSURE
  dataPressure.reserve(50);
  #endif
  
  #if SOUND
  dataSound.reserve(50);
  #endif
  
  #if LIGHT
  dataLight.reserve(50);
  #endif
  
  #if PIR
  dataPir.reserve(50);
  #endif
  
  #if SENSOR2
  dataSensor.reserve(50);
  #endif
  
  #if VOLTAGE
  dataVoltage.reserve(50);
  #endif
  
  #if MATRIX3208
    HT1632.begin(MATRIX3208CS1, MATRIX3208WR, MATRIX3208DATA);
    matrixLength = HT1632.getTextWidth(matrixString, FONT_5X4_END, FONT_5X4_HEIGHT);
  #endif
 
  debugMessage("Setup outputs");
  
  #if PIXELS
    //neopixels
    debugMessage("Setup pixels");
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
  #endif
  
  #if BASIC_HUMID
    //basic humid
    debugMessage("Setup basic humidity");
    dht.begin();
  #endif
  
  #if POWER
    debugMessage("Setup power");
    emon1.current(POWER_PIN, 111.1);
  #endif
  
  #if HUMID
    debugMessage("Setup humidity");
    htu.begin();
  #endif
  
  #if PRESSURE
    debugMessage("Setup pressure");
    baro.begin();
  #endif
  
  #if TEMP
    debugMessage("Setup temperature");
    tempsensor.begin();
  #endif
    
  #if PIR
    //pir
    debugMessage("Setup pir");
    pinMode(PIR_PIN, INPUT);
    digitalWrite(PIR_PIN, LOW);
  #endif
  
  #if VOLTAGE
    //device voltage pin
    debugMessage("Setup voltage");
    pinMode(VOLTAGE_PIN, INPUT);
  #endif
  
  #if SENSOR2
    debugMessage("Setup sensor2");
    pinMode(SENSOR2_PIN, INPUT);   
    digitalWrite(SENSOR2_PIN, HIGH);
  #endif
  
  #if LEDS
    debugMessage("Setup leds");
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
  #endif
  
  #if LCD128
    debugMessage("Setup lcd128");
    pinMode(BACKLIGHT_LED_RED, OUTPUT);
    pinMode(BACKLIGHT_LED_GREEN, OUTPUT);
    pinMode(BACKLIGHT_LED_BLUE, OUTPUT);
    
    digitalWrite(BACKLIGHT_LED_RED, HIGH);
    digitalWrite(BACKLIGHT_LED_GREEN, LOW);
    digitalWrite(BACKLIGHT_LED_BLUE, HIGH);
   
    u8g.setColorIndex(1);         // pixel on
    redrawLcd();
  #endif
  
  #if BUTTONS
    if (!cap.begin(0x5A)) {
      debugMessage("Error in button setup");
    }
  #endif
  
  #if SEGMENT14
    debugMessage("Setup 14 Segment");
    alpha4.begin(0x70);  // pass in the address
    alpha4.writeDigitAscii(0, 'H');
    alpha4.writeDigitAscii(1, 'E');
    alpha4.writeDigitAscii(2, 'Y');
    alpha4.writeDigitAscii(3, ' ');
    alpha4.writeDisplay();
  #endif
  
  #if SEGMENT7
    debugMessage("Setup 7 Segment");
    matrix.begin(0x70);
    matrix.print(1234);
    matrix.drawColon(true);
    matrix.writeDisplay();
  #endif
  
  #if MATRIX8X8
    debugMessage("Setup 8x8 Matrix");
    matrix.begin(0x70);
    matrix.clear();
    matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
    matrix.writeDisplay();
  #endif
  
  #if MATRIX8X8BI
    debugMessage("Setup 8x8 Bi-Color Matrix");
    matrix.begin(0x70);
    matrix.clear();
    matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_GREEN);
    matrix.writeDisplay();
  #endif
  
  
 
  debugMessage("Setup complete!");
  
}

//#####################################
//  DEBUG message
//#####################################

void debugMessage(String message){
  
  #if DEBUG
    Serial.println(message);
  #endif
  
}

//#####################################
//  Matrix Images
//#####################################

#if MATRIX8X8 || MATRIX8X8BI

  static const uint8_t PROGMEM
    smile_bmp[] =
    { B00111100,
      B01000010,
      B10100101,
      B10000001,
      B10100101,
      B10011001,
      B01000010,
      B00111100 },
    neutral_bmp[] =
    { B00111100,
      B01000010,
      B10100101,
      B10000001,
      B10111101,
      B10000001,
      B01000010,
      B00111100 },
    frown_bmp[] =
    { B00111100,
      B01000010,
      B10100101,
      B10000001,
      B10011001,
      B10100101,
      B01000010,
      B00111100 };
#endif


//#####################################
//  LCD128 draw
//#####################################

#if LCD128
void drawLcd128(void) {
  // graphic commands to redraw the complete screen should be placed here  
 
  if (lcd_page == "home"){
    
  String temp = "";
  char temp_char[12] = "";
  temp += lcd_temp;
  temp += (char)176;
  temp += "c ";
  temp.toCharArray(temp_char, temp.length());
  
  String humid = "";
  char humid_char[12] = "";
  humid += lcd_humid;
  humid += "% ";
  humid.toCharArray(humid_char, humid.length());
  
  String pressure = "";
  char pressure_char[12] = "";
  pressure += lcd_pressure;
  pressure += "b ";
  pressure.toCharArray(pressure_char, pressure.length());
  
  String power = "";
  char power_char[12] = "";
  power += lcd_power;
  power += "w ";
  power.toCharArray(power_char, power.length());
  
  String light = "";
  char light_char[12] = "";
  light += lcd_light;
  light.toCharArray(light_char, light.length());
  
  u8g.setFont(u8g_font_helvR14);
  u8g.drawStr( 0, 14, "HeyData!");
  
  u8g.drawLine( 0, 16, 128, 16);
   
  u8g.setFont(u8g_font_baby);
  u8g.drawStr( 0, 39, "Humidity");
  u8g.drawStr( 0, 62, "Temperature");
  u8g.drawStr( 62, 39, "Pressure");
  u8g.drawStr( 62, 62, "Light");
  
  u8g.setFont(u8g_font_helvR14);
  u8g.drawStr( 0, 33, humid_char);
  u8g.drawStr( 0, 56, temp_char);
  u8g.drawStr( 62, 33, pressure_char);
  u8g.drawStr( 62, 56, light_char);
  
  } else if (menu_selected >= 0){
    
    u8g.setFont(u8g_font_helvR14);
    u8g.drawStr( 0, 14, menu[menu_selected]);
    u8g.drawLine( 0, 16, 128, 16);
    
    if (menu_selected == 0){
      // menu 0
      
    } else if (menu_selected == 1){
      // menu 1
   
    } else if (menu_selected == 2){
      // menu 2
     
    } else if (menu_selected == 3){
      // menu 3
     
    } else if (menu_selected == 4){
      // menu 4
     
    } else if (menu_selected == 5){
      // menu 5 
      
    }
    
  } else if (lcd_page == "menu"){
    
    u8g.setFont(u8g_font_helvR14);
    u8g.drawStr( 0, 14, "Menu");
    u8g.drawLine( 0, 16, 128, 16);
    
    u8g.setFont(u8g_font_baby);
    
    for (int lcdloop = 0; lcdloop < 6; lcdloop++){
      u8g.drawStr( 3, 23 + (lcdloop*8), menu[lcdloop]); 
    }
    
    u8g.drawLine( 0, 16 + (menu_pos * 8), 55, 16 + (menu_pos * 8));
    u8g.drawLine( 0, 24 + (menu_pos * 8), 55, 24 + (menu_pos * 8));
    u8g.drawLine( 0, 16 + (menu_pos * 8), 0, 24 + (menu_pos * 8));
    u8g.drawLine( 55, 16 + (menu_pos * 8), 55, 24 + (menu_pos * 8));

  }
  
}
#endif

//#####################################
//  Check if there is incoming RF data
//#####################################

void checkRf(){
  
  //RF In -> Serial Out
  if (rf12_recvDone() && rf12_crc == 0) {
      // a packet has been received
      RfIn = "";
      
      Serial.print("");
      for (byte i = 0; i < rf12_len; ++i){
          Serial.print((char)rf12_data[i]);
          RfIn += (char)rf12_data[i];
      }
      Serial.println();
      
      RfInComplete = true;
  }
}

//#####################################
//  Generate Send String
//#####################################

String generateSendString(String type, String value){
  
  debugMessage("Generating String");
  
  String tempString;
  
  tempString = "data|";
  tempString += NODEID;
  tempString += "|0|";
  tempString += type;
  tempString += "|";
  tempString += value;
  tempString += "|";
  
  return tempString;
  
}


//#####################################
//  Process the incoming string
//#####################################

void processString(){
  
    String node_from = "";
    String node_to = "";
    String node_type = "";
    String node_value = "";
    String node_extra1 = "";
    String node_extra2 = "";
    String node_extra3 = "";
 
    int message_section = 0;
    char currentCharacter;
    char delimiter = '|';
    
    // if input starts with the string 'data' then we know it is for us
    if (stringToProcess.substring(0,5) == "data|") {
      for (int i = 5; i < stringToProcess.length(); i++){

        currentCharacter = stringToProcess.charAt(i);
        if (currentCharacter == delimiter){
          message_section++;
        } else {
          if (message_section == 0){
            node_from += currentCharacter;
          }
          else if (message_section == 1){
            node_to += currentCharacter;
          }
          else if (message_section == 2){
            node_type += currentCharacter;
          }
          else if (message_section == 3){
            node_value += currentCharacter;
          } 
          else if (message_section == 4){
            node_extra1 += currentCharacter;
          } 
          else if (message_section == 5){
            node_extra2 += currentCharacter;
          } 
          else if (message_section == 6){
            node_extra3 += currentCharacter;
          } 
        }
      }
    }
    
    #if PIXELS
      if ((node_to.toInt() == NODEID) && (node_type == "colour")){
        colorWipe(Wheel(node_value.toInt()));
      }
    #endif
    
    #if LCD128
      if ((node_to == "0") && (node_type == "temp")){
        lcd_temp = node_value;
        redrawLcd();
      }
      

      if ((node_to == "0") && (node_type == "humid")){
        lcd_humid = node_value;
        redrawLcd();
      }
      
      /*
      if ((node_to == "0") && (node_type == "power")){
        lcd_power = node_value;
        redrawLcd();
      }
      */
      
      if ((node_to == "0") && (node_type == "light")){
        lcd_light = node_value;
        redrawLcd();
      }

      if ((node_to == "0") && (node_type == "pressure")){
        lcd_pressure = node_value;
        redrawLcd();
      }

    #endif
    
    #if SEGMENT14
      if ((node_to.toInt() == NODEID) && (node_type == "string")){
        char valArray[8];
        node_value.toCharArray(valArray, 8);
        
        for (int i=0; i <= 4; i++){
          if (node_value.charAt(i) == ' '){
            valArray[i] = ' ';
          }
        }

        alpha4.writeDigitAscii(0, valArray[0]);
        alpha4.writeDigitAscii(1, valArray[1]);
        alpha4.writeDigitAscii(2, valArray[2]);
        alpha4.writeDigitAscii(3, valArray[3]);
        alpha4.writeDisplay();
      }
    #endif
    
    #if SEGMENT7
      if ((node_to.toInt() == NODEID) && (node_type == "string")){
        
        char floatbuf[32]; // make this at least big enough for the whole string
        node_value.toCharArray(floatbuf, sizeof(floatbuf));
        float f = atof(floatbuf);

        matrix.drawColon(false);
        matrix.print(f);
        matrix.writeDisplay();
      }
    #endif
    
    #if MATRIX3208
      if ((node_to.toInt() == NODEID) && (node_type == "string")){
        matrix3208NewData(node_value);
      }
    #endif
  
    stringToProcess = "";
    
  } 

//#####################################
//  Send our RF data
//#####################################
/*
void sendReading(String type, String value){
  
  toRf = "data|";
  toRf += NODEID;
  toRf += "|0|";
  toRf += type;
  toRf += "|";
  toRf += value;
  toRf += "|";
  
  toRfComplete = true;
  sendRf();
  
}
*/

void scheduleNextRfSend(){
  
  if (toRfComplete == false){
    if (dataVoltage.length() > 0){
      toRf = dataVoltage;
      dataVoltage = "";
      toRfComplete = true;
    } else if (dataHumid.length() > 0){
      toRf = dataHumid;
      dataHumid = "";
      toRfComplete = true;
    } else if (dataTemp.length() > 0){
      toRf = dataTemp;
      dataTemp = "";
      toRfComplete = true;
    } else if (dataPir.length() > 0){
      toRf = dataPir;
      dataPir = "";
      toRfComplete = true;
    } else if (dataPower.length() > 0){
      toRf = dataPower;
      dataPower = "";
      toRfComplete = true;
    } else if (dataPressure.length() > 0){
      toRf = dataPressure;
      dataPressure = "";
      toRfComplete = true;
    } else if (dataLight.length() > 0){
      toRf = dataLight;
      dataLight = "";
      toRfComplete = true;
    } else if (dataSound.length() > 0){
      toRf = dataSound;
      dataSound = "";
      toRfComplete = true;
    } else if (dataSensor.length() > 0){
      toRf = dataSensor;
      dataSensor = "";
      toRfComplete = true;
    }
  } 
    
}

//#####################################
//  Send our RF data
//#####################################

void sendPayload(String message){
   payload.print(message);
   rf12_sendStart(0, payload.buffer(), payload.length());
   payload.reset(); 
}
  
void sendRf(){
  
  //RF Out
  if (toRfComplete) {
    
    //this doesn't seem to be working, so taking it out
    //if (rf12_canSend()){
      
      debugMessage("Sending");
      
      sendPayload(toRf);
      
      //if this is the hub, send over rf AND serial  
      if (HUB){
        Serial.println(toRf);      
      }
      
      toRfComplete = false;
      
      //process this here too, as we might want to action the data locally
      stringToProcess = toRf;
      toRf = "";
      
      processString();
      
      
    //}
  } 
  
  /*
  if (toRfComplete && rf12_canSend()) {
      toRfComplete = false;
      
      payload.print(toRf);
      rf12_sendStart(0, payload.buffer(), payload.length());
      payload.reset();
      
      toRf = "";
  } 
  */
  
}

//#####################################
//  Process RF data
//#####################################

void processRf(){
 
  //we have received rf signal, do we need to process locally?
  //(it has already been sent out on the serial port)  
  if (RfInComplete){
    
    RfInComplete = false;
    //do some stuff maybe?
    
    stringToProcess = RfIn;
    RfIn = "";
    
    processString();
  
  }
  
}




//#####################################
//  Capture incoming serial data
//#####################################

//Serial In
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      toRfComplete = true;
    
      //Serial.println("complete");      
    } else {
      
      //asdd the character to the incoming string
      toRf += inChar;
      //Serial.print(".");
      
    }
  }
  
}

#if PIXELS

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


#endif

//#####################################
//  Sensors
//#####################################

#if BASIC_HUMID
  void checkBasicHumid(){
  
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    char temp_char[10];
    char humid_char[10];
    
    dtostrf(h, 5, 1, humid_char);
    dtostrf(t, 5, 1, temp_char);
    
    String temp = temp_char;
    String humid = humid_char;
    
    humid.trim();
    temp.trim();
    
    if (!(isnan(t))) {    
      dataTemp = generateSendString("temp", temp);
    }
    
    #if BASIC_HUMID_TEMP
      if (!(isnan(h))) {    
       dataHumid = generateSendString("humid", humid);
      }
    #endif
      
  }
#endif


#if HUMID
  void checkHumid(){
 
    debugMessage("Checking Humidity");
     
    float h = htu.readHumidity();
    float t = htu.readTemperature();
    
    char temp_char[10];
    char humid_char[10];
    
    dtostrf(h, 5, 1, humid_char);
    dtostrf(t, 5, 1, temp_char);
    
    String temp = temp_char;
    String humid = humid_char;
    
    humid.trim();
    temp.trim();
    
    #if HUMID_TEMP
    if (!(isnan(t))) {    
      dataTemp = generateSendString("temp", temp);
    }
    #endif
    
    if (!(isnan(h))) {    
     dataHumid = generateSendString("humid", humid);
    }
       
  }
#endif

#if PRESSURE
  void checkPressure(){
  
    debugMessage("Checking Pressure");
    
    float pascals = baro.getPressure();
    float bar = pascals/100000;
    
    float temp = baro.getTemperature();
    
    char pressure_char[10];
    char temp_char[10];
    
    dtostrf(bar, 5, 3, pressure_char);
    dtostrf(temp, 5, 1, temp_char);
    
    String pressure = pressure_char;
    String temperature = temp_char;
    
    pressure.trim();
    temperature.trim();
    
    dataPressure = generateSendString("pressure", pressure);
    
    #if PRESSURE_TEMP
       dataTemp = generateSendString("temp", temperature);
    #endif
      
  }
#endif

#if TEMP
  void checkTemp(){
  
    debugMessage("Checking Temperature");
    
    float temp = tempsensor.readTempC();
    char temp_char[10];
    
    dtostrf(temp, 5, 2, temp_char);
    String temperature = temp_char;
    
    temperature.trim();
    dataTemp = generateSendString("temp", temperature);
      
  }
#endif

#if LIGHT
  void checkLight(){
   
   debugMessage("Checking Light");
   
   int li = analogRead(LIGHT_PIN);
   char light_char[10];
   dtostrf(li, 5, 0, light_char);
   String light = light_char;
   
   light.trim();
   
   dataLight = generateSendString("light", light);
    
  }
#endif

//#####################################
//  Process Sensors
//#####################################

void checkSensorTiming(){
  
  runContinuousSensors();
  
  int minuteNow = minute();
  
  if (minuteNow >= lastMinute + PERIOD){  
    lastMinute = minuteNow;
    runSensors();    
  } else if (minuteNow == 0){
    lastMinute = 1;
  }
 
}

void runSensors(){
  
  debugMessage("Running Sensors");
  
  if (BASIC_HUMID){
    checkBasicHumid();
  }
  
  if (HUMID){
    checkHumid();
  }
  
  if (PRESSURE){
    checkPressure();
  }
  
  if (TEMP){
    checkTemp();
  }
  
  if (LIGHT){
    checkLight();
  }
  
  if (SOUND){
    int soundLevel = soundMax - soundMin;
    char sound[4];
    dtostrf(soundLevel, 4, 0, sound);
    
    String sound_st = sound;
    sound_st.trim();
    
    dataSound = generateSendString("sound", sound_st);
    
    //cear the results and start again
    soundMax = 0;
    soundMin = 9999;
  }
  
  if (PIR){
    if (pir_movement == true){
      dataPir = generateSendString("pir", "1");
    } else {
      dataPir = generateSendString("pir", "0");
    }
    pir_movement = false;
  }
  
  if (POWER){
    char power_char[10]; 
    dtostrf(power_max, 5, 0, power_char);
    String power = power_char;    
    power.trim();
    if (power != "5729"){
      dataPower = generateSendString("power", power);
    }
    power_max = 0; 
  }
  
  if (SENSOR2){
    if (sensor2_triggered == true){
      dataSensor = generateSendString("sensor2", "1");
    } else {
      dataSensor = generateSendString("sensor2", "0");
    }
    sensor2_triggered = false;
  }
  
  if (VOLTAGE){
    
    int voltage_adc = analogRead(VOLTAGE_PIN);
    //adc level * arduino voltage * 2 (as we are in potential divider)
    float voltage_float = (voltage_adc/1024.0) * 3.3 * 2;
    
    char voltage_char[10]; 
    dtostrf(voltage_float, 5, 3, voltage_char);
    String voltage = voltage_char;   
    
    dataVoltage = generateSendString("voltage", voltage); 
  }
  
}


//#####################################
//  Matrix 3208 - new string
//#####################################

#if MATRIX3208
void matrix3208NewData(String newString){
 
  newString.trim();
  newString.toCharArray(matrixString, newString.length() + 1);

  Serial.println(matrixString);
  
  //matrixString = newString;
  matrixLength = HT1632.getTextWidth(matrixString, FONT_5X4_END, FONT_5X4_HEIGHT);
  matrixLoop = 0;
  
  HT1632.clear();
  HT1632.render();
}

//#####################################
//  Matrix 3208 - draw the text!
//#####################################

void drawMatrix3208(){
 
 long millisNow = millis();
 
 //make sure that when we roll over to 0 we reset everything
 if (millisNow < 2000){
   millisCheck = 0;
   return;
 }
 
 if (millisNow < (millisCheck + MATRIX3208SPEED)){
   return;
 }
 
 millisCheck = millisNow;
 
 if (matrixLength < 33){
    
    HT1632.clear();
    HT1632.drawText(matrixString, 0, 2, FONT_5X4, FONT_5X4_END, FONT_5X4_HEIGHT);
    HT1632.render();
    
  } else {
  
    HT1632.clear();
    HT1632.drawText(matrixString, OUT_SIZE - matrixLoop, 2, FONT_5X4, FONT_5X4_END, FONT_5X4_HEIGHT);
    HT1632.render();
    
    matrixLoop = (matrixLoop+1)%(matrixLength + OUT_SIZE);
    
  } 
}
#endif



void runContinuousSensors(){
  
  if (PIR){
    if (digitalRead(PIR_PIN) == HIGH){
        pir_movement = true;
    }
  }
  
  //sensor pin has internal pullup
  if (SENSOR2){
    if (digitalRead(SENSOR2_PIN) == LOW){
        sensor2_triggered = true;
    }
  }
  
  #if POWER
    float Irms = emon1.calcIrms(1480);
    float pwr = Irms * 110.0;
    power_max = max(power_max, pwr);
  #endif
  
  if (SOUND){
    soundMax = max(soundMax, analogRead(SOUND_PIN));
    soundMin = min(soundMin, analogRead(SOUND_PIN));
  }
 
  
}

//#####################################
//  Redraw the LCD
//#####################################

void redrawLcd(){
  #if LCD128
    debugMessage("Redrawing 128 LCD");
    u8g.firstPage();  
    do {
      drawLcd128();
    } while( u8g.nextPage() );
  #endif
}

//#####################################
//  Run all the code needed every second
//#####################################

void checkSecond(){
 
   //every second!
  int secondNow = second();
  if (secondNow >= lastSecond + 1){  
    lastSecond = secondNow;
    
    scheduleNextRfSend();
    
  } else if (secondNow == 0){
    lastSecond = 1;
  } 
  
  if (SENSORS){
    
    #if LEDS
    ledLoop++;
    #endif
  }
  
}

#if BUTTONS
void checkButtons(){
  
  buttoncurrent = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((buttoncurrent & _BV(i)) && !(buttonlast & _BV(i)) ) {
      buttons[i] = 1;
      #if DEBUG
        Serial.print(i); Serial.println(" touched");
      #endif
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(buttoncurrent & _BV(i)) && (buttonlast & _BV(i)) ) {
      #if DEBUG
        Serial.print(i); Serial.println(" released");
      #endif
    }
  }

  // reset our state
  buttonlast = buttoncurrent;
  
}

boolean checkButton(int button_id){
  if (buttons[button_id] == 1){
     buttons[button_id] = 0;       
     return true;
  } else {
    return false;
  }
}

#endif


//#####################################
//  Main Loop
//#####################################

void loop () 
{
  
  /*
  //delay(10);
  if (ledLoop < 255000){
    analogWrite(LED1, ledLoop/200);
    ledLoop++;
  } else if (ledLoop < 510000){
    analogWrite(LED1, (255000 - (ledLoop - 255000))/200);
    ledLoop++;
  } else {
    ledLoop = 0; 
  }
  */
  
  //if we are in low power mode, only check the sensors once every wakeup period
  #if LOW_POWER
    
    if (SENSORS){
      runContinuousSensors();
      runSensors();
    }
    
    scheduleNextRfSend();
    sendRf();
    
    /*
    delay(50);
    scheduleNextRfSend();
    sendRf();
    delay(50);
    scheduleNextRfSend();
    sendRf();
    delay(50);
    scheduleNextRfSend();
    sendRf();
    delay(50);
    scheduleNextRfSend();
    sendRf();
    */
    
    //sleep for 64 seconds * the sensor period (it's near enough a minute)
    for (int i = 0; i < (8 * PERIOD); i++) { 
       LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
    }
    
  //else run the sensors normally  
  #else
  
    if (SENSORS){
      checkSensorTiming();
    }
    
    checkSecond();
    
  #endif
  
  checkRf();
  processRf();
  sendRf();
  
  #if BUTTONS
  
    if (checkButton(btn_up)){
        Serial.println("up");
        menu_pos--;
        if (menu_pos < 0){
          menu_pos = 0; 
        }
        redrawLcd();
    }
    
    if (checkButton(btn_down)){
        Serial.println("down");
        menu_pos++;
        if (menu_pos > 5){
          menu_pos = 5; 
        }
        redrawLcd();
    }
    
    if (checkButton(btn_left)){
        //Serial.println("left");
        menu_selected = -1;
        redrawLcd();
    }
    
    if (checkButton(btn_right)){
        //Serial.println("right");
        if (menu_selected == -1){
           menu_selected = menu_pos;
        }
        redrawLcd();
    }
    
    if (checkButton(btn_menu)){
        lcd_page = "menu";
        menu_selected = -1;
        menu_pos = 0; 
        redrawLcd();
    }
  
    if (checkButton(btn_home)){
        lcd_page = "home";
        redrawLcd();
    }
    
    
    
    checkButtons();
  #endif
  
  #if MATRIX3208
    drawMatrix3208();
  #endif

}


