# 07_energy_pzem_read

Ported from your ESP32 code.

## Wiring
- Energy TX -> D2 (PA10)
- Energy RX <- D8 (PA9)
- GND common

## UART
- Energy: 9600 8N1
- Debug: COM13 115200 8N1

## Behavior
- Poll every 10s
- Print raw hex + decoded V/A/W/kWh
