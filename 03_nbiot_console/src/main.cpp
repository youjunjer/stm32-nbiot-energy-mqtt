#include <Arduino.h>

// PC debug terminal over ST-LINK VCP
#define PC_PORT Serial
// NB-IoT module UART: USART1 RX=PA10(D2), TX=PA9(D8)
HardwareSerial nbiotUart(PA10, PA9);
#define NBIOT_PORT nbiotUart

static char cmdBuf[192];
static size_t cmdLen = 0;
static uint32_t lastBlinkMs = 0;
static uint32_t lastProbeMs = 0;
static bool ledState = false;

static void sendLineToNbiot(const char *s)
{
  PC_PORT.print("\r\n[TX] ");
  PC_PORT.println(s);

  NBIOT_PORT.write((const uint8_t *)s, strlen(s));
  NBIOT_PORT.write('\r');
}

static void sendCommandLine()
{
  if (cmdLen == 0) {
    return;
  }

  cmdBuf[cmdLen] = '\0';
  sendLineToNbiot(cmdBuf);

  cmdLen = 0;
  PC_PORT.print("> ");
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  PC_PORT.begin(115200);
  NBIOT_PORT.begin(115200);

  delay(300);
  PC_PORT.println();
  PC_PORT.println("=== NB-IoT Console ===");
  PC_PORT.println("NB UART: USART1 (TX=PA9/D8, RX=PA10/D2), 115200 8N1");
  PC_PORT.println("Type AT command and press Enter. Example: ATI, AT, AT+CSQ");
  PC_PORT.println("Auto probe: send AT every 3 seconds");
  PC_PORT.print("> ");
}

void loop()
{
  while (PC_PORT.available() > 0) {
    int c = PC_PORT.read();
    if (c < 0) {
      continue;
    }

    if (c == '\r' || c == '\n') {
      sendCommandLine();
      continue;
    }

    if ((c == 0x08 || c == 0x7F) && cmdLen > 0) {
      cmdLen--;
      PC_PORT.print("\b \b");
      continue;
    }

    if (cmdLen < (sizeof(cmdBuf) - 1U)) {
      cmdBuf[cmdLen++] = (char)c;
      PC_PORT.write((char)c);
    }
  }

  while (NBIOT_PORT.available() > 0) {
    int c = NBIOT_PORT.read();
    if (c >= 0) {
      if (c == '\r') {
        continue;
      }
      PC_PORT.write((char)c);
    }
  }

  if ((millis() - lastProbeMs) >= 3000) {
    lastProbeMs = millis();
    sendLineToNbiot("AT");
  }

  if ((millis() - lastBlinkMs) >= 500) {
    lastBlinkMs = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  }
}
