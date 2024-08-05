#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <Fuzzy.h>

// Library include
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_BMP280 bmp;
BH1750 lightMeter(0x23);

const int ledPin = PC15;
const int ledSTM32 = LED_BUILTIN;

// Analog range
const int sensorMin = 0;
const int sensorMax = 4096;

int ledState = HIGH;
int i = 0;

int state = 0;

// Untuk membaca value
float lux = -1;
int temp = -1, pressure = -1, height = -1, sensorReading = -1, range = -1;

// Implementasi supaya awalnya status false untuk mendeteksi apakah sensor putus atau tidak
bool bmpStatus = false, bh1750Status = false, rainSensorStatus = false;

unsigned long delayTime;

// Fuzzy objects
Fuzzy * fuzzy = new Fuzzy();

// Temperature fuzzy sets
FuzzySet * veryCold = new FuzzySet(-10, 0, 5, 15);
FuzzySet * cold = new FuzzySet(5, 15, 20, 25);
FuzzySet * normal = new FuzzySet(20, 25, 30, 35);
FuzzySet * hot = new FuzzySet(30, 35, 40, 45);
FuzzySet * veryHot = new FuzzySet(40, 45, 50, 60);

// Height fuzzy sets
FuzzySet * veryLow = new FuzzySet(0, 0, 50, 100);
FuzzySet * low = new FuzzySet(50, 100, 150, 200);
FuzzySet * mediumHeight = new FuzzySet(150, 200, 250, 300);
FuzzySet * high = new FuzzySet(250, 300, 350, 400);
FuzzySet * veryHigh = new FuzzySet(350, 400, 450, 500);

// Pressure fuzzy sets
FuzzySet * veryLowPressure = new FuzzySet(950, 960, 970, 980);
FuzzySet * lowPressure = new FuzzySet(970, 980, 990, 1000);
FuzzySet * normalPressure = new FuzzySet(990, 1000, 1010, 1020);
FuzzySet * highPressure = new FuzzySet(1010, 1020, 1030, 1040);
FuzzySet * veryHighPressure = new FuzzySet(1030, 1040, 1050, 1060);

// Correction fuzzy sets
FuzzySet * smallCorrection = new FuzzySet(-5, -3, -3, -1);
FuzzySet * mediumCorrection = new FuzzySet(-3, -1, -1, 1);
FuzzySet * largeCorrection = new FuzzySet(-1, 1, 3, 5);

// Light fuzzy sets
FuzzySet * veryDark = new FuzzySet(0, 0, 25, 50);
FuzzySet * dark = new FuzzySet(25, 50, 75, 100);
FuzzySet * dim = new FuzzySet(75, 100, 500, 1000);
FuzzySet * bright = new FuzzySet(500, 1000, 5000, 10000);
FuzzySet * veryBright = new FuzzySet(5000, 10000, 50000, 65535);

// Timing variables
unsigned long previousMillis = 0;
const unsigned long interval = 500;

unsigned long previousMillis2 = 0;
const unsigned long interval2 = 500;

unsigned long previousMillis3 = 0;
const unsigned long interval3 = 1000;

const unsigned long sensorReadInterval = 1000; // Read sensors every 1 second

unsigned long sensorMilis_1 = 0;
const unsigned long sensorinterval_1 = 1000;
 
unsigned long sensorMilis_2 = 0;
const unsigned long sensorinterval_2 = 1000;

unsigned long previousMillisState = 0;
const unsigned long lcdUpdateInterval = 8000;  // Update LCD every 8 seconds

unsigned long previousMillisJson = 0;
const unsigned long jsonSendInterval = 1000;  // Send JSON every 1 second

// New timing variables for sensor management
unsigned long sensorStartTime = 0;
const unsigned long bmpDuration = 8000;  // 8 seconds
const unsigned long bh1750Duration = 1000;  // 1 second
const unsigned long rainfallDuration = 1000;  // 1 second

// Declare FuzzyRuleConsequent variables globally
FuzzyRuleConsequent * thenLargeCorrection = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenMediumCorrection = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenSmallCorrection = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenLargeCorrectionHeight = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenMediumCorrectionHeight = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenSmallCorrectionHeight = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenLargeCorrectionPressure = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenMediumCorrectionPressure = new FuzzyRuleConsequent();
FuzzyRuleConsequent * thenSmallCorrectionPressure = new FuzzyRuleConsequent();

