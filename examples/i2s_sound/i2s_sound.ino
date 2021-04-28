/**
 * @file i2s_sound.ino
 * @author Phil Schatzmann
 * @brief A simple tool to test the ESP32 Audio DAC with the help of a SineWaveGenerator which generates a tone.
 * @version 0.1
 * @date 2021-01-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "AudioToolsESP32.h" 
#include "Notes.h"

I2S i2s; // I2S output destination
SineWaveGenerator<int16_t> sound(0x7FFF);

void setup(){
    Serial.begin(115200);
    sound.begin(44100, D4);
    i2s.begin(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN, sound);
}

void loop(){
    static uint8_t buffer[512];
    int len = sound.readBytes(buffer, 512);
    i2s.write(buffer, len);
}