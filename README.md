# Desenvolvimento de um Sistema IoT Embarcado para Monitoramento Ambiental e Telemetria em Nuvem com Alertas Automatizados

Repositório oficial do firmware, esquemático e dados experimentais do Trabalho de Conclusão de Curso (TCC) apresentado ao curso de **Engenharia da Computação** da **Universidade Veiga de Almeida (UVA)**.

- **Autor:** Pedro Campos Ferreira dos Santos
- **Orientador:** Prof. Vitor Amadeu de Souza
- **Ano:** 2026

---

## 1. Visão Geral do Sistema

O projeto consiste em um sistema embarcado de baixo custo para monitoramento contínuo de temperatura e umidade relativa em pequenas salas de servidores e armários de TI (CPDs), baseado nos envelopes operacionais da norma **ASHRAE Thermal Guidelines (Class A1/A2)** e nos princípios de confidencialidade e integridade da **ISO/IEC 27001**:

- **Camada de Borda (Edge):** Microcontrolador **ESP32** realizando aquisições não bloqueantes (via `millis()`, a cada 2 segundos) do sensor digital **DHT22**, exibição local em **Display LCD 16x2 I2C**, sinalização visual via **LED RGB**, acionamento automático de refrigeração auxiliar (**módulo relé 5 V + microventilador 12 V DC**) e alarme sonoro (**buzzer piezoelétrico**).
- **Camada de Transporte Seguro:** Comunicação criptografada via protocolo **MQTTS (MQTT sobre TLS 1.2)** na porta `8883` com validação de certificado digital X.509.
- **Camada de Nuvem e Alertas:** Persistência de séries temporais e visualização em tempo real no **ThingsBoard Cloud**, com disparo automatizado de notificações push via **Telegram Bot API** através do Motor de Regras (*Rule Engine*) e redundância no firmware.

### Lógica de Controle e Máquina de Estados (Norma ASHRAE)

| Estado Operacional | Condição Térmica | Atuadores Locais e Sinalização | Ação Remota / Nuvem |
| :--- | :--- | :--- | :--- |
| **Normal** | $T \le 27,0\,^\circ\text{C}$ | Relé desligado, *buzzer* desligado, **LED Verde** | Telemetria contínua via MQTTS |
| **Alerta** | $27,0\,^\circ\text{C} < T \le 32,0\,^\circ\text{C}$ | **Relé ativado** (ventilação auxiliar 12 V DC), *buzzer* desligado, **LED Azul** | Telemetria contínua via MQTTS |
| **Crítico** | $T > 32,0\,^\circ\text{C}$ | **Relé ativado**, **buzzer ativado**, **LED Vermelho** | Disparo imediato de alerta via **Telegram Bot API** |

---

## 2. Estrutura do Repositório

```text
firmware_TCC_definitivo/
├── firmware_TCC.ino                          # Código-fonte principal em C++ (ESP32)
├── secrets_example.h                         # Modelo de configuração de credenciais (versionado)
├── secrets.h                                 # Credenciais reais locais (ignorado pelo Git via .gitignore)
├── .gitignore                                # Regras de proteção de arquivos sensíveis
├── README.md                                 # Documentação técnica do projeto
├── docs/
│   └── figura2_esquema_eletrico.png          # Diagrama esquemático do circuito (Figura 2 do TCC)
└── dados_experimentais/
    ├── dht22_vs_termohigrometro.csv          # 100 amostras comparativas de acurácia (Tabela 1)
    └── rtt_mqtt_vs_mqtts.csv                 # 100 amostras de RTT por protocolo: MQTT vs. MQTTS (Tabela 2)
```

---

## 3. Bibliotecas e Dependências

Para compilar o firmware na **Arduino IDE** (com o pacote *ESP32 by Espressif Systems* instalado) ou no **PlatformIO / VS Code**, utilize as seguintes bibliotecas:

- **`PubSubClient`** (Nick O'Leary) — Cliente MQTT para publicação de telemetria JSON no tópico `v1/devices/me/telemetry`.
- **`DHT sensor library`** + **`Adafruit Unified Sensor`** (Adafruit) — Leitura digital do sensor DHT22 (AM2302).
- **`LiquidCrystal_I2C`** (Frank de Brabander / Marco Schwartz) — Controle do display LCD 16x2 via adaptador I2C (PCF8574).
- **`WiFi.h`**, **`WiFiClientSecure.h`** e **`Wire.h`** — Bibliotecas nativas do core ESP32 para conexão sem fio, handshake TLS 1.2 com certificado X.509 e barramento I2C.

---

## 4. Tabela de Pinagem e Condicionamento de Sinais

A tabela abaixo corresponde integralmente ao mapeamento físico do protótipo apresentado na Seção 4.1 da monografia:

| Componente | Pino do Componente | Pino Correspondente no ESP32 / Alimentação | Função / Descrição da Ligação |
| :--- | :--- | :--- | :--- |
| **Sensor DHT22** | Pino 1 (`VCC`) | Pino `3V3` do ESP32 | Alimentação positiva (`3,3 V`) |
| **Sensor DHT22** | Pino 2 (`DATA`) | Pino `GPIO 4` | Barramento de dados (com resistor de *pull-up* externo de $4,7\text{ k}\Omega$ entre `3V3` e `DATA`) |
| **Sensor DHT22** | Pino 3 (`NC`) | Não conectado | Pino livre / sem função |
| **Sensor DHT22** | Pino 4 (`GND`) | Pino `GND` do ESP32 | Referência terra comum |
| **Módulo Relé (5 V)** | `VCC` | Pino `VIN` ou `5V` do ESP32 | Alimentação lógica do módulo relé optoacoplado |
| **Módulo Relé (5 V)** | `GND` | Pino `GND` do ESP32 | Referência terra comum |
| **Módulo Relé (5 V)** | `IN` (Entrada) | Pino `GPIO 16` | Sinal lógico de comando (Nível Alto ativa o relé para chavear o microventilador de 12 V DC) |
| **Buzzer Piezoelétrico** | Fio Positivo (`+`) | Pino `GPIO 17` | Sinal de acionamento do alarme sonoro |
| **Buzzer Piezoelétrico** | Fio Negativo (`-`) | Pino `GND` do ESP32 | Referência terra comum |
| **Display LCD I2C** | `VCC` | Pino `5V` ou `VIN` do ESP32 | Alimentação positiva do display 16x2 |
| **Display LCD I2C** | `GND` | Pino `GND` do ESP32 | Referência terra comum |
| **Display LCD I2C** | `SDA` | Pino `GPIO 21` do ESP32 | Conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$) ou *pull-up* em $3,3\text{ V}$ para proteção das entradas digitais |
| **Display LCD I2C** | `SCL` | Pino `GPIO 22` do ESP32 | Conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$) ou *pull-up* em $3,3\text{ V}$ para proteção das entradas digitais |
| **LED RGB (Status)** | Anodo Comum / `VCC` | Pino `3V3` do ESP32 | Alimentação positiva (com resistores limitadores de corrente) |
| **LED RGB (Status)** | Pino Vermelho (`Red`) | Pino `GPIO 18` | Controle do canal vermelho (alerta térmico crítico) |
| **LED RGB (Status)** | Pino Verde (`Green`) | Pino `GPIO 19` | Controle do canal verde (estado normal) |
| **LED RGB (Status)** | Pino Azul (`Blue`) | Pino `GPIO 23` | Controle do canal azul (estado de alerta / refrigeração ativa) |

