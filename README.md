# RP2040 → YM2151 VGM Player

"It's a work in progress"

### Backstory

**2018**
This project started out using an ATMEGA328P and Arduino, but I found that it
was hard to write fast enough code to play songs at full speed. I then found
the STM32 series of microcontrollers; It was promising, but I really didn't like
the SDK or the HAL that was provided for these parts. After a few years, the
Raspberry Pi Foundation released the Raspberry Pi Pico. I bought the board
during its pre-order stage simply because it's a $5 microcontroller. This new
board, with their own RP2040 chip, has one key feature that I didn't know at the
time will make this project viable for me. The Programmable I/O (PIO).

**2022**
I finally got to looking at the datasheet for this board during DEFCON 2022,
where they used the RP2040 for their interactive badge. The PIO section caught
my eye, but I didn't look into it until a month later.

When I read the examples they had for driving WS2812 LED strips, I realized
what I could do with this extra power. I could easily get data to the chip on
time, every time; and have plenty of time for drawing a UI to a screen,
fetching data from an SD card over SPI, and in the future, emulating other
sound chips for game systems with AD/PCM and other chips.

### Hardware

![YM2151 Pico Dev Board](/img/devboard.jpg)

I made this development board for the project.

GPIO 0-7 are connected to D0-D7 on the YM2151. GPIO 8-10 are connected to the
`A0`, `WR`, and `CS` pins. I'm using SPI to connect to the SD card, and an
LTC6903 programmable clock generator making the main clock for the YM2151.

The supporting circuitry for the YM2151 was ripped from the same place I got
the chip from, a Yamaha CX5 Music Computer.

### Software


*To be documented... and written*

### Thank You

Special thanks to:

- [ReimuNotMoe](https://github.com/ReimuNotMoe) for their work on
  [TinyVGM](https://github.com/SudoMaker/TinyVGM). This framework for digesting
  the `.vgm` files is really helpful for me.
- [Aidan Lawrence](https://github.com/AidanHockey5) for open-sourcing his
  [YM2151 music player](https://github.com/AidanHockey5/YM2151_VGM_STM32) which
  got me started in the ATMEGA328P days.
