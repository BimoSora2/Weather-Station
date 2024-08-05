// wire i2c
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// bme280
#include <SPI.h>
#include <Adafruit_BMP280.h>

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);
// end

// bh1750
#include <BH1750.h>

BH1750 lightMeter;
//end

// tm1637
#include <TM1637Display.h>
#define CLK PB15
#define DIO PA8
TM1637Display display(CLK, DIO);
// end

const int ledPin =  PC15;
const int ledSTM32 =  LED_BUILTIN;
int ledState = HIGH;
int state ;

const int sensorMin = 0;     // sensor minimum
const int sensorMax = 4096;  // sensor maximum

int analogInput = PB0;
float vout = 0.0;
float vin = 0.0;
float R1 = 22000.0;
float R2 = 3000.0;
int value = 0;
int i = 0;

int temp, pressure, sensorReading, range;
float lux;

unsigned long previousMillis = 0;
unsigned long interval = 500;

unsigned long previousMillis1 = 0;
const long interval1 = 8000;

unsigned long previousMillis2 = 0;
const long interval2 = 500;

unsigned long sensorMilis_1 = 0;
const long sensorinterval_1 = 500;

unsigned long sensorMilis_2 = 0;
const long sensorinterval_2 = 500;

const uint8_t maxvolt[] = {
  SEG_G,
  SEG_G,
  SEG_G,
  SEG_G
};

void setup() {
  analogReadResolution(12);
  Serial.begin(9600);
  
  Wire.begin();
  lightMeter.begin();
  
  lcd.init();
  lcd.backlight();

  display.setBrightness(5);
  
  // Clear the display
  display.clear();

  pinMode(analogInput, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ledSTM32, OUTPUT);
  digitalWrite(ledPin, HIGH);
  digitalWrite(ledSTM32, HIGH);
  
  if (!bmp.begin(0x76)) {
    Serial.println("Tidak ada sensor BME280, cek rangkaianmu!");
    while (1);
  }
  /*
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,         // Operating Mode
                  Adafruit_BMP280::SAMPLING_X1,         // Temp. oversampling
                  Adafruit_BMP280::SAMPLING_X1,         // Pressure oversampling
                  Adafruit_BMP280::FILTER_OFF);         // Filtering
                  // Adafruit_BMP280::STANDBY_MS_4000); // Standby time
  */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  lcd.clear();
  for (i = 16; i >= 0; i--) //program increamental nilai i dari 0 sampai 15
  {
    lcd.setCursor(i, 0); //setting posisi kursor kolom sesuai nilai i
    lcd.print("Weather Station"); //menampilkan karakter
    delay(500); //menunda tampilan selama 500 ms
    lcd.clear(); //menghapus semua karakter
  }
}


void loop() {
  bmp.takeForcedMeasurement();

  temp = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0;
  sensorReading = analogRead(PA1);
  range = map(sensorReading, sensorMin, sensorMax, 0, 3);
  lux = lightMeter.readLightLevel();

  if(millis() - previousMillis >= interval) {
    previousMillis = millis();

    // Create the JSON document
    const size_t capacity = JSON_OBJECT_SIZE(128);
    DynamicJsonDocument doc(capacity);

    doc["temperature"] = temp;
    doc["pressure"] = pressure;
    doc["range"] = range;
    doc["lux"] = lux;
  
    //Send data to NodeMCU
    serializeJson(doc, Serial);
  }

  if(millis() - previousMillis1 >= interval1) {
    previousMillis1 = millis();
    state++;
  }
  
  if(state==4){
    state=1;
  }
  
  switch (state){
    case 1:
      if(millis() - sensorMilis_1 >= sensorinterval_1) {
        sensorMilis_1 = millis();
        if(temp < 20){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("SUHU");
          lcd.setCursor(0,1);
          lcd.print(temp);
          lcd.print(" *C!");
        }else if(temp > 40){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("SUHU");
          lcd.setCursor(0,1);
          lcd.print(temp);
          lcd.print(" *C!");
        }else{
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("SUHU");
          lcd.setCursor(0,1);
          lcd.print(temp);
          lcd.print(" *C");
        }
      }
    break;
    case 2:
      if(millis() - sensorMilis_1 >= sensorinterval_1) {
        sensorMilis_1 = millis();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("TEKANAN UDARA");
        lcd.setCursor(0,1);
        lcd.print(pressure);
        lcd.print(" hPa");
      }
    break;
    /*
    case 3:
        if(millis() - sensorMilis_1 >= sensorinterval_1) {
          sensorMilis_1 = millis();
          int ketinggian = bmp.readAltitude(1013.25);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("KETINGGIAN");
          lcd.setCursor(0,1);
          lcd.print(ketinggian);
          lcd.print(" m");
        }
    break;
    */
    case 3:
      if(millis() - sensorMilis_2 >= sensorinterval_2) {
        sensorMilis_2 = millis();
          switch (range){
            case 0:
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("CURAH HUJAN");
              lcd.setCursor(0,1);
              lcd.print("HUJAN");
            break;
            case 1:
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("CURAH HUJAN");
              lcd.setCursor(0,1);
              lcd.print("PERINGATAN HUJAN");
            break;
            case 2:
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("CURAH HUJAN");
              lcd.setCursor(0,1);
              lcd.print("TIDAK HUJAN");
            break;
          }
      }
    break;
  }
  if(lux < 100){
    if (millis() - previousMillis2 >= interval2) {
      // save the last time you blinked the LED
      previousMillis2 = millis();
    
        // if the LED is off turn it on and vice-versa:
        if (ledState == HIGH) {
          ledState = LOW;
        } else {
          ledState = HIGH;
        }
    
        // set the LED with the ledState of the variable:
        digitalWrite(ledSTM32, ledState);
    }
    digitalWrite(ledPin, HIGH);
  }else if(lux > 100){
    digitalWrite(ledSTM32, HIGH);
    digitalWrite(ledPin, LOW);
  }
}
