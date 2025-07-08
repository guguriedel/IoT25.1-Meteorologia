// COPIE E COLE AQUI O CÓDIGO DOS TESTES INICIAIS

#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
#include <Arduino.h>
#include <Adafruit_BME680.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <MQTT.h>
#include <ArduinoJson.h>

Adafruit_BME680 sensorBME;

// Pressão ao nível do mar em hPa
#define SEALEVELPRESSURE_HPA (1013.25)

int windDirectionPin = 4;
int windSpeedPin = 5;
int rainfallPin = 6;
int lightPin = 16;

int Winddir = 0; // Declara o valor inicial em 0
float valor = 0; // declara a variável inicial em 0

unsigned long instanteAnterior = 0;

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);

// Create an instance of the weather meter kit
SFEWeatherMeterKit weatherMeterKit(windDirectionPin, windSpeedPin, rainfallPin);

JsonDocument dados_tempo;

void reconectarWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin("Projeto", "2022-11-07");
    Serial.print("Conectando ao WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(1000);
    }
    Serial.print("conectado!\nEndereço IP: ");
    Serial.println(WiFi.localIP());
  }
}

void reconectarMQTT()
{
  if (!mqtt.connected())
  {
    Serial.print("Conectando MQTT...");
    while (!mqtt.connected())
    {
      mqtt.connect("LET-10", "aula", "zowmad-tavQez");
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" conectado!");

    mqtt.subscribe("tempo"); // qos = 0
  }
}

void recebeuMensagem(String topico, String conteudo)
{
  Serial.println(topico + ": " + conteudo);
}

void setup()
{
  // Begin serial
  Serial.begin(115200);
  delay(500);
  reconectarWiFi();

  conexaoSegura.setCACert(certificado1);
  mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura);
  mqtt.onMessage(recebeuMensagem);
  mqtt.setKeepAlive(10);

  reconectarMQTT();

  // Here we create a struct to hold all the calibration parameters
  SFEWeatherMeterKitCalibrationParams calibrationParams = weatherMeterKit.getCalibrationParams();

  // already calibrated

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

  // The rainfall detector contains a small cup that collects rain water. When
  // the cup fills, the water is dumped and the total rainfall is incremented
  // by some value. This value defaults to 0.2794mm of rain per count, as
  // specified by the datasheet
  calibrationParams.mmPerRainfallCount = 0.3636;

  // The rainfall detector switch can sometimes bounce, causing multiple extra
  // triggers. This input is debounced by ignoring extra triggers within a
  // time window, which defaults to 100ms
  calibrationParams.minMillisPerRainfall = 100;

  // The anemometer contains a switch that opens and closes as it spins. The
  // rate at which the switch closes depends on the wind speed. The datasheet
  // states that a wind of 2.4kph causes the switch to close once per second
  calibrationParams.kphPerCountPerSec = 2.4;

  // Because the anemometer generates discrete pulses as it rotates, it's not
  // possible to measure the wind speed exactly at any point in time. A filter
  // is implemented in the library that averages the wind speed over a certain
  // time period, which defaults to 1 second. Longer intervals result in more
  // accurate measurements, but cause delay in the measurement
  calibrationParams.windSpeedMeasurementPeriodMillis = 1000;

  // Now we can set all the calibration parameters at once
  weatherMeterKit.setCalibrationParams(calibrationParams);

  // Begin weather meter kit
  weatherMeterKit.begin();

  if (!sensorBME.begin())
  {
    Serial.println("Erro no sensor BME");
    while (true)
      ;
  }
  // aumenta amostragem dos sensores (1X, 2X, 4X, 8X, 16X ou NONE)
  sensorBME.setTemperatureOversampling(BME680_OS_8X);
  sensorBME.setHumidityOversampling(BME680_OS_2X);
  sensorBME.setPressureOversampling(BME680_OS_4X);
  sensorBME.setIIRFilterSize(BME680_FILTER_SIZE_3);
  sensorBME.setGasHeater(0, 0); // ˚C e ms, (0, 0) para desativar
}

