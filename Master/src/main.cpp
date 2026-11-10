#include <Arduino.h>
#include <Preferences.h> // salvar na EEPROM
#include <ESP32SPISlave.h> // comunicação spi

SPIClass master(VSPI); // configura automaticamente os pinos do VSPI
static constexpr uint8_t PIN_SS = 5;
static constexpr size_t BUFFER_SIZE {4};

uint8_t tx_buf[BUFFER_SIZE];
uint8_t rx_buf[BUFFER_SIZE];

Preferences prefs;

const int ntcPin = 33;   // Pino analógico do NTC

// parâmetros do NTC e divisor resistivo
const float BETA = 3950.0;     // confira no datasheet do NTC (3380, 3435 e 3950 são comuns)
const float R0 = 10000.0;
const float Tzero = 298.15; // (273.15 + 25.00)
const float R_SERIE = 10000.0;
const float VCC_MV = 3300.0;   // tensão que alimenta o divisor

float temperaturaAtual = 0.0;
float temperaturaAnterior = 0.0;

// função para converter tensão no pino (mV) -> Temperatura (°C)
float calcularTemperatura(int mv) { 
  if (mv <= 0 || mv >= VCC_MV) return NAN; // checar se está dentro dos limites

  // ntc no 3.3V (resistor fixo para GND): Vout = VCC * R_SERIE / (R_SERIE + Rntc)
  float Rntc = R_SERIE * (VCC_MV - mv) / mv; // calcular resistência do ntc

  float tempK = 1.0 / ((1.0 / Tzero) + (1.0 / BETA) * log(Rntc / R0)); // calcular a temperatura em kelvin
  return tempK - 273.15; // converter a temperatura em kelvin para celsius e aplicar a calibração
}


void setup() {
  Serial.begin(115200);

  pinMode(PIN_SS, OUTPUT);
  digitalWrite(PIN_SS, HIGH);
  master.begin(SCK, MISO, MOSI, PIN_SS); // inicializa os pinos do VSPI previamente configurados

  // limpar os buffers
	memset(tx_buf, 0, BUFFER_SIZE); 
	memset(rx_buf, 0, BUFFER_SIZE);
  // endereço, valor, tamanho

  Serial.println("SPI configurado e incializado.");

  // Inicia o namespace "ntc" na flash (equivalente à EEPROM no ESP32)
  prefs.begin("ntc", false);

  // Lê o último valor analógico armazenado (em mV)
  int mvAnterior = prefs.getInt("mv", -1);

  if (mvAnterior != -1) {
    temperaturaAnterior = calcularTemperatura(mvAnterior);
    Serial.println("Recuperando dados salvos na EEPROM (Preferences):");
    Serial.print("Tensão anterior: ");
    Serial.print(mvAnterior);
    Serial.print(" mV | Temperatura anterior: ");
    Serial.print(temperaturaAnterior);
    Serial.println(" °C");
  } else {
    Serial.println("Nenhum valor anterior encontrado na EEPROM.");
  }

  Serial.println();
}

void loop() {
  // lê a tensão atual do NTC já calibrada
  // média de 32 leituras para reduzir o ruído do ADC
  uint32_t soma = 0;
  for (int i = 0; i < 32; i++) {
    soma += analogReadMilliVolts(ntcPin);
  }
  int mvAtual = soma / 32;
  temperaturaAtual = calcularTemperatura(mvAtual);

  Serial.println("===== Leitura Atual =====");
  Serial.printf("Temperatura anterior: %.2f °C\n", temperaturaAnterior);
  Serial.printf("Temperatura atual: %.2f °C | Tensão: %d mV\n", temperaturaAtual, mvAtual);
  Serial.println("=========================\n");

  // armazena o valor atual (mV) na EEPROM/NVS
  prefs.putInt("mv", mvAtual);

  // atualiza a variável de temperatura anterior na RAM
  temperaturaAnterior = temperaturaAtual;

  memcpy(tx_buf, &temperaturaAtual, BUFFER_SIZE); // escreve o valor da temperatura atual no buffer do tx

  // copiado do exemplo da lib
  master.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_SS, LOW);
  master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
  digitalWrite(PIN_SS, HIGH);
  master.endTransaction();

  Serial.println("Temperatura enviada!\n");

  delay(30000);
}


