//#####################################
//  Turn on features and define pins
//#####################################

//Our node id
#define NODEID 7
#define GROUPID 28

//128 x 64 LCD screen
#define LCD128 false

//All sensors on or off
#define SENSORS false

//Led pixels enabled?
#define PIXELS true
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

#define VOLTAGE true
#define VOLTAGE_PIN A0

//building power
#define POWER false
#define POWER_PIN A1

#define LEDS false
#define LED1 3
#define LED2 5
#define LED3 6
#define LED4 9

#define HUB false
 
//Output serial debug strings
#define DEBUG false

#define MATRIX3208 false
#define MATRIX3208CS1 4
#define MATRIX3208WR 5
#define MATRIX3208DATA 6
#define MATRIX3208SPEED 100

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
  #define BACKLIGHT_LED_RED 17
  #define BACKLIGHT_LED_GREEN 4
  #define BACKLIGHT_LED_BLUE 3

  U8GLIB_LM6059 u8g(6, 5, 9, 7, 8);

  String lcd_temp = "0";
  String lcd_humid = "0";
  String lcd_pressure = "0";
  String lcd_power = "0";
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
    HT1632.begin(MATRIX3208DATA, MATRIX3208WR, MATRIX3208CS1);
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
  
  debugMessage("Setup complete!");
  
}

//#####################################
//  LCD128 draw
//#####################################

void debugMessage(String message){
  
  #if DEBUG
    Serial.println(message);
  #endif
  
}

//#####################################
//  LCD128 draw
//#####################################

#if LCD128
void drawLcd128(void) {
  // graphic commands to redraw the complete screen should be placed here  
  
  String temp = "";
  char temp_char[12] = "";
  temp += lcd_temp;
  temp += (char)176;
  temp += "c ";
  temp.toCharArray(temp_char, temp.length());
  
  String humid = "";
  char humid_char[10] = "";
  humid += lcd_humid;
  humid += "% ";
  humid.toCharArray(humid_char, humid.length());
  
  String pressure = "";
  char pressure_char[8] = "";
  pressure += lcd_pressure;
  pressure += "mb ";
  pressure.toCharArray(pressure_char, pressure.length());
  
  String power = "";
  char power_char[8] = "";
  power += lcd_power;
  power += "w ";
  power.toCharArray(power_char, power.length());
  
  u8g.setFont(u8g_font_helvR14);
  u8g.drawStr( 0, 14, "Living Room");
  
  u8g.drawLine( 0, 16, 128, 16);
   
  u8g.setFont(u8g_font_baby);
  u8g.drawStr( 0, 39, "Power");
  u8g.drawStr( 0, 62, "Temperature");
  u8g.drawStr( 62, 39, "Pressure");
  u8g.drawStr( 62, 62, "Humidity");
  
  u8g.setFont(u8g_font_helvR14);
  u8g.drawStr( 0, 33, power_char);
  u8g.drawStr( 0, 56, temp_char);
  u8g.drawStr( 62, 33, pressure_char);
  u8g.drawStr( 62, 56, humid_char);
  
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
      
      toRf = "";
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
    if (RfIn.substring(0,5) == "data|") {
      for (int i = 5; i < RfIn.length(); i++){

        currentCharacter = RfIn.charAt(i);
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
      
      if ((node_to == "0") && (node_type == "power")){
        lcd_power = node_value;
        redrawLcd();
      }
      
      if ((node_to == "0") && (node_type == "pres")){
        lcd_pressure = node_value;
        redrawLcd();
      }
    #endif
    
    #if MATRIX3208
      if ((node_to.toInt() == NODEID) && (node_type == "string")){
        matrix3208NewData(node_value);
      }
    #endif
  
    RfIn = "";
    
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
  
  #if MATRIX3208
    drawMatrix3208();
  #endif

}


