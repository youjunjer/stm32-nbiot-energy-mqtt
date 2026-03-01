#include <Arduino.h>

// Debug UART to PC (ST-LINK VCP on NUCLEO-L053R8)
#define DBG_PORT Serial
// NB-IoT UART instance on USART1 pins (RX=PA10, TX=PA9)
HardwareSerial nbiotUart(PA10, PA9);
#define NBIOT_PORT nbiotUart

static uint32_t g_round = 0;

static void readNbiotResponse(uint32_t waitMs)
{
  uint32_t start = millis();
  bool gotAny = false;

  while ((millis() - start) < waitMs) {
    while (NBIOT_PORT.available() > 0) {
      int c = NBIOT_PORT.read();
      if (c >= 0) {
        DBG_PORT.write((char)c);
        gotAny = true;
      }
    }
    delay(2);
  }

  if (!gotAny) {
    DBG_PORT.println("[NBIOT] (no response)");
  } else {
    DBG_PORT.println();
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  DBG_PORT.begin(115200);
  NBIOT_PORT.begin(115200);

  delay(500);
  DBG_PORT.println("02_nbiot_ati start");
  DBG_PORT.println("Send ATI every 10 seconds...");
}

void loop()
{
  g_round++;
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  while (NBIOT_PORT.available() > 0) {
    NBIOT_PORT.read();
  }

  DBG_PORT.print("[ROUND ");
  DBG_PORT.print(g_round);
  DBG_PORT.println("] TX: ATI");

  NBIOT_PORT.print("ATI\r\n");

  DBG_PORT.println("[NBIOT RX]");
  readNbiotResponse(2000);

  delay(8000);
}
