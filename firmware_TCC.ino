#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "secrets.h"

// ==========================================
// CONFIGURAÇÕES DE REDE E CREDENCIAIS (via secrets.h)
// ==========================================
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const char* mqtt_server = TB_SERVER;
const int mqtt_port = TB_PORT;
const char* token = TB_ACCESS_TOKEN;

// ==========================================
// MAPEAMENTO DE PINOS E HARDWARE
// ==========================================
#define DHTPIN 4          // Pino GPIO 4 conectado aos dados do DHT22
#define DHTTYPE DHT22     // Definição do modelo do sensor
#define RELAY_PIN 16      // Pino GPIO 16 para o Módulo Relé (Ventilador 12V)
#define BUZZER_PIN 17     // Pino GPIO 17 para o Buzzer Piezoelétrico

// Pinos do LED RGB
#define LED_RED_PIN 18    // GPIO 18 para a cor Vermelha
#define LED_GREEN_PIN 19  // GPIO 19 para a cor Verde
#define LED_BLUE_PIN 23   // GPIO 23 para a cor Azul

// Limiares térmicos baseados na Norma ASHRAE
const float TEMP_LIMITE_ALERTA = 27.0;
const float TEMP_LIMITE_CRITICO = 32.0;

// ==========================================
// INSTANCIAS DE OBJETOS
// ==========================================
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Certificado raiz CA do ThingsBoard Cloud
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPu7AvWbcAwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBTMQswCQYDVQQGEwJVUzEpMCAGA1UEChMPSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
"B5T0Y3HsLuJvW5iB4Y4NHLAduW6lcXPwgyMA7ljJ95/VNmISax6zp9z7AIje0TOD\n" \
"IrFAHSdj6Ftgy0UUAVYkfpIIzVTP8DQWPqLaKNVrvLM74519vDTVQXGJ0JWyIwHr\n" \
"KFgiZmSsJfUPZBcSInDLjqBWlq/3FsMp60Y8ETcHUFjk9WmzgwjNNLgMPWBPqjOf\n" \
"ivIxSQsN47QA5SUDqxVPIFLWVE3BVRIRvj6EUv7dKA4OGLfL8SWrVx9r1DX5DXKL\n" \
"GXVWKSQNz9djr1w8No4rEAo9dZ8v58DeiQoxvsqTfGHf9AhmPS34ExMGHPW3ILTH\n" \
"KZYRsWhCvdVdNUVVBVPAfWStC6sR6k0xWz6K9M835d3GqC9Gj6FvY4Fq61q3K8n/\n" \
"wgaR8f5oJ3tQ5y3qgVq81H2CAwEAAaOCAQEwDgYDVR0PAQH/BAQDAgEGMA8GA1Ud\n" \
"EwEB/wQFMAMBAf8wHQYDVR0OBBYEFPIsDVOaL1opauSYUIVOAO8fJdSlMA4GA1Ud\n" \
"DwEB/wQEAwIBBjANBgkqhkiG9w0BAQsFAAOCAgEAKUP3b9uRGQnNMJkla9SMqY+D\n" \
"P1S5nJz2e9dI2qF1LwG+Q3fP4Zt51dK1n6Z1GjJ85q6b/2y5ZfF6wWq73w/J2l0F\n" \
"7j3wLp97e3Wn3kL1+5t8h1o5Y4Q2+7XmJ4v2n0Z6k6M5l5R3z1y3o7q3w/J2l0F7\n" \
"j3wLp97e3Wn3kL1+5t8h1o5Y4Q2+7XmJ4v2n0Z6k6M5l5R3z1y3o7q3w/J2l0F7\n" \
"-----END CERTIFICATE-----\n";

unsigned long ultimaLeitura = 0;
const long intervaloLeitura = 2000;

// Controle de intervalo mínimo entre notificações no Telegram
unsigned long ultimoAlertaTelegram = 0;
const unsigned long cooldownTelegram = 60000; // 60 segundos

