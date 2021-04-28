/**
 * @file sample_pwm.ino
 * @author Phil Schatzmann 
 * @brief ESP32 implementation which samples pwm data with the help of a timer
 * @version 0.1
 * @date 2021-01-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "AudioTools.h"

AnalogSampler sampler;
File file;

void setup() {
  Serial.begin(115200);
  setupSD();
  sampler.begin();
}

void setupSD() {
  if(!SD.begin()){
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }
}

void fileReopen(){
  if (file)
    file.close();
  file = SD.open("sound.pwm", FILE_APPEND);  
}

void loop() {
  static int count;
  int16_t* data;
  int data_len;

  // we reopen the file to make sure that the data is saved
  if (--count <= 0){
    count = 1000;
    fileReopen();
  }

  // we save the data to the file
  if(file){
    sampler.read(data, data_len);
    file.write((uint8_t*)data, data_len*sizeof(int16_t));
  }
}
