# 02_nbiot_ati

Board: NUCLEO-L053R8

## UART mapping
- Debug to PC: ST-LINK VCP (`COM13`) via `Serial` at 115200 8N1
- NB-IoT: `Serial1` (USART1)
  - MCU TX: `PA9` (`D8`) -> NB RX
  - MCU RX: `PA10` (`D2`) <- NB TX
  - GND common

## Behavior
- Send `ATI` every 10 seconds
- Wait 2 seconds for response and print it to debug port
- If no response, print `[NBIOT] (no response)`

## Build/Upload (PlatformIO)
- `python -m platformio run -t upload`
- Open monitor at 115200
