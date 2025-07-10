# IoT25.1-Meteorologia

 Alunos:

Gustavo Maia Riedel - 2210375

Michela Sgrosso - 2421066

Luana Göbel - 2210872

# Estação Meteorológica Residencial — Solar + LoRaWAN

Um projeto enxuto de estação meteorológica autônoma: energia ≥ 100 % solar, bateria de lítio de reserva e telemetria LoRaWAN (classe A). O firmware principal está em **Codigo\_Final.cpp**.

A IoT resume‑se à ideia de conectar objetos físicos à Internet para coletar, transmitir e analisar dados em tempo real. Para demonstrar esses conceitos, construímos uma estação meteorológica residencial totalmente alimentada por energia solar que envia suas medições para a nuvem por LoRaWAN.

O objetivo é oferecer um dispositivo barato, autônomo e de longo alcance que qualquer morador possa instalar no telhado ou quintal. Ele monitora temperatura, umidade, pressão atmosférica, luminosidade, direção e velocidade do vento e volume de chuva. Esses dados permitem otimizar irrigação, prever tempestades, avaliar o rendimento de painéis solares ou simplesmente acompanhar o clima local com precisão.


---

## Hardware resumido

| Bloco       | Componentes usados                                                                                                                             |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| MCU + RF    | ESP32‑S3 dev‑board                                                                                               |
| Alimentação | Painel solar → diodo Schottky de bloqueio → *TP4096* (carregador) → 1× 18650 Li‑ion → *Duckboost XI6019* 5 V → VIN da placa                           |
| Sensores    | • *Wind vane* 10 k Ω  • Anemômetro   • Pluviômetro basculante   • BME280   • Sensor de luminosidade |

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

## Links Uteis:
[Gráficos do Grafana](https://luanagobel.grafana.net/goto/Ypuqt8yHR?orgId=1 ) 

[Vídeo no Youtube](https://youtu.be/U0asfO6R6Dc)

---



## Consumo de energia

Em operação, o ESP32‑S3 permanece em deep‑sleep consumindo cerca de 80 µA, com os
sensores mantendo mais 200 µA constantes. A cada 1 minuto ele acorda, lê todos os
sensores e dispara uma transmissão LoRa que atinge 120 mA por 40 milissegundos. São
1 440 envios por dia, o que, somando repouso e picos, chega a aproximadamente
≈ 18 mAh diários.


• Deep sleep entre leituras ⇒ < 1 mA médio @ 3 V3.
•  Painel ≥ 5 W + 2 600 mAh garante > algumas semanas sem sol.

---

## Próximos passos

* OTA via LoRaWAN
* Suporte a sensor UV

---

