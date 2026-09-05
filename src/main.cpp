#include <Arduino.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

const int ledPin = 25;
SX1276 radio = new Module(18, 26, 23, 33);

Adafruit_SSD1306 display(128, 64, &Wire, -1);

volatile bool txDone = false;

void IRAM_ATTR onTxDone()
{
  txDone = true;
}

void ledTask(void *param)
{
  uint32_t state = LOW;
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (true)
  {
    int64_t time = esp_timer_get_time();

    digitalWrite(ledPin, state);

    state = !state;
    Serial.print("state: ");
    Serial.println(state);
    Serial.print("time: ");
    Serial.println(time);

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));
  }
}

void radioTask(void *param)
{
  uint8_t packet[32];

  for (int i = 0; i < 32; i++)
  {
    packet[i] = 'A' + (i % 26);
  }

  while (true)
  {

    txDone = false;
    int state = radio.startTransmit(packet, sizeof(packet));

    if (state != RADIOLIB_ERR_NONE)
    {
      Serial.print("TX start failed, code: ");
      Serial.println(state);
      vTaskDelete(nullptr);
    }

    Serial.println("TX started asynchronously");

    while (!txDone)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    state = radio.finishTransmit();

    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println("TX finished");
    }
    else
    {
      Serial.print("TX finish failed, code: ");
      Serial.println(state);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void setup()
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);

  ConfigLoRa_t config;
  config.frequency = 868;
  config.spreadingFactor = 11;
  config.bandwidth = 500.0;
  config.codingRate = 5;

  int state = radio.begin(config);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED не знайдено");
    while (true)
    {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);

  if (state == RADIOLIB_ERR_NONE)
  {

    radio.setDio0Action(onTxDone, RISING);

    Serial.println(F("success!"));

    display.clearDisplay();
    display.println("success!");
    display.display();
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);

    display.clearDisplay();
    display.print(F("failed, code "));
    display.display();
    while (true)
    {
      delay(10);
    }
  }

  xTaskCreatePinnedToCore(
      radioTask,
      "radioTask",
      2048,
      nullptr,
      1,
      nullptr,
      0);
  xTaskCreatePinnedToCore(
      ledTask,
      "ledTask",
      2048,
      nullptr,
      1,
      nullptr,
      1);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(100));
}
