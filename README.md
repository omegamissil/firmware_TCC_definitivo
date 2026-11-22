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
    ├── rtt_mqtt.csv                          # 100 amostras de RTT em MQTT plano - porta 1883 (Tabela 2)
    └── rtt_mqtts.csv                         # 100 amostras de RTT em MQTTS/TLS 1.2 - porta 8883 (Tabela 2)