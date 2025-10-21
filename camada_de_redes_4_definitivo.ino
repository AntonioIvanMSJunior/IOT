#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>  // <=== ADICIONADO para CallMeBot

// ======== DEFINIÇÃO DE PINOS ========
#define motor_esq 13
#define motor_dir 12
#define led_red 2
#define led_green 4
#define LDR_module 34
#define DHTPIN 25
#define PIR_sensor 33       // Sensor de presença PIR
#define DHTTYPE DHT11       // ou DHT22

// ======== OBJETOS ========
DHT dht(DHTPIN, DHTTYPE);
Servo servoEsq;
Servo servoDir;

// ======== CONFIG REDE/MQTT ========
const char* ssid = "xxxxxxxxxxx";    // ou sua rede local
const char* password = "xxxxxxxxxx";
const char* mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/robo/comando";
const char* mqtt_sensor_topic = "esp32/robo/sensores";

// ======== CONFIG CALLMEBOT ========
const String phoneNumber = "xxxxxxxxxxx";   // seu número com DDI
const String apiKey = "xxxxxxxxxxx";             // chave fornecida pelo CallMeBot

WiFiClient espClient;
PubSubClient client(espClient);

// ======== VARIÁVEIS ========
bool roboLigado = true;
unsigned long lastSensorTime = 0;
bool alertaEnviado = false;  // evita repetir envio constante

// ======== FUNÇÃO CALLMEBOT ========
void enviarMensagemWhatsApp(String mensagem) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    mensagem.replace(" ", "%20");  // codifica espaços na URL
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber +
                 "&text=" + mensagem + "&apikey=" + apiKey;

    Serial.println("Enviando mensagem via CallMeBot...");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      Serial.println("✅ Mensagem enviada com sucesso!");
    } else {
      Serial.print("⚠️ Erro ao enviar mensagem. Código: ");
      Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("⚠️ Wi-Fi não conectado. Não foi possível enviar a mensagem.");
  }
}

// ======== FUNÇÕES DE CONEXÃO ========
void setup_wifi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ======== FUNÇÕES DE CONTROLE ========
void pararMotores() {
  servoEsq.write(90);
  servoDir.write(90);
}
void frente() { servoEsq.write(180); servoDir.write(0); }
void tras() { servoEsq.write(0); servoDir.write(180); }
void esquerda() { servoEsq.write(90); servoDir.write(0); }
void direita() { servoEsq.write(180); servoDir.write(90); }

// ======== CALLBACK MQTT ========
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();

  Serial.print("Comando recebido: ");
  Serial.println(message);

  if (message == "DESLIGAR") {
    roboLigado = false;
    pararMotores();
    digitalWrite(led_green, LOW);
    digitalWrite(led_red, HIGH);
    Serial.println("Robô desligado remotamente.");
    return;
  }

  if (message == "LIGAR") {
    roboLigado = true;
    digitalWrite(led_green, HIGH);
    digitalWrite(led_red, LOW);
    Serial.println("Robô ligado novamente.");
    return;
  }

  if (!roboLigado) return;

  if (message == "Frente") frente();
  else if (message == "Trás") tras();
  else if (message == "Esquerda") esquerda();
  else if (message == "Direita") direita();
  else if (message == "Parado") pararMotores();
}

// ======== RECONEXÃO MQTT ========
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT... ");
    if (client.connect("ESP32RoboServo")) {
      Serial.println("Conectado!");
      client.subscribe(mqtt_topic);
      client.publish(mqtt_topic, "Robô servo conectado");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 3s");
      delay(3000);
    }
  }
}

// ======== SETUP ========
void setup() {
  Serial.begin(115200);

  pinMode(led_red, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(LDR_module, INPUT);
  pinMode(PIR_sensor, INPUT);

  dht.begin();
  servoEsq.attach(motor_esq);
  servoDir.attach(motor_dir);
  pararMotores();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  digitalWrite(led_green, HIGH);
  Serial.println("Robô operacional com sensores e servos SG90!");
}

// ======== LOOP ========
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  unsigned long currentTime = millis();
  if (currentTime - lastSensorTime > 2000) { // A cada 2 segundos
    lastSensorTime = currentTime;

    // ==== LEITURA DOS SENSORES ====
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    int pir = digitalRead(PIR_sensor); // 1 = presença detectada
    int luz_bruta = analogRead(LDR_module);
    int luz_invertida = 4095 - luz_bruta;
    int luz_percent = map(luz_invertida, 0, 4095, 0, 100);

    // Verificação DHT
    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Falha ao ler o DHT!");
      return;
    }

    // ==== CÁLCULO DA PROBABILIDADE DE VIDA ====
    int probabilidade = 0;

    if (temperatura >= 15 && temperatura <= 30) probabilidade += 25;
    if (umidade >= 40 && umidade <= 70) probabilidade += 25;
    if (luz_percent >= 50) probabilidade += 20; // luz adequada
    if (pir == 1) probabilidade += 30;          // presença detectada

    if (probabilidade > 100) probabilidade = 100;

    // ==== ESTADO DO ROBÔ ====
    String estadoRobo = roboLigado ? "Ligado" : "Desligado";
    if (!roboLigado) {
      digitalWrite(led_green, LOW);
      digitalWrite(led_red, HIGH);
    } else {
      if (probabilidade > 75) {
        digitalWrite(led_green, LOW);
        digitalWrite(led_red, HIGH);
        Serial.println("⚠️ ALERTA! Alta probabilidade de vida detectada!");
        if (!alertaEnviado) {
          enviarMensagemWhatsApp("Alerta! Alta probabilidade de vida detectada no planeta!");
          alertaEnviado = true;
        }
      } else {
        digitalWrite(led_green, HIGH);
        digitalWrite(led_red, LOW);
        Serial.println("Exploração normal. Nenhum indício relevante detectado.");
        alertaEnviado = false;
      }
    }

    // ==== MONITOR SERIAL ====
    Serial.println("--------------------------------------------------");
    Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
    Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
    Serial.print("Luz: "); Serial.print(luz_percent); Serial.println(" %");
    Serial.print("Presença: "); Serial.println(pir == 1 ? "Detectada" : "Ausente");
    Serial.print("Estado do robô: "); Serial.println(estadoRobo);
    Serial.print("Probabilidade de vida: "); Serial.print(probabilidade); Serial.println(" %");
    Serial.println("--------------------------------------------------\n");

    // ==== ENVIO MQTT ====
    char msg[200];
    snprintf(msg, sizeof(msg),
             "{\"temperatura\":%.1f,\"umidade\":%.1f,\"luz\":%d,\"presenca\":%d,\"probabilidade\":%d}",
             temperatura, umidade, luz_percent, pir, probabilidade);
    client.publish(mqtt_sensor_topic, msg);
  }
}