// Setup dimulai
void setup() {
  // Resolusi STM32 32Bit
  analogReadResolution(12);

  Serial.begin(9600);
  Wire.begin();

  // Instalasi pin LED
  pinMode(ledPin, OUTPUT);
  pinMode(ledSTM32, OUTPUT);
  digitalWrite(ledSTM32, LOW);
  digitalWrite(ledPin, HIGH);
  
  // LCD Instalasi
  lcd.init();
  lcd.backlight();

  // Membaca address pada sensor
  bh1750Status = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  bmpStatus = bmp.begin(0x76);

  // Membaca apakah BMP280 gangguan atau tidak
  if (!bmpStatus) {
    Serial.println("Error initializing BMP280");
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED, Adafruit_BMP280::SAMPLING_X1, Adafruit_BMP280::SAMPLING_X1, Adafruit_BMP280::FILTER_OFF);
    delayTime = 60000; // in milliseconds
  }

  // Membaca apakah BH1750 gangguan atau tidak
  if (!bh1750Status) {
    Serial.println("Error initializing BH1750");
  }

  // Tulisan mengetik satu persatu
  lcd.clear();
  for (i = 16; i >= 0; i--) { // Program increamental nilai i dari 16 sampai 0
    lcd.setCursor(i, 0); // Setting posisi kursor kolom sesuai nilai i
    lcd.print("WEATHER STATION"); // Menampilkan karakter
    delay(500); // Menunda tampilan selama 500 ms
    lcd.clear();
  }
  
  // Fuzzy setup for temperature
  FuzzyInput * temperature = new FuzzyInput(1);
  temperature -> addFuzzySet(veryCold);
  temperature -> addFuzzySet(cold);
  temperature -> addFuzzySet(normal);
  temperature -> addFuzzySet(hot);
  temperature -> addFuzzySet(veryHot);
  fuzzy -> addFuzzyInput(temperature);

  // Fuzzy setup for height
  FuzzyInput * heightInput = new FuzzyInput(2);
  heightInput -> addFuzzySet(veryLow);
  heightInput -> addFuzzySet(low);
  heightInput -> addFuzzySet(mediumHeight);
  heightInput -> addFuzzySet(high);
  heightInput -> addFuzzySet(veryHigh);
  fuzzy -> addFuzzyInput(heightInput);

  // Fuzzy setup for pressure
  FuzzyInput * pressureInput = new FuzzyInput(3);
  pressureInput -> addFuzzySet(veryLowPressure);
  pressureInput -> addFuzzySet(lowPressure);
  pressureInput -> addFuzzySet(normalPressure);
  pressureInput -> addFuzzySet(highPressure);
  pressureInput -> addFuzzySet(veryHighPressure);
  fuzzy -> addFuzzyInput(pressureInput);

  // Fuzzy setup for light (lux)
  FuzzyInput * light = new FuzzyInput(4);
  light -> addFuzzySet(veryDark);
  light -> addFuzzySet(dark);
  light -> addFuzzySet(dim);
  light -> addFuzzySet(bright);
  light -> addFuzzySet(veryBright);
  fuzzy -> addFuzzyInput(light);

  // Fuzzy setup for correction
  FuzzyOutput * correction = new FuzzyOutput(1);
  correction -> addFuzzySet(smallCorrection);
  correction -> addFuzzySet(mediumCorrection);
  correction -> addFuzzySet(largeCorrection);
  fuzzy -> addFuzzyOutput(correction);

  // Fuzzy rules for temperature
  FuzzyRuleAntecedent * ifVeryCold = new FuzzyRuleAntecedent();
  ifVeryCold -> joinSingle(veryCold);
  thenLargeCorrection -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule1 = new FuzzyRule(1, ifVeryCold, thenLargeCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule1);

  FuzzyRuleAntecedent * ifCold = new FuzzyRuleAntecedent();
  ifCold -> joinSingle(cold);
  thenMediumCorrection -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule2 = new FuzzyRule(2, ifCold, thenMediumCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule2);

  FuzzyRuleAntecedent * ifNormal = new FuzzyRuleAntecedent();
  ifNormal -> joinSingle(normal);
  thenSmallCorrection -> addOutput(smallCorrection);
  FuzzyRule * fuzzyRule3 = new FuzzyRule(3, ifNormal, thenSmallCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule3);

  FuzzyRuleAntecedent * ifHot = new FuzzyRuleAntecedent();
  ifHot -> joinSingle(hot);
  thenMediumCorrection -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule4 = new FuzzyRule(4, ifHot, thenMediumCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule4);

  FuzzyRuleAntecedent * ifVeryHot = new FuzzyRuleAntecedent();
  ifVeryHot -> joinSingle(veryHot);
  thenLargeCorrection -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule5 = new FuzzyRule(5, ifVeryHot, thenLargeCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule5);

  // Fuzzy rules for height
  FuzzyRuleAntecedent * ifVeryLow = new FuzzyRuleAntecedent();
  ifVeryLow -> joinSingle(veryLow);
  thenLargeCorrectionHeight -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule6 = new FuzzyRule(6, ifVeryLow, thenLargeCorrectionHeight);
  fuzzy -> addFuzzyRule(fuzzyRule6);

  FuzzyRuleAntecedent * ifLow = new FuzzyRuleAntecedent();
  ifLow -> joinSingle(low);
  thenMediumCorrectionHeight -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule7 = new FuzzyRule(7, ifLow, thenMediumCorrectionHeight);
  fuzzy -> addFuzzyRule(fuzzyRule7);

  FuzzyRuleAntecedent * ifMedium = new FuzzyRuleAntecedent();
  ifMedium -> joinSingle(mediumHeight);
  thenSmallCorrectionHeight -> addOutput(smallCorrection);
  FuzzyRule * fuzzyRule8 = new FuzzyRule(8, ifMedium, thenSmallCorrectionHeight);
  fuzzy -> addFuzzyRule(fuzzyRule8);

  FuzzyRuleAntecedent * ifHigh = new FuzzyRuleAntecedent();
  ifHigh -> joinSingle(high);
  thenMediumCorrectionHeight -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule9 = new FuzzyRule(9, ifHigh, thenMediumCorrectionHeight);
  fuzzy -> addFuzzyRule(fuzzyRule9);

  FuzzyRuleAntecedent * ifVeryHigh = new FuzzyRuleAntecedent();
  ifVeryHigh -> joinSingle(veryHigh);
  thenLargeCorrectionHeight -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule10 = new FuzzyRule(10, ifVeryHigh, thenLargeCorrectionHeight);
  fuzzy -> addFuzzyRule(fuzzyRule10);

  // Fuzzy rules for pressure
  FuzzyRuleAntecedent * ifVeryLowPressure = new FuzzyRuleAntecedent();
  ifVeryLowPressure -> joinSingle(veryLowPressure);
  thenLargeCorrectionPressure -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule11 = new FuzzyRule(11, ifVeryLowPressure, thenLargeCorrectionPressure);
  fuzzy -> addFuzzyRule(fuzzyRule11);

  FuzzyRuleAntecedent * ifLowPressure = new FuzzyRuleAntecedent();
  ifLowPressure -> joinSingle(lowPressure);
  thenMediumCorrectionPressure -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule12 = new FuzzyRule(12, ifLowPressure, thenMediumCorrectionPressure);
  fuzzy -> addFuzzyRule(fuzzyRule12);

  FuzzyRuleAntecedent * ifNormalPressure = new FuzzyRuleAntecedent();
  ifNormalPressure -> joinSingle(normalPressure);
  thenSmallCorrectionPressure -> addOutput(smallCorrection);
  FuzzyRule * fuzzyRule13 = new FuzzyRule(13, ifNormalPressure, thenSmallCorrectionPressure);
  fuzzy -> addFuzzyRule(fuzzyRule13);

  FuzzyRuleAntecedent * ifHighPressure = new FuzzyRuleAntecedent();
  ifHighPressure -> joinSingle(highPressure);
  thenMediumCorrectionPressure -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule14 = new FuzzyRule(14, ifHighPressure, thenMediumCorrectionPressure);
  fuzzy -> addFuzzyRule(fuzzyRule14);

  FuzzyRuleAntecedent * ifVeryHighPressure = new FuzzyRuleAntecedent();
  ifVeryHighPressure -> joinSingle(veryHighPressure);
  thenLargeCorrectionPressure -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule15 = new FuzzyRule(15, ifVeryHighPressure, thenLargeCorrectionPressure);
  fuzzy -> addFuzzyRule(fuzzyRule15);

  // Fuzzy rules for light
  FuzzyRuleAntecedent * ifVeryDark = new FuzzyRuleAntecedent();
  ifVeryDark -> joinSingle(veryDark);
  thenLargeCorrection -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule16 = new FuzzyRule(16, ifVeryDark, thenLargeCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule16);

  FuzzyRuleAntecedent * ifDark = new FuzzyRuleAntecedent();
  ifDark -> joinSingle(dark);
  thenMediumCorrection -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule17 = new FuzzyRule(17, ifDark, thenMediumCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule17);

  FuzzyRuleAntecedent * ifDim = new FuzzyRuleAntecedent();
  ifDim -> joinSingle(dim);
  thenSmallCorrection -> addOutput(smallCorrection);
  FuzzyRule * fuzzyRule18 = new FuzzyRule(18, ifDim, thenSmallCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule18);

  FuzzyRuleAntecedent * ifBright = new FuzzyRuleAntecedent();
  ifBright -> joinSingle(bright);
  thenMediumCorrection -> addOutput(mediumCorrection);
  FuzzyRule * fuzzyRule19 = new FuzzyRule(19, ifBright, thenMediumCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule19);

  FuzzyRuleAntecedent * ifVeryBright = new FuzzyRuleAntecedent();
  ifVeryBright -> joinSingle(veryBright);
  thenLargeCorrection -> addOutput(largeCorrection);
  FuzzyRule * fuzzyRule20 = new FuzzyRule(20, ifVeryBright, thenLargeCorrection);
  fuzzy -> addFuzzyRule(fuzzyRule20);

  lcd.clear(); // Menghapus semua karakter
}

