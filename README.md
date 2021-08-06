
# Arduino AAC Encoding and Decoding Library

I was looking for a way to encode sound PCM data to the AAC or MPEG data format on some Arduino Devices. That's when I found  the [Fraunhofer FDK AAC library](https://en.wikipedia.org/wiki/Fraunhofer_FDK_AAC). 

The Android-targeted implementation of the Fraunhofer AAC can be used for encoding and decoding, uses fixed-point math and is optimized for encoding on embedded devices/mobile phones. The library is currently limited to 16-bit PCM input. So this seems to be the perfect match to be used in Arduino based Microcontrollers.

I have forked the [fdk-aac](https://github.com/mstorsjo/fdk-aac/tree/v2.0.1) project,  converted it to an Arduino library and provided a simple Arduino friendly API. The only caveat is, that I have removed all optimized processor specific code (e.g. for ARM, 386 and mips), so that it will compile with the same code on all environements.


## Decoding Example

```
#include "AACDecoderFDK.h"
#include "BabyElephantWalk60_aac.h"

using namespace aac_fdk;


void dataCallback(CStreamInfo &info, INT_PCM *pcm_data, size_t len) {
    int channels = info.numChannels;
    for (size_t i=0; i<len; i+=channels){
        for (int j=0;j<channels;j++){
            Serial.print(pcm_data[i+j]);
            Serial.print(" ");
        }
        Serial.println();
    }
}

AACDecoderFDK aac(dataCallback);

void setup() {
    Serial.begin(115200);
    aac.begin();
}

void loop() {
    Serial.println("writing...");
    aac.write(BabyElephantWalk60_aac, BabyElephantWalk60_aac_len);    

    // restart from the beginning
    delay(2000);
    aac.begin();
}

```

## Encoding Example

```
#include "AACEncoderFDK.h"
#include <stdlib.h>     /* srand, rand */

using namespace aac_fdk;

void dataCallback(uint8_t *aac_data, size_t len) {
    Serial.print("AAC generated with ");
    Serial.print(len);
    Serial.println(" bytes");
}

AACEncoderFDK aac(dataCallback);
AudioInfo info;
int16_t buffer[512];

void setup() {
    Serial.begin(115200);

    info.channels = 1;
    info.sample_rate = 10000;
    aac.begin(info);
}

void loop() {
    Serial.println("writing 512 samples of random data");
    for (int j=0;j<512;j++){
        buffer[j] = (rand() % 100) - 50;         
    }
    aac.write(buffer, 512);
}

```


## Installation

You can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone pschatzmann/arduino-fdk-aac.git
```

## Class Documentation

The [generated class documentation](https://pschatzmann.github.io/arduino-fdk-aac/html/) can be found here.

## Compile Issues / Supported Architectures

On some processors we get compile errors because the arrays are too big.  
Please uncomment the #define PIT_MAX_MAX line in the AACConstantsOverride.h file. 

All examples have been written and tested on a ESP32. The basic funcationality of the encoder and decoder however should work on all Arduino Devices and is independent from the processor.

## Copyright

Please read the included [NOTICE](NOTICE).
