# 09_energy_softserial_read

Step 2/3/4 plan implementation.

- Poll energy every 10 seconds
- SoftwareSerial RX=D4, TX=D5 at 9600
- Uses listen + clear buffer + write + delay(100) + read available
- Echo-frame detection and one immediate retry
- Fail counter; print SOFTSERIAL_LINK_FAIL after 3 consecutive failures
