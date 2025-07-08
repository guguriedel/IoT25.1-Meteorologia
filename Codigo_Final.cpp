#include <Arduino.h>
#include <CayenneLPP.h>
#include <Preferences.h>
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
#include <Adafruit_BME680.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"

// Definição de pinos para LoRaWAN
#define LORA_TX 41
#define LORA_RX 47

// Pinos para o sensor de tempo
int windDirectionPin = 4;
int windSpeedPin = 5;
int rainfallPin = 6;
int lightPin = 16;

int Winddir = 0; // Direção do vento
float valor = 0; // Leitura analógica do vento (para calibração)

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 15000; // Intervalo de 15 segundos para envio

WiFiClientSecure conexaoSegura;
SFEWeatherMeterKit weatherMeterKit(windDirectionPin, windSpeedPin, rainfallPin);
Adafruit_BME680 sensorBME;

CayenneLPP lpp(51); // Buffer para dados do protocolo LoRa

// Configuração da pressão ao nível do mar em hPa
#define SEALEVELPRESSURE_HPA (1013.25)

float readBatteryVoltage()
{
  int raw = analogRead(15);
  float voltage = raw * (3.3 / 4095.0);
  Serial.println("voltagem_divider"+ String(voltage));
  return voltage * (2); // Correção para divisor utilizando 2 resistores iguais
}

int estimateBatteryPercentage(float voltage)
{
  if (voltage >= 4.2)
    return 100;
  if (voltage <= 3.2)
    return 0;
  return (int)((voltage - 3.2)* 100);
}

Preferences preferencias; // Instanciação das preferências
float t_offset= -1.2;
float h_offset=0; 
float l_offset=0;
int minimo=80;
int maximo=100;


void setup()
{
  // Inicia a comunicação serial
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX); // Configuração do LoRa
  Serial1.setTimeout(1000);

  Serial1.println("AT+JOIN"); // Tenta conectar à rede LoRaWAN

  SFEWeatherMeterKitCalibrationParams calibrationParams = weatherMeterKit.getCalibrationParams();

  calibrationParams.vaneADCValues[WMK_ANGLE_0_0] = 1033;
  calibrationParams.vaneADCValues[WMK_ANGLE_22_5] = 3830;
  calibrationParams.vaneADCValues[WMK_ANGLE_45_0] = 2313;
  calibrationParams.vaneADCValues[WMK_ANGLE_67_5] = 2283;
  calibrationParams.vaneADCValues[WMK_ANGLE_90_0] = 3491;
  calibrationParams.vaneADCValues[WMK_ANGLE_112_5] = 3481;
  calibrationParams.vaneADCValues[WMK_ANGLE_135_0] = 3235;
  calibrationParams.vaneADCValues[WMK_ANGLE_157_5] = 3219;
  calibrationParams.vaneADCValues[WMK_ANGLE_180_0] = 2837;
  calibrationParams.vaneADCValues[WMK_ANGLE_202_5] = 1491;
  calibrationParams.vaneADCValues[WMK_ANGLE_225_0] = 1706;
  calibrationParams.vaneADCValues[WMK_ANGLE_247_5] = 1690;
  calibrationParams.vaneADCValues[WMK_ANGLE_270_0] = 327;
  calibrationParams.vaneADCValues[WMK_ANGLE_292_5] = 330;
  calibrationParams.vaneADCValues[WMK_ANGLE_315_0] = 668;
  calibrationParams.vaneADCValues[WMK_ANGLE_337_5] = 666;

  calibrationParams.mmPerRainfallCount = 0.3636;

  calibrationParams.minMillisPerRainfall = 100;

  calibrationParams.kphPerCountPerSec = 2.4;

  calibrationParams.windSpeedMeasurementPeriodMillis = 1000;

  weatherMeterKit.setCalibrationParams(calibrationParams);

  // Begin weather meter kit
  weatherMeterKit.begin();

  // Inicialização do sensor BME680 (temperatura, umidade, pressão)
  if (!sensorBME.begin())
  {
    Serial.println("Erro no sensor BME");
    while (true)
      ; // Loop infinito em caso de erro
  }
  sensorBME.setTemperatureOversampling(BME680_OS_8X);
  sensorBME.setHumidityOversampling(BME680_OS_2X);
  sensorBME.setPressureOversampling(BME680_OS_4X);
  sensorBME.setIIRFilterSize(BME680_FILTER_SIZE_3);
  sensorBME.setGasHeater(0, 0); // ˚C e ms, (0, 0) para desativar


  // Inicialização das preferências
  preferencias.begin("ajustesLora");
}

