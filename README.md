# Firmware de Monitoramento Ambiental para Salas de Servidores (ESP32)

Repositório oficial do firmware e dos dados experimentais desenvolvidos para o Trabalho de Conclusão de Curso (TCC) em Engenharia da Computação. O projeto consiste em um sistema embarcado de monitoramento contínuo de temperatura e umidade para pequenas salas de servidores e racks de TI, utilizando **ESP32**, sensor **DHT22**, atuadores e alertas locais (**módulo relé/microventilador**, **buzzer piezoelétrico**, **display LCD I2C** e **LED RGB**), telemetria segura via **MQTTS (TLS 1.2)** na plataforma **ThingsBoard Cloud** e notificações instantâneas por meio de um **Telegram Bot**.

---

## 1. Visão Geral do Sistema

O sistema realiza a aquisição periódica de temperatura e umidade relativa do ar e avalia os valores segundo faixas operacionais inspiradas nas diretrizes da norma **ASHRAE** para ambientes de TI:

- **Estado Normal ($\le 27,0\,^\circ\text{C}$):** Ventilação auxiliar desligada, alarme sonoro inativo e LED RGB indicando operação normal.
- **Estado de Alerta ($> 27,0\,^\circ\text{C}$ e $\le 32,0\,^\circ\text{C}$):** Acionamento automático do relé do microventilador de exaustão, sinalização visual correspondente no LED RGB e disparo de alerta remoto via Telegram.
- **Estado Crítico ($> 32,0\,^\circ\text{C}$):** Manutenção da ventilação forçada, ativação do alarme sonoro (*buzzer*), alerta visual crítico no LED RGB e notificação remota imediata via Telegram (com controle de *cooldown* de 60 segundos para evitar *spam* de mensagens).

Simultaneamente, todas as leituras são exibidas em tempo real no display LCD $16 \times 2$ e enviadas em formato JSON via **MQTTS** para o **ThingsBoard Cloud**.

---

## 2. Estrutura do Repositório

```text
firmware_TCC/
├── firmware_TCC.ino            # Código-fonte principal do firmware para o ESP32
├── secrets_example.h           # Modelo versionado com placeholders para credenciais
├── secrets.h                   # Arquivo local com credenciais reais (ignorado pelo Git)
├── .gitignore                  # Regras para impedir o envio de segredos ao repositório
├── README.md                   # Documentação técnica e guia de replicação do projeto
├── docs/
│   └── figura2_esquema_eletrico.png   # Esquema elétrico convencional (Figura 2 do TCC)
└── dados_experimentais/
    ├── dht22_vs_termohigrometro.csv   # 100 leituras comparativas (DHT22 vs. referência)
    └── rtt_mqtt_vs_mqtts.csv          # 100 tempos de RTT por protocolo (MQTT vs. MQTTS)
```

---

## 3. Bibliotecas e Dependências

Para compilar o firmware na **Arduino IDE** ou **VS Code (PlatformIO / Arduino CLI)**, utilize o core oficial da **Espressif ESP32** e instale as seguintes bibliotecas:

- **`PubSubClient`** (Nick O'Leary) — Cliente MQTT utilizado para publicação de telemetria no ThingsBoard.
- **`DHT sensor library`** (Adafruit) — Leitura do sensor digital de temperatura e umidade DHT22.
- **`Adafruit Unified Sensor`** (Adafruit) — Dependência base exigida pela biblioteca do sensor DHT.
- **`LiquidCrystal_I2C`** (Frank de Brabander / Marco Schwartz) — Controle do display LCD $16 \times 2$ via barramento I2C.
- **`WiFi.h`**, **`WiFiClientSecure.h`** e **`Wire.h`** — Bibliotecas nativas do pacote ESP32 para conexão Wi-Fi, comunicação TLS/SSL (MQTTS e API HTTPS do Telegram) e interface I2C.

---

## 4. Tabela de Pinagem e Condicionamento de Sinais

A tabela abaixo detalha todas as interligações elétricas entre o microcontrolador ESP32, os periféricos e as proteções de nível lógico:

| Componente | Pino do Componente | Pino Correspondente no ESP32 / Alimentação | Função / Descrição da Ligação |
| :--- | :--- | :--- | :--- |
| **Sensor DHT22** | Pino 1 (VCC) | Pino `3V3` do ESP32 | Alimentação positiva ($3,3\text{ V}$) |
| **Sensor DHT22** | Pino 2 (DATA) | Pino `GPIO 4` | Barramento de dados (exige resistor de *pull-up*) |
| **Sensor DHT22** | Pino 3 (NC) | Não conectado | Pino livre / sem função |
| **Sensor DHT22** | Pino 4 (GND) | Pino `GND` do ESP32 | Referência terra comum |
| **Módulo Relé (5V)** | `VCC` | Pino `VIN` ou `5V` do ESP32 | Alimentação lógica do módulo relé |
| **Módulo Relé (5V)** | `GND` | Pino `GND` do ESP32 | Referência terra comum |
| **Módulo Relé (5V)** | `IN` (Entrada) | Pino `GPIO 16` | Sinal lógico de comando (Nível Alto ativa o relé) |
| **Buzzer Piezoelétrico** | Fio Positivo (`+`) | Pino `GPIO 17` | Sinal de acionamento do alarme sonoro |
| **Buzzer Piezoelétrico** | Fio Negativo (`-`) | Pino `GND` do ESP32 | Referência terra comum |
| **Display LCD I2C** | `VCC` | Pino `5V` ou `VIN` do ESP32 | Alimentação positiva do display |
| **Display LCD I2C** | `GND` | Pino `GND` do ESP32 | Referência terra comum |
| **Display LCD I2C** | `SDA` | Pino `GPIO 21` do ESP32 | Conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$) ou *pull-up* em $3,3\text{ V}$ para proteção das entradas digitais |
| **Display LCD I2C** | `SCL` | Pino `GPIO 22` do ESP32 | Conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$) ou *pull-up* em $3,3\text{ V}$ para proteção das entradas digitais |
| **LED RGB (Status)** | Anodo Comum / `VCC` | Pino `3V3` do ESP32 | Alimentação positiva (com resistores limitadores) |
| **LED RGB (Status)** | Pino Vermelho (Red) | Pino `GPIO 18` | Controle do canal vermelho (alerta térmico) |
| **LED RGB (Status)** | Pino Verde (Green) | Pino `GPIO 19` | Controle do canal verde (estado normal) |
| **LED RGB (Status)** | Pino Azul (Blue) | Pino `GPIO 23` | Controle do canal azul (diagnóstico/conexão) |

