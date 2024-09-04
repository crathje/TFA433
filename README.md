[![Actions Status](https://github.com/crathje/TFA433/workflows/PlatformIO%20CI/badge.svg)](https://github.com/crathje/TFA433/actions)

# TFA 433

Use your Arduino to receive temperature and humidity data from TFA remote temperatur sensor.

This is a fork of https://github.com/denxhun/TFA433 - credits to denxhun.

Adapted to work with the TFA 30.3212.02 temperature only sensors ( https://www.tfa-dostmann.de/produkt/temperatursender-30-3212/ ) that come along with the compatible base station 30.3055.01 ( https://www.tfa-dostmann.de/produkt/funk-thermometer-joker-30-3055/ ).

## Download
https://github.com/crathje/TFA433

## Info
### Hardware

There are several 433 MHz receiver available in many places. I used a cheap YX-MK-5V and it works perfectly. 
A level shifter to 3.3V for ESP32 is recommended - a voltage divider will most likely also work. 

### Usage

Use at your own risk! Truly! There is no warranty!

In the examples directory you can find a simple way of usage.

## Protocol

The protocol of the TFA 30.3212.02 differs from the original one used in this fork's source. Like there is no humidity available and the temperature is provided as binary encoded degrees celcius. 

See this capture while transmitting 27.2°C as reference:
![LA Captured TFA 30.3212.02 transmitting 27.2°C](raw-data/TFA-30.3212.02-sample-data-captured-27.2-degree-celcius.png?raw=true)

Checksum seems to be
```
reveng -w8 -sF 42105affe6 421845ff43 42125aff6a 421000ffef 4210b4fffd 4210cbff5f 4211ceff6e
width=8  poly=0x31  init=0x00  refin=false  refout=false  xorout=0x00  check=0xa2  residue=0x00  name=(none)
```


### Inspiration

Thanks to https://github.com/denxhun/TFA433 for the basics.