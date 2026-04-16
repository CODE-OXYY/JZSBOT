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
#define motA1 9
#define motA2 10
#define motB1 11
#define motB2 12
#define motSleep 13  

// I2S Speaker
#define I2C_BCLK  48
#define I2S_LRC   21
#define I2S_DOUT  47


Adafruit_LSM6DS33 lsm;
Adafruit_BME680 bme; 
Adafruit_VL53L0X laser_back = Adafruit_VL53L0X();  
Adafruit_VL53L0X laser_front = Adafruit_VL53L0X(); 
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
  Serial.println("FACE Initialized");

  // MOTOR 
  pinMode(motA1, OUTPUT);
  pinMode(motA2, OUTPUT);
  pinMode(motB1, OUTPUT);
  pinMode(motB2, OUTPUT);
  pinMode(motSleep, OUTPUT);
  digitalWrite(motSleep, HIGH); 
  Serial.println("LEGS Initialized !!!!!");

  // DUAL I2C  SETUP
  Wire.begin(I2C1_SDA, I2C1_SCL);        
  Wire1.begin(I2C2_SDA, I2C2_SCL);       

  // LSM6DS3 
  if (!lsm.begin_I2C(0x6A, &Wire)) { 
    Serial.println("LSM6DS3 Not Found :( ");
    tft.drawString("RIP LSM6DS3 KABOOM", 10, 40, 2);
  } else {
    Serial.println("LSM6DS3 Found :)");
    tft.drawString("LSM6DS3 Is Happyyy", 10, 40, 2);
  }

  // BME680 
  if (!bme.begin(0x76, &Wire)) { 
    Serial.println(" BME680 Not Found :( ");
    tft.drawString(" RIP BME680 KABOOM ", 10, 60, 2);
  } else {
    Serial.println(" BME680 Found :) ");
    tft.drawString("BME680 Is Happyyy ", 10, 60, 2);
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); 
  }

   //  LASER (VL53L0X)
  if (!laser_back.begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT)) {
    Serial.println("Laser Back Not Found :( ");
  } else {
    Serial.println("Laser Back Found :) ");
  }

  if (!laser_front.begin(0x29, false, &Wire1, Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT)) {
    Serial.println(" Laser Front Not Found :( ");
  } else {
    Serial.println("Laser Front Found :)");
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
   .bck_io_num = I2C_BCLK,
   .ws_io_num = I2S_LRC,
   .data_out_num = I2S_DOUT,
   .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  Serial.println("Audio Initialized HEHEHEHE !!!!");

  Serial.println(" JZS WOKE UP \n");
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  if (millis() - lastReadTime > 1000) { 
    lastReadTime = millis();
    
    // READ BME680 Sensor Value
    if (bme.performReading()) {
      Serial.print("Temperature is ");
      Serial.print(bme.temperature);
      Serial.println(" *C");
    }

    // READ LSM6DS3 Values
    sensors_event_t accel, gyro, temp;
    lsm.getEvent(&accel, &gyro, &temp);
    Serial.print("Acceleration: "); 
    Serial.print(accel.acceleration.x); 
    Serial.println(" m/s^2");

    // READ VL53L0X Values
    VL53L0X_RangingMeasurementData_t measure_back;
    laser_back.rangingTest(&measure_back, false);
    if (measure_back.RangeStatus!= 4) {
      Serial.print("Above Surface Back: "); 
      Serial.print(measure_back.RangeMilliMeter); 
      Serial.println(" mm");
    }
    
    VL53L0X_RangingMeasurementData_t measure_front;
    laser_front.rangingTest(&measure_front, false);
    if (measure_front.RangeStatus!= 4) {
      Serial.print("Above Surface Front: "); 
      Serial.print(measure_front.RangeMilliMeter); 
      Serial.println(" mm");
    }

    Serial.println("========= HIIIIIIIIIIIIIII =======\n");
  }

  motarTest(); 
}

void motarTest() {
  // Move forward
  digitalWrite(motA1, HIGH); 
  digitalWrite(motA2, LOW);

  digitalWrite(motB1, HIGH); 
  digitalWrite(motB2, LOW);
  delay(1000);

  // Stopppp
  digitalWrite(motA1, LOW); 
  digitalWrite(motA2, LOW);

  digitalWrite(motB1, LOW); 
  digitalWrite(motB2, LOW);
  delay(2000);
}
