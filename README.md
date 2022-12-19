# ADC 1256 y STM32

The original code was taken form:

- <https://www.youtube.com/watch?v=rsi9o5PQzwM&list=PLaeIi4Gbl1T-RpVNM8uKdiV1G_3t5jCIu&index=5>
- <https://curiousscientist.tech/blog/ads1256-improved-arduino-code>

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