void debugLPPPayload(const uint8_t *buffer, size_t size)
{
  Serial.println("Decodificação do Payload CayenneLPP:");
  for (size_t i = 0; i < size;)
  {
    uint8_t channel = buffer[i++];
    uint8_t type = buffer[i++];

    switch (type)
    {
    case 0x67: // Temperature
    {
      int16_t val = (buffer[i++] << 8) | buffer[i++];
      float temp = val / 10.0;
      Serial.print("Canal ");
      Serial.print(channel);
      Serial.print(" - Temperatura: ");
      Serial.print(temp);
      Serial.println(" °C");
      break;
    }
    case 0x68: // Humidity
    {
      uint8_t val = buffer[i++];
      float hum = val / 2.0;
      Serial.print("Canal ");
      Serial.print(channel);
      Serial.print(" - Umidade: ");
      Serial.print(hum);
      Serial.println(" %");
      break;
    }
    case 0x73: // Barometric pressure
    {
      uint16_t val = (buffer[i++] << 8) | buffer[i++];
      float pressure = val / 10.0;
      Serial.print("Canal ");
      Serial.print(channel);
      Serial.print(" - Pressão: ");
      Serial.print(pressure);
      Serial.println(" hPa");
      break;
    }
    case 0x02: // Analog Input
    case 0x74: // Luminosity (unsigned 2 bytes)
    {
      uint16_t val = (buffer[i++] << 8) | buffer[i++];
      Serial.print("Canal ");
      Serial.print(channel);
      if (type == 0x02)
        Serial.print(" - Entrada Analógica: ");
      else
        Serial.print(" - Luminosidade: ");
      Serial.println(val);
      break;
    }
    default:
      Serial.print("Canal ");
      Serial.print(channel);
      Serial.print(" - Tipo desconhecido: 0x");
      Serial.println(type, HEX);
      // Ignorar os próximos 2 bytes por segurança
      i += 2;
      break;
    }
  }
}