> **Nota sobre lógica de acionamento do LED RGB:** Como o LED RGB utiliza **Anodo Comum** ligado ao pino `3V3` (com resistores limitadores de corrente em série), cada canal é acionado em **nível lógico baixo (`LOW`)** para acender e colocado em **nível lógico alto (`HIGH`)** para apagar. Certifique-se de que a função `setLEDColor()` no firmware reflita essa polaridade caso utilize um módulo de anodo comum.

---

## 5. Configuração e Execução

### 5.1. Configuração de Credenciais (`secrets.h`)
1. Na mesma pasta onde está o arquivo `firmware_TCC.ino`, faça uma cópia do arquivo `secrets_example.h` e renomeie a cópia para **`secrets.h`**.
2. Abra o `secrets.h` e preencha suas credenciais reais:
   - `WIFI_SSID` e `WIFI_PASSWORD` (rede Wi-Fi local);
   - `TB_SERVER`, `TB_PORT` (`8883`) e `TB_ACCESS_TOKEN` (ThingsBoard Cloud);
   - `TELEGRAM_BOT_TOKEN` e `TELEGRAM_CHAT_ID` (Telegram Bot).
3. O arquivo `secrets.h` já está incluído no `.gitignore` para garantir que senhas e *tokens* privados nunca sejam enviados ao GitHub.

### 5.2. Configuração no ThingsBoard Cloud
1. Crie uma conta ou acesse seu painel no **ThingsBoard Cloud** (`thingsboard.cloud`).
2. Vá em **Entities > Devices** e clique em **+ Add new device**.
3. Dê um nome ao dispositivo (ex.: `ESP32_ServerRoom_Monitor`) e finalize a criação.
4. Abra os detalhes do dispositivo criado, clique em **Copy access token** e cole o valor na constante `TB_ACCESS_TOKEN` do seu arquivo `secrets.h`.
5. Após ligar o ESP32, verifique a chegada das chaves `temperature` e `humidity` na aba **Latest telemetry** e vincule-as aos *widgets* do seu *dashboard*.

### 5.3. Configuração do Bot no Telegram
1. No aplicativo do Telegram, busque pelo contato oficial **`@BotFather`** e inicie uma conversa.
2. Envie o comando `/newbot`, siga as instruções para definir o nome e o usuário do bot e copie o **HTTP API Token** gerado para `TELEGRAM_BOT_TOKEN` no arquivo `secrets.h`.
3. Inicie uma conversa com o seu novo bot (clicando em **Start** ou enviando um `"Oi"`).
4. Acesse no navegador `https://api.telegram.org/bot<SEU_BOT_TOKEN>/getUpdates`, localize o campo `"chat":{"id":...}` e copie o número correspondente para `TELEGRAM_CHAT_ID` no arquivo `secrets.h`.

---

## 6. Documentação Complementar e Dados Experimentais

- **Esquema Elétrico:** Disponível em [`docs/figura2_esquema_eletrico.png`](docs/figura2_esquema_eletrico.png), correspondente à Figura 2 da monografia.
- **Dados Experimentais:** Disponíveis na pasta [`dados_experimentais/`](dados_experimentais/), contendo os registros brutos das 100 leituras comparativas entre o DHT22 e o termohigrômetro de referência (`dht22_vs_termohigrometro.csv`) e das 100 medições de tempo de ida e volta (RTT) comparando MQTT e MQTTS (`rtt_mqtt_vs_mqtts.csv`).