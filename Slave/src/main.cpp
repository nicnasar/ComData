#include <Arduino.h>
#include <ESP32SPISlave.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// configs do MQTT

WiFiClientSecure wifi;
PubSubClient mqtt(wifi);

// WiFi
const char *ssid = "NICOLAS-GAIN";
const char *password = "senha123";
// const char *ip = "broker.hivemq.com";
// const int port = 1883;
const int port = 8884;
const char *ip = "humblecuckoo-90269042.a01.euc1.aws.hivemq.cloud";
const char *topic = "trabalho/temperatura";

ESP32SPISlave slave; // criar o objeto da classe

static constexpr uint8_t BUFFER_SIZE{4};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void reconnect(String temp)
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("esp32-trabalho-comdata"))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt.publish(topic, temp.c_str());
    }
    else
    {
      Serial.print("failed, rc=");
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
  delay(1000);

  WiFi.begin(ssid, password);
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
      reconnect(tempStr);
    }
    mqtt.loop();
  }
}
