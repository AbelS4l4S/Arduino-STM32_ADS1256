# ADC 1256 y STM32

## Schematics

# Pin configuration - STM32F103C8T6

## SPI default pins

STM32|||ADS1256||
-|-|-|-|-
MOSI1|PA7||DIN|P23
MISO1|PA6||DOUT|P22
SCK1|PA5||SLCK|P24
NSS1|PA4||CS|P20

**MOSI:** Master OUT Slave IN  
**MISO:** Master IN Slave OUT

## Other pins

STM32|||ADS1256||
-|-|-|-|-
ADC3/RX2/T2C4|PA3||RESET|P15
ADC2/TX2/T2C3|PA2||DRDY|P21
ADC1/RTS2/T2C2|PA1 / +3.3V||PDWN/SYNC|P14

## Clock rate

**f_CLKIN** = 7.68 MHz  
**tau** = 130.2 ns

## Resources

[Very low noise, 24-Bit analog-to-digital converter DataSheet](https://www.ti.com/lit/ds/symlink/ads1256.pdf?ts=1671416123919&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FADS1256)

- ADS 1256 24b bits and Arduino/STM32 [Curious Scientist blog](<https://curiousscientist.tech/blog/ads1256-improved-arduino-code>) | [Youtube video](<https://www.youtube.com/watch?v=rsi9o5PQzwM&list=PLaeIi4Gbl1T-RpVNM8uKdiV1G_3t5jCIu&index=5>)
- ADS1256-and-Arduino by mbilsky [Github Repo](https://github.com/mbilsky/ADS1256-and-Arduino)
- ADS1256 Arduino library by adienakhmad [GitHub Repo](https://github.com/adienakhmad/ADS1256)
- ADS1256.c by dariosalvi78 [Github Repo](https://gist.github.com/dariosalvi78/f2e990b4317199d235bbf5963c3486ae)