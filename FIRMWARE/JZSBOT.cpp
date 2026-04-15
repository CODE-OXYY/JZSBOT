#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM6DS33.h> 
#include <Adafruit_BME680.h>
#include <Adafruit_VL53L0X.h>
#include <TFT_eSPI.h>          
#include <driver/i2s.h>        

// PINS
#define I2C1_SDA 35   // SDA1
#define I2C1_SCL 36   // SCL1
#define I2C2_SDA 2    // SDA2
#define I2C2_SCL 13   // SCL2

//  Motor Driver
#define MOT_AIN1 9
#define MOT_AIN2 10
#define MOT_BIN1 11
#define MOT_BIN2 12
#define MOT_SLEEP 13  

// I2S Speaker
#define I2S_BCLK  48
#define I2S_LRC   21
#define I2S_DOUT  47


Adafruit_LSM6DS33 lsm6ds3;
Adafruit_BME680 bme; 
Adafruit_VL53L0X lox_back = Adafruit_VL53L0X();  
Adafruit_VL53L0X lox_front = Adafruit_VL53L0X(); 
TFT_eSPI tft = TFT_eSPI(); 
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("\n--- JZSBOT STARTTTTTTTTTT ---");

  //  DISPLAY
  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("JZSBOT NOW I AM COMING", 10, 10, 4);
  Serial.println("Display Initialized");

  // MOTOR 
  pinMode(MOT_AIN1, OUTPUT);
  pinMode(MOT_AIN2, OUTPUT);
  pinMode(MOT_BIN1, OUTPUT);
  pinMode(MOT_BIN2, OUTPUT);
  pinMode(MOT_SLEEP, OUTPUT);
  digitalWrite(MOT_SLEEP, HIGH); 
  Serial.println("LEGS Initialized !!!!!");

  // DUAL I2C  SETUP
  Wire.begin(I2C1_SDA, I2C1_SCL);        
  Wire1.begin(I2C2_SDA, I2C2_SCL);       

  // LSM6DS3 
  if (!lsm6ds3.begin_I2C(0x6A, &Wire)) { 
    Serial.println("LSM6DS3 NOT FOUND!");
    tft.drawString("LSM6DS3: FAIL", 10, 40, 2);
  } else {
    Serial.println("LSM6DS3 Found!");
    tft.drawString("LSM6DS3: OK", 10, 40, 2);
  }

  // BME680 
  if (!bme.begin(0x76, &Wire)) { 
    Serial.println(" BME680 NOT FOUND!");
    tft.drawString(" BME680: FAIL", 10, 60, 2);
  } else {
    Serial.println(" BME680 Found!");
    tft.drawString("BME680: OK", 10, 60, 2);
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); 
  }

   // VL53L0X 
  if (!lox_back.begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT)) {
    Serial.println("VL53L0X (Back) NOT FOUND!");
  } else {
    Serial.println("VL53L0X (Back) Found!");
  }

  if (!lox_front.begin(0x29, false, &Wire1, Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT)) {
    Serial.println(" VL53L0X (Front) NOT FOUND!");
  } else {
    Serial.println("VL53L0X (Front) Found!");
  }

   // AUDIO
  i2s_config_t i2s_config = {
   .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
   .sample_rate = 44100,
   .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
   .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
   .communication_format = I2S_COMM_FORMAT_STAND_I2S,
   .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
   .dma_buf_count = 8,
   .dma_buf_len = 64,
   .use_apll = false
  };
  i2s_pin_config_t pin_config = {
   .bck_io_num = I2S_BCLK,
   .ws_io_num = I2S_LRC,
   .data_out_num = I2S_DOUT,
   .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  Serial.println("Audio Initialized !!!!");

  Serial.println(" JZS WOKE UP \n");
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  if (millis() - lastReadTime > 1000) { 
    lastReadTime = millis();
    
    // READ BME680
    if (bme.performReading()) {
      Serial.print("BME680   -> Temp: "); Serial.print(bme.temperature); Serial.println(" *C");
    }
    // READ LSM6DS3
    sensors_event_t accel, gyro, temp;
    lsm6ds3.getEvent(&accel, &gyro, &temp);
    Serial.print("LSM6DS3  -> Accel X: "); Serial.print(accel.acceleration.x); Serial.println(" m/s^2");

    // READ VL53L0X
    VL53L0X_RangingMeasurementData_t measure_back;
    lox_back.rangingTest(&measure_back, false);
    if (measure_back.RangeStatus!= 4) {
      Serial.print("VL53 (B) -> Dist: "); Serial.print(measure_back.RangeMilliMeter); Serial.println(" mm");
    }
    
    VL53L0X_RangingMeasurementData_t measure_front;
    lox_front.rangingTest(&measure_front, false);
    if (measure_front.RangeStatus!= 4) {
      Serial.print("VL53 (F) -> Dist: "); Serial.print(measure_front.RangeMilliMeter); Serial.println(" mm");
    }

    Serial.println("====================================\n");
  }

  testMotors(); 
}

void testMotors() {
  // Forward
  digitalWrite(MOT_AIN1, HIGH); digitalWrite(MOT_AIN2, LOW);
  digitalWrite(MOT_BIN1, HIGH); digitalWrite(MOT_BIN2, LOW);
  delay(1000);
  // Stop
  digitalWrite(MOT_AIN1, LOW); digitalWrite(MOT_AIN2, LOW);
  digitalWrite(MOT_BIN1, LOW); digitalWrite(MOT_BIN2, LOW);
  delay(2000);
}