void loop()
{
  reconectarWiFi();
  reconectarMQTT();
  mqtt.loop();

  // Print data from weather meter kit
  Serial.print(F("Wind direction (degrees): "));
  Serial.print(weatherMeterKit.getWindDirection(), 1);
  Serial.print(F("\t\t"));
  dados_tempo["direcao vento (degrees)"] = weatherMeterKit.getWindDirection();

  valor = analogRead(windDirectionPin) * (5.0 / 4097); // Calcula a tensão

  // Indica a posição oeste
  if (valor <= 0.70)
  {
    Winddir = 270;
    Serial.print("Oeste");
    dados_tempo["direcao_vento"] = "Oeste";
  }

  // Indica a posição noroeste
  else if (valor <= 1.11)
  {
    Winddir = 315;
    Serial.print("Noroeste");
    dados_tempo["direcao_vento"] = "Noroeste";
  }

  // Indica a posição norte
  else if (valor <= 1.50)
  {
    Winddir = 0;
    Serial.print("Norte");
    dados_tempo["direcao_vento"] = "Norte";
  }

  // Indica a posição sudoeste
  else if (valor <= 2.20)
  {
    Winddir = 225;
    Serial.print("Sudoeste");
    dados_tempo["direcao_vento"] = "Sudoeste";
  }

  // Indica a posição nordeste
  else if (valor <= 2.90)
  {
    Winddir = 45;
    Serial.print("Nordeste");
    dados_tempo["direcao_vento"] = "Nordeste";
  }

  // Indica a posição sul
  else if (valor <= 3.75)
  {
    Winddir = 180;
    Serial.print("Sul");
    dados_tempo["direcao_vento"] = "Sul";
  }

  // Indica a posição sudeste
  else if (valor <= 4.00)
  {
    Winddir = 135;
    Serial.print("Sudeste");
    dados_tempo["direcao_vento"] = "Sudeste";
  }

  // Indica a posição leste
  else if (valor <= 4.25)
  {
    Winddir = 90;
    Serial.print("Leste");
    dados_tempo["direcao_vento"] = "Leste";
  }

  // Caso nenhum valor seja compatível, imprime FAIL
  else
  {
    Winddir = 000;
    Serial.print("FAIL");
    dados_tempo["direcao_vento"] = "FAIL";
  }

  Serial.print(F("\t\t"));

  Serial.print(F("Wind speed (kph): "));
  Serial.print(weatherMeterKit.getWindSpeed(), 1);
  Serial.print(F("\t\t"));

  dados_tempo["vel vento (kph)"] = weatherMeterKit.getWindSpeed();

  Serial.print(F("Total rainfall (mm): "));
  Serial.println(weatherMeterKit.getTotalRainfall(), 1);

  float totalRainfallMM=weatherMeterKit.getTotalRainfall() ;
  // Convert back to mm^3
  float totalWaterMM3 = totalRainfallMM * 5500.0;
  // Convert to mL (since 1 mL = 1000 mm^3)
  float totalWaterML = totalWaterMM3 / 1000.0;
  Serial.print("Total Water (mL): ");
  Serial.print(totalWaterML);
  Serial.print(F("\t\t"));

  dados_tempo["tot chuva (mm)"] = weatherMeterKit.getTotalRainfall();

  sensorBME.performReading();
  float temperatura = sensorBME.temperature - 1.2; // ˚C
  Serial.print("Temperatura (˚C): ");
  Serial.print(temperatura);
  Serial.print(F("\t\t"));

  dados_tempo["temperatura (˚C)"] = temperatura;

  float pressao = sensorBME.pressure / 100.0; // hPa
  Serial.print("Pressao (hPa): ");
  Serial.print(pressao);
  Serial.print(F("\t\t"));

  dados_tempo["pressao (hPa)"] = pressao;

  float altitude = 44330.0 * (1.0 - pow(pressao / SEALEVELPRESSURE_HPA, 0.1903));
  Serial.print("altitude (m): ");
  Serial.print(altitude);
  Serial.print(F("\t\t"));

  dados_tempo["altitude (m)"] = altitude;

  float umidade = sensorBME.humidity; // %
  Serial.print("Umidade (%): ");
  Serial.print(umidade);
  Serial.print(F("\t\t"));

  dados_tempo["umidade (%)"] = umidade;

  int leitura = analogRead(lightPin);
  int porcentagemLuz = map(leitura, 0, 4095, 0, 100);
  porcentagemLuz = map(porcentagemLuz, 40, 100, 0, 100);
  Serial.print("luz (%): ");
  Serial.println(porcentagemLuz);

  dados_tempo["luz (%)"] = porcentagemLuz;

  String conteudo;
  serializeJson(dados_tempo, conteudo);

  unsigned long instanteAtual = millis();
  if (instanteAtual > instanteAnterior + 10000)
  {
    mqtt.publish("tempo", conteudo);
    instanteAnterior = instanteAtual;
  }

  // Only print once per second
  delay(1000);
}