// Looping dimulai
void loop() {
  unsigned long currentMillis = millis();

  // Sensor reading
  if (currentMillis - previousMillis >= sensorReadInterval) {
    previousMillis = currentMillis;
    readSensors();
    processFuzzyLogic();
  }

  // Send JSON data every second
  if (currentMillis - previousMillisJson >= jsonSendInterval) {
    previousMillisJson = currentMillis;
    sendJsonData();
  }

  // Update LCD display
  if (currentMillis - previousMillisState >= lcdUpdateInterval) {
    previousMillisState = currentMillis;
    state = (state % 5) + 1;
    updateLCD(currentMillis);
  }

  // Handle sensor intervals separately
  handleSensorIntervals(currentMillis);
  
  // Update LED states
  updateLEDs(currentMillis);
}

void readSensors() {
  readBMP280();
  readBH1750();
  readRainfall();
}

void processFuzzyLogic() {
  processFuzzyLogicBMP();
  processFuzzyLogicBH1750();
}

void sendJsonData() {
  JsonDocument doc;

  doc["temperature"] = bmpStatus ? temp : -1;
  doc["pressure"] = bmpStatus ? pressure : -1;
  doc["height"] = bmpStatus ? height : -1;
  doc["lux"] = bh1750Status ? lux : -1;
  doc["range"] = rainSensorStatus ? range : -1;

  // Send data to NodeMCU
  serializeJson(doc, Serial);
  Serial.println();  // Add a newline for better parsing on the receiving end
}

