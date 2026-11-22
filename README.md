# Firmware de Monitoramento Ambiental para Salas de Servidores (ESP32)

Repositório oficial do firmware e dos artefatos experimentais desenvolvidos para o Trabalho de Conclusão de Curso (TCC), referente a um sistema IoT de baixo custo para monitoramento contínuo de temperatura e umidade em pequenas salas de servidores e racks de TI.

---

## 1. Visão Geral do Sistema

O sistema utiliza um microcontrolador **ESP32** para coletar dados ambientais por meio de um sensor digital **DHT22**, exibindo as leituras localmente em um **Display LCD $16 \times 2$ (I2C)** e sinalizando estados operacionais (Normal, Alerta e Crítico, baseados nos limiares térmicos da norma ASHRAE) através de um **LED RGB**.

Em caso de elevação térmica:
- **Estado de Alerta ($T > 27,0\,^\circ\text{C}$):** Aciona o **módulo relé** para ligar o microventilador de exaustão auxiliar, altera o LED RGB para azul e envia notificação preventiva via **Telegram Bot**.
- **Estado Crítico ($T > 32,0\,^\circ\text{C}$):** Mantém a ventilação ativa, aciona o **buzzer** piezoelétrico para alarme sonoro local, altera o LED RGB para vermelho e dispara alerta crítico imediato via **Telegram Bot** (com controle de *cooldown* de $60\text{ s}$).

Toda a telemetria é transmitida de forma segura via protocolo **MQTTS (MQTT sobre TLS 1.2 na porta 8883)** para a plataforma em nuvem **ThingsBoard Cloud**.

---

## 2. Estrutura do Repositório

```text
firmware_TCC/
├── firmware_TCC.ino               # Código-fonte principal do firmware para ESP32
├── secrets_example.h              # Modelo de configuração de credenciais (versionado)
├── secrets.h                      # Arquivo local com credenciais reais (ignorado pelo Git)
├── .gitignore                     # Regras de exclusão de arquivos sensíveis e temporários
├── README.md                      # Documentação completa do projeto
├── docs/
│   └── figura2_esquema_eletrico.png # Esquema elétrico convencional do circuito (Figura 2 do TCC)
└── dados_experimentais/
    ├── dht22_vs_termohigrometro.csv # 100 leituras comparativas (DHT22 vs. instrumento de referência)
    └── rtt_mqtt_vs_mqtts.csv        # 100 medições de latência RTT comparando MQTT e MQTTS
```

*(Nota: Caso prefira organizar o código-fonte em uma subpasta `src/`, basta mover `firmware_TCC.ino` e `secrets_example.h` para `src/` mantendo `secrets.h` na mesma pasta do `.ino`.)*

---

## 3. Bibliotecas e Dependências

Para compilar o projeto na **Arduino IDE** (com o pacote *esp32 by Espressif Systems* instalado) ou **PlatformIO/VS Code**, instale as seguintes bibliotecas:

| Biblioteca | Autor / Referência | Versão Recomendada | Finalidade |
| :--- | :--- | :--- | :--- |
| **PubSubClient** | Nick O'Leary | `2.8.0` | Cliente MQTT para publicação de telemetria no ThingsBoard |
| **DHT sensor library** | Adafruit | `1.4.4` ou superior | Leitura digital de temperatura e umidade do sensor DHT22 |
| **Adafruit Unified Sensor** | Adafruit | `1.1.9` ou superior | Dependência base exigida pela biblioteca do sensor DHT |
| **LiquidCrystal_I2C** | Frank de Brabander / Marco Schwartz | `1.1.2` | Controle do display LCD $16 \times 2$ via barramento I2C |
| **WiFi** | Espressif (nativa core ESP32) | Integrada ao core | Conexão do ESP32 à rede sem fio local |
| **WiFiClientSecure** | Espressif (nativa core ESP32) | Integrada ao core | Comunicação criptografada TLS 1.2 (MQTTS e HTTPS Telegram) |
| **Wire** | Arduino / Espressif (nativa) | Integrada ao core | Comunicação serial síncrona I2C |

---

## 4. Tabela de Pinagem e Condicionamento de Sinais

O barramento I2C do display LCD opera originalmente em nível lógico de $5\text{ V}$, enquanto os pinos GPIO do ESP32 operam em $3,3\text{ V}$. Para proteção do microcontrolador e integridade de sinal, utiliza-se um **conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$)** (ou resistores de *pull-up* referenciados a $3,3\text{ V}$).