void loop()
{ // Escuta retorno do módulo LoRaWAN
  if (Serial1.available() > 0)
  {
    String resposta = Serial1.readStringUntil('\n');
    resposta.trim();
    Serial.println("LoRaWAN: " + resposta);

    if (resposta.startsWith("RX"))
    {
      int firstColon = resposta.indexOf(':');
      int secondColon = resposta.indexOf(':', firstColon + 1);
      String hexStr = resposta.substring(firstColon + 1, secondColon);

      String decodedText = "";
      for (int i = 0; i < hexStr.length(); i += 2)
      {
        String byteStr = hexStr.substring(i, i + 2);
        char c = strtol(byteStr.c_str(), nullptr, 16);
        decodedText += c;
      }

      Serial.print("Texto decodificado: ");
      Serial.println(decodedText);

      if (decodedText.startsWith("tempo"))
      {
        int tempo = decodedText.substring(6).toInt();
        preferencias.putInt("tempo", tempo);
        Serial.println("Preferencia salva");
      }
      // offset temperatura
      if (decodedText.startsWith("t_offset"))
      {
        t_offset = decodedText.substring(9).toFloat();
        preferencias.putFloat("t_offset", t_offset);
        float temp_offset = preferencias.getFloat("t_offset");
        Serial.println(temp_offset);
        Serial.println("Preferencia salva");
      }
      // offset humidade
      if (decodedText.startsWith("h_offset"))
      {
        h_offset = decodedText.substring(9).toFloat();
        preferencias.putFloat("h_offset", h_offset);
        Serial.println("Preferencia salva");
      }
      // offset luminosidade
      if (decodedText.startsWith("l_offset"))
      {
        l_offset = decodedText.substring(9).toFloat();
        preferencias.putFloat("l_offset", l_offset);
        Serial.println("Preferencia salva");
      }
      // fator de calibração luminosidade
      if (decodedText.startsWith("cal_lum"))
      {
        String lum = decodedText.substring(8);
        lum.trim(); // remove espaços extras

        // Encontra a posição do espaço entre os dois números
        int separador = lum.indexOf(' ');

        // Separa os valores
        minimo = lum.substring(0, separador).toInt();
        maximo = lum.substring(separador + 1).toInt();
        preferencias.putFloat("lum_min", minimo);
        preferencias.putFloat("lum_max", maximo);
        Serial.println("Preferencias salvas");
      }
    }
  }

  // Leitura e envio de comandos via monitor serial (opcional)
  if (Serial.available() > 0)
  {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    Serial1.println(comando);
  }

  // Enviar dados a cada 30 segundos
  if (millis() - lastSendTime >= sendInterval)
  {
    lastSendTime = millis();

    // Leitura dos sensores
    sensorBME.performReading();
    float temperatura = sensorBME.temperature + t_offset; // Calibração de temperatura
    float pressao = sensorBME.pressure / 100.0;      // Convertendo Pa para hPa
    Serial.println(pressao);
    float umidade = sensorBME.humidity + h_offset;

    // Leitura dos dados do kit meteorológico (vento e chuva)
    float windSpeed = weatherMeterKit.getWindSpeed();
    Serial.printf("Veloc_vento:%f\n", windSpeed);
    float totalRainfallMM = weatherMeterKit.getTotalRainfall();
    // Convert back to mm^3
    float totalWaterMM3 = totalRainfallMM * 5500.0;
    // Convert to mL (since 1 mL = 1000 mm^3)
    float totalWaterML = totalWaterMM3 / 1000.0;
    // Serial.print("Total Water (mL): ");
    // Serial.print(totalWaterML);

    valor = analogRead(windDirectionPin) * (5.0 / 4097); // Direção do vento (calibração)

    int leitura = analogRead(lightPin);
    int porcLuz = map(leitura, 0, 4095, 0, 100);
    int porcentagemLuz = map(porcLuz, 65, 100, 0, 100);
    if (porcentagemLuz < 0){
      porcentagemLuz = 0;
    }
    String Dir;
    // Serial.print("luz (%): ");
    // Serial.println(porcentagemLuz);

    // Calibrar as leituras de direção do vento
    if (valor <= 0.70)
    {
      Winddir = 270;
      Dir = "Oeste";
    }
    else if (valor <= 1.11)
    {
      Winddir = 315;
      Dir = "Noroeste";
    }
    else if (valor <= 1.50)
    {
      Winddir = 0;
      Dir = "Norte";
    }
    else if (valor <= 2.20)
    {
      Winddir = 225;
      Dir = "Sudoeste";
    }
    else if (valor <= 2.90)
    {
      Winddir = 45;
      Dir = "Nordeste";
    }
    else if (valor <= 3.75)
    {
      Winddir = 180;
      Dir = "Sul";
    }
    else if (valor <= 4.00)
    {
      Winddir = 135;
      Dir = "Sudeste";
    }
    else if (valor <= 4.25)
    {
      Winddir = 90;
      Dir = "Leste";
    }
    Serial.println("dir" + Dir);
    Serial.println("Luz: "+ String(leitura));
    Serial.println("Luz_perc: "+ String(porcentagemLuz));

    // leitura bateria
    float batteryVoltage = readBatteryVoltage();
    Serial.printf("batt_volt:%f\n", batteryVoltage);
    int batteryPercent = estimateBatteryPercentage(batteryVoltage);
    Serial.println("batt_volt_perc" + String(batteryPercent));

    // Adicionar dados dos sensores ao buffer CayenneLPP
    lpp.addTemperature(1, temperatura);    // Temperatura
    lpp.addRelativeHumidity(2, umidade);   // Umidade
    lpp.addBarometricPressure(3, pressao); // Pressão (em Pa x10)
    lpp.addAnalogInput(4, windSpeed);      // Velocidade do vento
    lpp.addAnalogInput(5, Winddir);        // Direção do vento
    lpp.addAnalogInput(6, totalWaterML);   // Chuva acumulada
    lpp.addLuminosity(7, porcentagemLuz);  // Luminosidade (valor analógico)
    lpp.addAnalogInput(8, batteryVoltage);
    lpp.addAnalogInput(9, batteryPercent);

    // Converte para HEX string
    String hexPayload = "";
    for (int i = 0; i < lpp.getSize(); i++)
    {
      if (lpp.getBuffer()[i] < 0x10)
        hexPayload += "0";
      hexPayload += String(lpp.getBuffer()[i], HEX);
    }

    debugLPPPayload(lpp.getBuffer(), lpp.getSize());

    lpp.reset();

    Serial.println("Payload HEX (CayenneLPP): " + hexPayload);
    Serial1.println("AT+SENDB=1:" + hexPayload); // Envia via LoRaWAN
  }
}