void readBMP280() {
  if (bmpStatus) {
    temp = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0;
    height = bmp.readAltitude(1013.25);
 
    // Check for valid readings
    if (isnan(temp) || isnan(pressure) || isnan(height)) {
      bmpStatus = false;
    }
  }
}

void readBH1750() {
  if (bh1750Status && lightMeter.measurementReady()) {
    lux = lightMeter.readLightLevel();
    if (lux < 0) {
      bh1750Status = false;
    }
  }
}

void readRainfall() {
  sensorReading = analogRead(PA1);
  range = map(sensorReading, sensorMin, sensorMax, 0, 3);
  rainSensorStatus = (sensorReading != 0 && sensorReading != 4096);
}

void processFuzzyLogicBMP() {
  if (bmpStatus) {
    fuzzy->setInput(1, temp);
    fuzzy->setInput(2, height);
    fuzzy->setInput(3, pressure);
    fuzzy->fuzzify();
    float correction = fuzzy->defuzzify(1);
    temp += correction;
  }
}

void processFuzzyLogicBH1750() {
  if (bh1750Status) {
    fuzzy->setInput(4, lux);
    fuzzy->fuzzify();
    float lightCorrection = fuzzy->defuzzify(1);
    lux += lightCorrection;
  }
}

void updateLEDs(unsigned long currentMillis) {
  if (bh1750Status) {
    if (lux >= 0 && lux <= 100) {
      blinkLED(ledSTM32, currentMillis, interval2);
      digitalWrite(ledPin, HIGH);
    } else if (lux > 100 && lux <= 1000) {
      blinkLED(ledSTM32, currentMillis, interval3);
      digitalWrite(ledPin, HIGH);
    } else if (lux > 1000) {
      digitalWrite(ledSTM32, HIGH);
      digitalWrite(ledPin, LOW);
    }
  } else {
    digitalWrite(ledSTM32, LOW);
    digitalWrite(ledPin, HIGH);
  }
}