> **Nota de hardware:** Como o LED RGB utiliza configuração de **anodo comum** conectado ao pino `3V3`, o acionamento de cada canal de cor no firmware ocorre em nível lógico baixo (`LOW`) quando configurado nesse modo, ou nível lógico alto (`HIGH`) em montagens de catodo comum.

---

## 5. Configuração e Execução

### 5.1. Configuração Local de Credenciais (`secrets.h`)
1. Clone este repositório e acesse a pasta do projeto.
2. Copie o arquivo `secrets_example.h` e renomeie a cópia para `secrets.h`.
3. Preencha suas credenciais reais no `secrets.h`:
   - `WIFI_SSID` e `WIFI_PASSWORD` (rede Wi-Fi 2,4 GHz);
   - `TB_SERVER` (`"thingsboard.cloud"`), `TB_PORT` (`8883`) e `TB_ACCESS_TOKEN`;
   - `TELEGRAM_BOT_TOKEN` e `TELEGRAM_CHAT_ID`.
4. O arquivo `secrets.h` está bloqueado no `.gitignore` para impedir o envio acidental de credenciais ao GitHub.

### 5.2. Configuração da Plataforma ThingsBoard Cloud
1. Acesse o painel do **ThingsBoard Cloud** e crie um novo dispositivo (*Devices > Add new device*) com perfil MQTT padrão.
2. Copie o **Access Token** gerado e insira-o em `TB_ACCESS_TOKEN` no arquivo `secrets.h`.
3. Crie um *Dashboard* com widgets (*gauges* e gráficos de séries temporais) vinculados às chaves de telemetria `temperature` e `humidity`.
4. No **Rule Engine** (*Root Rule Chain*), adicione um nó **Filter Script** verificando `msg.temperature > 32.0` conectado a um nó **REST API Call** (`POST` para `https://api.telegram.org/bot<TOKEN>/sendMessage`) para envio de notificações push.

### 5.3. Configuração do Bot no Telegram
1. No aplicativo Telegram, inicie uma conversa com o **`@BotFather`** e envie `/newbot` para criar o bot e obter o **Bot Token**.
2. Envie uma mensagem inicial ao bot criado (ou adicione-o ao grupo de suporte de TI) e consulte `https://api.telegram.org/bot<SEU_TOKEN>/getUpdates` para identificar o **`chat_id`**.

---

## 6. Síntese dos Resultados Experimentais (Seção 4.4 do TCC)

Os registros completos dos 100 ciclos de ensaio estão disponíveis na pasta `dados_experimentais/` e sustentam os resultados apresentados na monografia:

- **Acurácia do Sensor DHT22 vs. Termohigrômetro Calibrado Testo 625 (Tabela 1):** Erro Absoluto Médio (MAE) de **$0,35\,^\circ\text{C}$** ($\sigma = \pm 0,12\,^\circ\text{C}$) para temperatura e **$1,30\%$** ($\sigma = \pm 0,45\%$) para umidade relativa, dentro das tolerâncias do fabricante.
- **Latência de Rede — MQTT vs. MQTTS (Tabela 2):** RTT médio de **$48,2\text{ ms}$** ($\sigma = \pm 14,3\text{ ms}$) no MQTT plano (porta `1883`) contra **$135,6\text{ ms}$** ($\sigma = \pm 38,1\text{ ms}$) no MQTTS com TLS 1.2 (porta `8883`), representando um *overhead* médio de $87,4\text{ ms}$.
- **Tempos de Resposta Dinâmica (Tabela 3):** Tempo médio de acionamento local do relé ($T_{\text{resposta}}$) de **$185\text{ ms}$** após detecção de $T > 27,0\,^\circ\text{C}$ e tempo médio de entrega do alerta no Telegram ($T_{\text{alerta}}$) de **$1,42\text{ s}$** após gatilho em nuvem para $T > 32,0\,^\circ\text{C}$.