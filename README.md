
# Arduino AAC Encoding and Decoding Library

I was looking for a way to encode sound PCM data to the AAC or MPEG data format on some Arduino Devices. That's when I found  the [Fraunhofer FDK AAC library](https://en.wikipedia.org/wiki/Fraunhofer_FDK_AAC). 

I have forked the [fdk-aac](https://github.com/mstorsjo/fdk-aac/tree/v2.0.1) project and converted it to an Arduino library. 

The Android-targeted implementation of the Fraunhofer AAC encoder uses fixed-point math and is optimized for encoding on embedded devices/mobile phones. The library is currently limited to 16-bit PCM input. So this seems to be the perfect match to be used in Arduino based Microcontrollers.

I have also added a basic C++ API which should be quite easy to use: 

## Encode to AAC
Here is a simple example sketch which encodes PWM data to AAC and stores the result in a SD file.  In the constructor of the AACEncoder we define the result Stream. Fortunatly the File is a subclass of Stream, so we can pass it directly as argument. To keep things simple, the input PWM data is just provided as an array.

In order to encode we just need to feed the encoder with PWM data by calling write()

```
#include <SPI.h>
#include <SD.h>
#include "AACEncoder.h"
#include "StarWars30.h"
#include "AudioTools.h"

AACEncoder *encoder;
File dataFile; 
MemoryStream music(StarWars30_raw, StarWars30_raw_len);

void setup(){
    Serial.begin(115200);
    SD.begin(); 

    // setup encoder
    dataFile = SD.open("example.aac", FILE_WRITE);
    encoder = new AACEncoder(dataFile);
    encoder->begin();
    Serial.println("Creating AAC file...");
}

void loop(){
    static uint8_t buffer[512];
    if (dataFile){
        if (music.available()>0){
            int len = music.readBytes(buffer, 512);
            encoder->write(buffer, len);
        } else {
            // no more data -> close file
            dataFile.close();
            Serial.println("File has been closed");
        }
    }
}
```

## Decode AAC
Decoding AAC converts the data to an array of PWM data. For the ESP32 we provide Stream implementation of the I2S interface, so that we can just use the i2s stream as result output of the deconding. By default this is sent to the internal DAC of the ESP32.
In the loop we just feed the decoder with AAC data. The input AAC data is also provided as an array.

```
#include "AudioTools.h" 
#include "aac.h"

I2S i2s; // I2S output destination
AACDecoder decoder(i2s);
MemoryStream music(sample1_aac, sample1_aac_len);


void setup(){
    Serial.begin(115200);
    i2s.begin();
    decoder.begin();
}

void loop(){
    static uint8_t buffer[512];
    if (music.available()>0){
        int len = music.readBytes(buffer, 512);
        decoder.write(buffer, len);
    }
}
```

## Installation
You can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone pschatzmann/arduino-fdk-aac.git
```

## Compile Issues / Supported Architectures
On some processors we get compile errors because the arrays are too big.  
Please uncomment the #define PIT_MAX_MAX line in the AACConstantsOverride.h file. 

All examples have been written and tested on a ESP32. The basic funcationality of the encoder and decoder however should work on all Arduino Devices and is independent from the processor.

## Project Status
This is currently work in progress. The standard fdk-aac functionality seems to compile w/o errors now - but I did not perform any tests yet. 
