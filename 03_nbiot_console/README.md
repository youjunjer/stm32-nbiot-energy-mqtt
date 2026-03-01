# 03_nbiot_console

Interactive UART bridge for NB-IoT AT commands.

## Wiring
- NB TX -> PA10 (D2)
- NB RX <- PA9 (D8)
- GND common
- NB UART: 115200, 8N1

## Usage
1. Flash firmware.
2. Open COM13 at 115200.
3. Type command (e.g. `ATI`) and press Enter.
4. Module response will be printed directly.
