# IoT25.1-Meteorologia

 Alunos:

Gustavo Maia Riedel - 2210375

Michela Sgrosso - 2421066

Luana Göbel - 2210872

# Estação Meteorológica Residencial — Solar + LoRaWAN

Um projeto enxuto de estação meteorológica autônoma: energia ≥ 100 % solar, bateria de lítio de reserva e telemetria LoRaWAN (classe A). O firmware principal está em **Codigo\_Final.cpp**.

---

## Hardware resumido

| Bloco       | Componentes usados                                                                                                                             |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| MCU + RF    | ESP32‑S3 dev‑board                                                                                               |
| Alimentação | Painel solar → diodo Schottky de bloqueio → *TP4096* (carregador) → 1× 18650 Li‑ion → *Duckboost XI6019* 5 V → VIN da placa                           |
| Sensores    | • *Wind vane* 10 k Ω  • Anemômetro cálice (reed)  • Pluviômetro basculante (reed)  • BME280 (T/H/P)  • Sensor de luminosidade (informe modelo) |

### Ligações rápidas (GPIO)

> Ajuste se necessário — consulte `Codigo_Final.cpp`

```
Vane        → GPIO 4   (ADC)
Anemômetro  → GPIO 5   (reed INT)
Pluviômetro → GPIO 6   (reed INT)
BME680      → I²C  SDA GPIO 8 / SCL GPIO 9  (padrão Wire no ESP32‑S3)
Lux sensor  → mesmo bus I²C
LoRa AT     → Serial1  TX GPIO 41 → RX módulo / RX GPIO 47 ← TX módulo
Batt sense  → GPIO 15  (ADC, divisor)
```


---

## Payload (Cayenne LPP)

| Canal | Métrica             | Unidade |
| :---: | ------------------- | ------- |
|    1  | Velocidade do vento | kph     |
|    2  | Direção do vento    | °       |
|    3  | Chuva acumulada     | mm      |
|    4  | Temperatura         | °C      |
|    5  | Umidade relativa    |  %      |
|    6  | Pressão             |  hPa    |
|    7  | Luminosidade        |  lux    |

---

## Grafana Link:
[Gráficos do Grafana](https://luanagobel.grafana.net/goto/Ypuqt8yHR?orgId=1 ) 

---



## Consumo de energia

• Deep sleep entre leituras ⇒ < 1 mA médio @ 3 V3.
•  Painel ≥ 5 W + 2 600 mAh garante > 5 dias sem sol.

---

## Próximos passos

* OTA via LoRaWAN
* Suporte a sensor UV

---

