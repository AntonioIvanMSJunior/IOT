#include "BluetoothSerial.h"
#include "DHT.h"

// --- Bluetooth ---
BluetoothSerial SerialBT;

// --- DHT11 ---
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- LED RGB ---
#define LED_R 13
#define LED_G 12
#define LED_B 14

// --- Fotorresistor ---
#define LDR_PIN 35

// --- Buzzer ---
#define BUZZER 21

// --- Botão ---
#define BOTAO 23

// --- Potenciômetro ---
#define POT_PIN 33

// --- Tipo de LED ---
// true = catodo comum, false = anodo comum
#define LED_COMUM_CATHODE true

// --- Controle de estados ---
bool tempAtiva = true;
bool detectorAtivo = true;
bool buzzerAtivo = true;
bool sistemaAtivo = true;

// --- Controle de LEDs via Bluetooth ---
bool btLedAtivo = false;
int btR = 0, btG = 0, btB = 0;

// --- Flag do botão ---
volatile bool botaoPressionado = false;

// --- Interrupção do botão ---
void IRAM_ATTR isrBotao() {
  botaoPressionado = true;
}

// --- Função auxiliar RGB ---
void setRGB(int r, int g, int b) {
  if(LED_COMUM_CATHODE) {
    digitalWrite(LED_R, r ? LOW : HIGH);
    digitalWrite(LED_G, g ? LOW : HIGH);
    digitalWrite(LED_B, b ? LOW : HIGH);
  } else { // anodo comum
    digitalWrite(LED_R, r ? HIGH : LOW);
    digitalWrite(LED_G, g ? HIGH : LOW);
    digitalWrite(LED_B, b ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32-Teste");
  dht.begin();

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BOTAO), isrBotao, FALLING);

  Serial.println("ESP32 Bluetooth + Sensores iniciado.");
}

void loop() {
  // --- Tratamento do botão ---
  if (botaoPressionado) {
    botaoPressionado = false;
    sistemaAtivo = !sistemaAtivo;
    if (!sistemaAtivo) {
      setRGB(0, 0, 0);
      digitalWrite(BUZZER, LOW);
      btLedAtivo = false;
      tempAtiva = false;
      detectorAtivo = false;
      buzzerAtivo = false;
      Serial.println("Sistema desligado pelo botão!");
    } else {
      tempAtiva = true;
      detectorAtivo = true;
      buzzerAtivo = true;
      Serial.println("Sistema reativado!");
    }
  }

  if (!sistemaAtivo) {
    delay(500);
    return;
  }

  // --- Temperatura ---
  float temp = dht.readTemperature();
  if (tempAtiva && !isnan(temp)) {
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.println(" °C");

    if ((temp < 0 || temp > 25) && buzzerAtivo) {
      digitalWrite(BUZZER, HIGH);
      Serial.println("Perigo! Buzzer ligado!");
    } else {
      digitalWrite(BUZZER, LOW);
    }
  }

  // --- Luminosidade ---
  int luz = analogRead(LDR_PIN);
  Serial.print("Luminosidade: ");
  Serial.println(luz);

  // --- Potenciômetro ---
  int pot = analogRead(POT_PIN);

  // --- Comando Bluetooth ---
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    Serial.print("Comando recebido: ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("Ligar")) sistemaAtivo = true;
    else if (cmd.equalsIgnoreCase("Desligar")) {
      sistemaAtivo = false;
      setRGB(0, 0, 0);
      digitalWrite(BUZZER, LOW);
      btLedAtivo = false;
    }
    else if (cmd.equalsIgnoreCase("Vermelho")) { btR = 1; btG = 0; btB = 0; btLedAtivo = true; }
    else if (cmd.equalsIgnoreCase("Amarelo"))  { btR = 1; btG = 1; btB = 0; btLedAtivo = true; }
    else if (cmd.equalsIgnoreCase("Azul"))     { btR = 0; btG = 0; btB = 1; btLedAtivo = true; }
    else if (cmd.equalsIgnoreCase("Desativar Temperatura")) tempAtiva = false;
    else if (cmd.equalsIgnoreCase("Ativar Temperatura")) tempAtiva = true;
    else if (cmd.equalsIgnoreCase("Desativar Detector")) detectorAtivo = false;
    else if (cmd.equalsIgnoreCase("Ativar Detector")) detectorAtivo = true;
    else if (cmd.equalsIgnoreCase("Desativar Buzzer")) { buzzerAtivo = false; digitalWrite(BUZZER, LOW); }
    else if (cmd.equalsIgnoreCase("Ativar Buzzer")) buzzerAtivo = true;
  }

  // --- Controle de LEDs ---
  if (btLedAtivo) {
    setRGB(btR, btG, btB);
  } 
  else if (detectorAtivo && luz < 1000) {
    setRGB(1, 1, 1); // Branco
  } 
  else { 
    if (pot < 1365) setRGB(1, 0, 0);      // vermelho
    else if (pot < 2730) setRGB(1, 1, 0); // amarelo
    else setRGB(0, 0, 1);                 // azul
  }

  delay(500);
}