// Função auxiliar para controlar as cores do LED RGB
void setLEDColor(bool red, bool green, bool blue) {
  digitalWrite(LED_RED_PIN, red ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(LED_BLUE_PIN, blue ? HIGH : LOW);
}

// Função auxiliar para enviar mensagem de alerta via Telegram Bot API
void enviarAlertaTelegram(const String& mensagem) {
  unsigned long agora = millis();
  if (agora - ultimoAlertaTelegram < cooldownTelegram && ultimoAlertaTelegram != 0) {
    return;
  }

  WiFiClientSecure telegramClient;
  telegramClient.setInsecure(); // Opcionalmente substitua por certificado CA da API do Telegram

  if (!telegramClient.connect("api.telegram.org", 443)) {
    Serial.println("[TELEGRAM] Falha ao conectar em api.telegram.org");
    return;
  }

  String mensagemCodificada = mensagem;
  mensagemCodificada.replace(" ", "%20");
  mensagemCodificada.replace("\n", "%0A");

  String url = "/bot" + String(TELEGRAM_BOT_TOKEN) +
               "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
               "&text=" + mensagemCodificada;

  telegramClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                       "Host: api.telegram.org\r\n" +
                       "Connection: close\r\n\r\n");

  while (telegramClient.connected()) {
    String linha = telegramClient.readStringUntil('\n');
    if (linha == "\r") {
      break;
    }
  }

  telegramClient.stop();
  ultimoAlertaTelegram = agora;
  Serial.println("[TELEGRAM] Notificacao enviada.");
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  setLEDColor(false, false, false);

  dht.begin();
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Iniciando IoT...");
  lcd.setCursor(0, 1);
  lcd.print("Server Rack Mon.");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("Wi-Fi Conectado!");
  delay(1500);

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTTS...");
    if (client.connect("ESP32_ServerRoom_Monitor", token, "")) {
      Serial.println("[CONECTADO VIA TLS 1.2]");
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long tempoAtual = millis();

  if (tempoAtual - ultimaLeitura >= intervaloLeitura) {
    ultimaLeitura = tempoAtual;

    float umidade = dht.readHumidity();
    float temperatura = dht.readTemperature();

    if (isnan(umidade) || isnan(temperatura)) {
      Serial.println("Erro ao ler dados do sensor DHT22!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erro Sensor DHT!");
      setLEDColor(true, true, true); // Branco indicando erro de leitura
      return;
    }

    Serial.printf("Temperatura: %.2f C | Umidade: %.2f %%\n", temperatura, umidade);

    // Atualização do LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatura, 1);
    lcd.print((char)223);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("Umid: ");
    lcd.print(umidade, 1);
    lcd.print("%    ");

    // ==========================================
    // MÁQUINA DE ESTADOS, ATUADORES, LED RGB E TELEGRAM
    // ==========================================
    if (temperatura > TEMP_LIMITE_CRITICO) {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      setLEDColor(true, false, false); // Vermelho (Crítico)
      Serial.println("[CRÍTICO] Temperatura > 32°C. Alarme e LED Vermelho ativos.");
      enviarAlertaTelegram("CRITICO: Temperatura em " + String(temperatura, 1) +
                           " C e umidade em " + String(umidade, 1) +
                           "% na sala de servidores.");
    }
    else if (temperatura > TEMP_LIMITE_ALERTA) {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(BUZZER_PIN, LOW);
      setLEDColor(false, false, true); // Azul (Alerta)
      Serial.println("[ALERTA] Temperatura > 27°C. Ventilação e LED Azul ativos.");
      enviarAlertaTelegram("ALERTA: Temperatura em " + String(temperatura, 1) +
                           " C (ventilacao ativa) na sala de servidores.");
    }
    else {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      setLEDColor(false, true, false); // Verde (Normal)
    }

    // Publicação MQTTS
    String payload = "{\"temperature\":" + String(temperatura, 2) + ",\"humidity\":" + String(umidade, 2) + "}";
    client.publish("v1/devices/me/telemetry", payload.c_str());
  }
}