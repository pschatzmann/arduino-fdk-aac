
# Arduino AAC Encoding and Decoding Library

I was looking for a way to encode sound PCM data to the AAC or MPEG data format on some Arduino Devices. That's when I found  the [Fraunhofer FDK AAC library](https://en.wikipedia.org/wiki/Fraunhofer_FDK_AAC). 

I have forked the [fdk-aac](https://github.com/mstorsjo/fdk-aac/tree/v2.0.1) project and converted it to an Arduino library. 

The Android-targeted implementation of the Fraunhofer AAC encoder uses fixed-point math and is optimized for encoding on embedded devices/mobile phones. The library is currently limited to 16-bit PCM input. So this seems to be the perfect match to be used in Arduino based Microcontrollers.

## Copyright

Please read the included [NOTICE](NOTICE).

## Examples

Examples can be found in the [arduino-audio-tools library](https://github.com/pschatzmann/arduino-audio-tools) 

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