| Componente / Periférico | Pino do Módulo | Pino no ESP32 | Condicionamento / Observações Técnicas |
| :--- | :--- | :--- | :--- |
| **Sensor DHT22** | `DATA` | `GPIO 4` | Resistor de *pull-up* de $4,7\,\text{k}\Omega$ a $10\,\text{k}\Omega$ entre `DATA` e `3V3` |
| **Sensor DHT22** | `VCC` / `GND` | `3V3` / `GND` | Alimentação regulada em $3,3\text{ V}$ |
| **Módulo Relé (Ventilador)** | `IN` | `GPIO 16` | Acionamento da carga de $12\text{ V}$ (microventilador) |
| **Buzzer Piezoelétrico** | `+` (Sinal) / `-` | `GPIO 17` / `GND` | Sinal sonoro ativo no estado crítico ($> 32,0\,^\circ\text{C}$) |
| **LED RGB (Catodo Comum)** | `R` / `G` / `B` | `GPIO 18` / `19` / `23` | Resistores limitadores de corrente em série ($220\,\Omega$); catodo em `GND` |
| **Display LCD $16 \times 2$ I2C** | `SDA` | `GPIO 21` | Via conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$) |
| **Display LCD $16 \times 2$ I2C** | `SCL` | `GPIO 22` | Via conversor de nível lógico bidirecional ($5\text{ V} \leftrightarrow 3,3\text{ V}$) |
| **Display LCD $16 \times 2$ I2C** | `VCC` / `GND` | `5V (VIN)` / `GND` | Alimentação do circuito do display e *backlight* |

---

## 5. Configuração e Execução

### 5.1. Configuração das Credenciais (`secrets.h`)
Por boas práticas de segurança, nenhuma senha ou *token* real é armazenado no repositório público:
1. Na mesma pasta onde está localizado o arquivo `firmware_TCC.ino`, faça uma cópia do arquivo `secrets_example.h` e renomeie a cópia para `secrets.h`.
2. Abra o arquivo `secrets.h` e preencha suas credenciais reais:
   ```cpp
   #define WIFI_SSID "NOME_DA_SUA_REDE_WIFI"
   #define WIFI_PASSWORD "SENHA_DA_SUA_REDE_WIFI"

   #define TB_SERVER "thingsboard.cloud"
   #define TB_PORT 8883
   #define TB_ACCESS_TOKEN "SEU_ACCESS_TOKEN_THINGSBOARD"

   #define TELEGRAM_BOT_TOKEN "SEU_BOT_TOKEN_TELEGRAM"
   #define TELEGRAM_CHAT_ID "SEU_CHAT_ID_TELEGRAM"
   ```
3. Certifique-se de que `secrets.h` permanece listado no `.gitignore`.

### 5.2. Configuração no ThingsBoard Cloud
1. Acesse sua conta no [ThingsBoard Cloud](https://thingsboard.cloud/) e vá até **Entities > Devices**.
2. Clique em **+ (Add new device)**, atribua um nome ao dispositivo (ex.: `ESP32_ServerRoom_Monitor`) e salve.
3. Abra os detalhes do dispositivo criado, clique em **Copy access token** e cole o valor na constante `TB_ACCESS_TOKEN` do arquivo `secrets.h`.
4. Após carregar o firmware no ESP32, acompanhe as chaves `temperature` e `humidity` na aba **Latest telemetry** e vincule-as aos *widgets* do seu *Dashboard*.

### 5.3. Configuração do Bot de Alertas no Telegram
1. No aplicativo do Telegram, busque pelo contato oficial **`@BotFather`** e envie o comando `/newbot`.
2. Siga as instruções para definir o nome e o usuário do bot; ao final, copie o **HTTP API Token** gerado e preencha em `TELEGRAM_BOT_TOKEN`.
3. Inicie uma conversa com o bot recém-criado clicando em **Start** (ou envie qualquer mensagem para ele).
4. Para descobrir o seu `TELEGRAM_CHAT_ID`, acesse no navegador a URL `https://api.telegram.org/bot<SEU_BOT_TOKEN>/getUpdates` (ou utilize o bot `@userinfobot`) e copie o número do campo `"chat":{"id": ...}` para a constante `TELEGRAM_CHAT_ID`.

---

## 6. Documentação Técnica e Dados Experimentais

- **Esquema Elétrico:** Disponível em [`docs/figura2_esquema_eletrico.png`](docs/figura2_esquema_eletrico.png), correspondendo à Figura 2 da monografia.
- **Dados Brutos dos Experimentos:** Disponíveis na pasta [`dados_experimentais/`](dados_experimentais/), permitindo a auditoria e reprodução das análises estatísticas apresentadas nas tabelas de resultados do TCC.