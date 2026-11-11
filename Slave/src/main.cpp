#include <Arduino.h>
#include <ESP32SPISlave.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "esp_wpa2.h"

// configs do MQTT

WiFiClientSecure wifi;
PubSubClient mqtt(wifi);

// WiFi
const char *ssid = "ws_wlan";

const char *identidade = "20241ceca0204";
const char *nome_user = "20241ceca0204";
const char *password = "@Academico123";

// const char *ssid = "iPhone de Matheus";
// const char *password = "matheus123";
// const char *ip = "broker.hivemq.com";
// const int port = 1883;
const uint16_t port = 8883;
const char *ip = "humblecuckoo-90269042.a01.euc1.aws.hivemq.cloud";
const char *topic = "datacom/temp";

const char *user = "datacom";
const char *pass = "Datacom12";

String id = "esp32-" + WiFi.macAddress();

ESP32SPISlave slave; // criar o objeto da classe

static constexpr uint8_t BUFFER_SIZE{4};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void reconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(id.c_str(), user, pass))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
    }
    else
    {
      Serial.print("failed, rc = ");
      Serial.print(mqtt.state());
      Serial.println(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)identidade, strlen(identidade));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)nome_user, strlen(nome_user));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
  esp_wifi_sta_wpa2_ent_enable();

  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED)
    delay(200);
  Serial.println("Conectado ao Hotspot.");
  wifi.setInsecure();
  mqtt.setServer(ip, port); // IP do PC na rede, não "localhost"

  // Init slave connection to the master ESP32
  slave.setDataMode(SPI_MODE0);
  slave.begin(VSPI);

  // limpa os buffers
  memset(spi_slave_tx_buf, 0, BUFFER_SIZE);
  memset(spi_slave_rx_buf, 0, BUFFER_SIZE);
  // endereço, valor, tamanho
}

void loop()
{
  const size_t received_bytes = slave.transfer(spi_slave_tx_buf, spi_slave_rx_buf, BUFFER_SIZE);

  float temp = 0.0;
  size_t tamanho = sizeof(temp);

  if (received_bytes == tamanho)
  {
    memcpy(&temp, spi_slave_rx_buf, tamanho);
    Serial.printf("Temperatura em %.2f °C\n", temp);

    String tempStr = String(temp, 2);

    if (!mqtt.connected())
    {
      reconnect();
    }
    mqtt.publish(topic, tempStr.c_str());
    mqtt.loop();
  }
}