void blinkLED(int pin, unsigned long currentMillis, long interval) {
  static unsigned long previousBlink = 0;
  if (currentMillis - previousBlink >= interval) {
    previousBlink = currentMillis;
    digitalWrite(pin, !digitalRead(pin));
  }
}

void updateLCD(unsigned long currentMillis) {
  switch (state) {
  case 1:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TEMPERATURE");
    lcd.setCursor(0, 1);
    lcd.print(temp != -1 ? String(temp) + " *C" : "ERROR");
    break;
  case 2:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("AIR PRESSURE");
    lcd.setCursor(0, 1);
    lcd.print(pressure != -1 ? String(pressure) + " hPa" : "ERROR");
    break;
  case 3:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HEIGHT");
    lcd.setCursor(0, 1);
    lcd.print(height != -1 ? String(height) + " m" : "ERROR");
    break;
  case 4:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RAINFALL");
    lcd.setCursor(0, 1);
    if (rainSensorStatus) {
      switch (range) {
        case 0:
          lcd.print("RAIN");
          break;
        case 1:
          lcd.print("RAIN WARNING");
          break;
        case 2:
          lcd.print("NOT RAINING");
          break;
        default:
          lcd.print("ERROR");
      }
    } else {
      lcd.print("ERROR");
    }
    break;
  case 5:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LIGHT");
    lcd.setCursor(0, 1);
    if (lux != -1) {
      if (lux >= 0 && lux <= 100) {
        lcd.print("DARK");
      } else if (lux >= 101 && lux <= 1000) {
        lcd.print("OVERCAST");
      } else if (lux >= 1001 && lux <= 10000) {
        lcd.print("BRIGHT");
      } else if (lux >= 10001 && lux <= 65535) {
        lcd.print("VERY BRIGHT");
      }
    } else {
      lcd.print("ERROR");
    }
    break;
  }
}

void handleSensorIntervals(unsigned long currentMillis) {
  if (currentMillis - sensorMilis_1 >= sensorinterval_1) {
    sensorMilis_1 = currentMillis;
    if (state == 4) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RAINFALL");
      lcd.setCursor(0, 1);
      if (rainSensorStatus) {
        switch (range) {
          case 0:
            lcd.print("RAIN");
            break;
          case 1:
            lcd.print("RAIN WARNING");
            break;
          case 2:
            lcd.print("NOT RAINING");
            break;
          default:
            lcd.print("ERROR");
        }
      } else {
        lcd.print("ERROR");
      }
    }
  }

  if (currentMillis - sensorMilis_2 >= sensorinterval_2) {
    sensorMilis_2 = currentMillis;
    if (state == 5) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("LIGHT");
      lcd.setCursor(0, 1);
      if (lux != -1) {
        if (lux >= 0 && lux <= 100) {
          lcd.print("DARK");
        } else if (lux >= 101 && lux <= 1000) {
          lcd.print("OVERCAST");
        } else if (lux >= 1001 && lux <= 10000) {
          lcd.print("BRIGHT");
        } else if (lux >= 10001 && lux <= 65535) {
          lcd.print("VERY BRIGHT");
        }
      } else {
        lcd.print("ERROR");
      }
    }
  }
}
