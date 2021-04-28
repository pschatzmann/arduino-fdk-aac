/**
 * @file aac_decode.ino
 * @author Phil Schatzmann
 * @brief Decodes AAC data to PWM and sends it to the ESP32 I2S internal DAC
 * @version 0.1
 * @date 2021-01-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "AudioAAC.h"
#include "AudioTools.h" 
#include "music.h"

I2S i2s; // I2S output destination
AACDecoder decoder(i2s);
MemoryStream music(music_aac, music_aac_len);


void setup(){
    Serial.begin(115200);
    i2s.begin();
    decoder.begin(TT_MP4_LOAS);
}

void loop(){
    static uint8_t buffer[512];
    if (music.available()>0){
        int len = music.readBytes(buffer, 512);
        decoder.write(buffer, len);
    }
}