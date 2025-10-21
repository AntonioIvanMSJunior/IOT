# 🤖 Robô Explorador Espacial – Camada de Serviço (AV2)

## 🧠 Descrição do Projeto
Este projeto faz parte da prática **“Camada de Serviço”** da disciplina *Práticas Integradas*.  
O objetivo é desenvolver um **robô explorador espacial**, controlado remotamente via ESP32, capaz de:
- Coletar dados de sensores (temperatura, luminosidade, presença);
- Calcular a **probabilidade de vida extraterrestre** com base nesses dados;
- Armazenar as leituras em um banco de dados via API Python (Flask ou FastAPI);
- Enviar alertas via WhatsApp usando o **Callmebot** quando forem detectadas condições favoráveis à vida.

---

## 🚀 Estrutura do Projeto
O projeto está dividido em **5 etapas principais**:

### 🧩 Etapa 01 – Controle Remoto no Wokwi
**Componentes:**
- ESP32 (simulado)
- Joystick analógico (eixos X e Y)
- LEDs (verde e vermelho)
- Botão de desligar

**Funcionalidades:**
- O joystick envia comandos de direção (“Frente”, “Trás”, “Esquerda”, “Direita”) para o robô.
- O botão envia o comando `DESLIGAR`.
- LED verde indica conexão ativa; LED vermelho indica robô desligado.

**Como testar no Wokwi:**
1. Acesse [https://wokwi.com](https://wokwi.com)
2. Faça login e importe o projeto usando o link abaixo:
https://wokwi.com/projects/445162604154928129
3. Execute a simulação e verifique as mensagens no monitor serial.

---

### ⚙️ Etapa 02 – Robô Físico (Laboratório)
**Componentes:**
- ESP32 físico  
- 2 motores DC  
- Sensor DHT (temperatura e umidade)  
- Fotorresistor (luminosidade)  
- Sensor PIR (presença)  
- LEDs verde e vermelho  

**Funcionalidades:**
- Leitura dos sensores e envio periódico (a cada 2 s) via serial.
- Cálculo da **probabilidade de vida**:
- Temperatura entre 15–30 ºC → +25%
- Umidade entre 40–70% → +25%
- Luz acima do limite → +20%
- Presença detectada → +30%
- Se a probabilidade > 75%:
- LED vermelho aceso  
- Mensagem via **Callmebot (WhatsApp)**:  
 “⚠️ Alerta! Alta probabilidade de vida detectada no planeta.”

---

### 💾 Etapa 03 – Armazenamento de Dados
**Backend em Python (Flask ou FastAPI):**
Crie uma API REST com endpoints:
- `POST /leituras` → Recebe e armazena dados no banco SQLite.
- `GET /leituras` → Retorna as últimas 100 leituras.

**Exemplo de registro enviado:**
```json
{
"timestamp": "2025-09-02T14:35:00Z",
"temperatura_c": 24.3,
"umidade_pct": 55,
"luminosidade": 723,
"presenca": 1,
"probabilidade_vida": 78.0
}
