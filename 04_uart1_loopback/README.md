# 04_uart1_loopback

Self test for USART1 loopback on NUCLEO-L053R8.

## Wiring
- Short `D8` and `D2` together.
- Open debug port `COM13` at 115200.

## Expected output
- `[ROUND n] PASS` every second when loopback is good.
- `FAIL` means TX/RX path is not working.
