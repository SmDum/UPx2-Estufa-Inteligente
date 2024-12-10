#include <DHT.h>
#include <Wire.h>
#include <HCSR04.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Informações do WiFi e URLs da API
const char* ssid = "Redmi10A";
const char* password = "joseluiz00";
const char* apiUrlGetConfig = "http://192.168.236.51:5501/api/GreenHouseConfig/GetAll";
const char* apiUrlGetById = "http://192.168.236.51:5501/api/GreenHouseConfig/GetById/"; // URL base para buscar por ID

// Variáveis globais para armazenar os valores obtidos
float TEMP_MIN, TEMP_MAX, UMIDADE_SOLO_LIMIAR, UMIDADE_SOLO_MIN, NIVEL_AGUA_MIN;
int selectedId = -1; // ID selecionado pelo usuário

// Definições de pinos e constantes
#define DHTPIN 33
#define DHTTYPE DHT22
#define CAPACITOR 32

#define VENTOINHA 2

#define BOMBA1 4
#define BOMBA2 0

#define CAP_MIN 2243
#define CAP_MAX 943
// Define os valores mínimos e máximos calibrados do sensor capacitivo

// Inicializa o sensor ultrassônico nos pinos TRIG e ECHO
UltraSonicDistanceSensor distanceSensor(14, 12);  // TRIG = 14, ECHO = 12;
DHT dht(DHTPIN, DHTTYPE); // Inicializa o DHT22

// Função para iniciar o hardware
void iniciarHardware() 
{ // Inicializa a placa de aquecimento
  pinMode(VENTOINHA, OUTPUT);   // Inicializa a ventoinha

  pinMode(BOMBA1, OUTPUT);      // Inicializa a bomba 1
  pinMode(BOMBA2, OUTPUT);      // Inicializa a bomba 2

  pinMode(CAPACITOR, INPUT);    // Inicializa o sensor de umidade do solo
  pinMode(DHTPIN, INPUT);       // Garante que o pino do DHT seja entrada
  
  dht.begin();                  // Inicializa o sensor de umidade do ar

  Serial.begin(115200);         // Inicializa o monitor serial

}

void iniciarRequisicao()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  Serial.println("Conectado ao WiFi!");


  // 1. Requisição ao GetConfig com um ID específico
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(apiUrlGetConfig);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String response = http.getString();
      Serial.println("Resposta do GetAll: " + response);

      // Processar JSON para listar as opções (simulação)
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);

      // Simula escolha de um ID baseado na resposta
      JsonArray configs = doc.as<JsonArray>();
      if (configs.size() > 0) {
        selectedId = configs[0]["id"]; // Aqui você poderia substituir pela escolha do usuário
        Serial.print("ID Selecionado: "); Serial.println(selectedId);
      }
    } else {
      Serial.println("Erro ao obter dados do GetConfig.");
    }

    http.end();
  }

  // 2. Requisição ao GetById usando o ID selecionado
}

// Função para leitura dos sensores
void lerSensores(float &temperatura, float &umidade, float &umidadeSolo, float &waterLevel) 
{
  temperatura = dht.readTemperature(); // Lê a temperatura
  umidade = dht.readHumidity();        // Lê a umidade

  umidadeSolo = analogRead(CAPACITOR); // Lê a umidade do solo

  umidadeSolo = ((CAP_MIN - umidadeSolo) * 100 / (CAP_MIN - CAP_MAX));  // Converte para porcentagem

  if(umidadeSolo<0)
  {
    umidadeSolo = 0;
  }

  waterLevel = 20 - distanceSensor.measureDistanceCm(); // Lê o nível de água
  if (waterLevel < 0) {
    waterLevel = 0;  // Define um valor padrão seguro
  }
}

// Função para exibição dos dados no Serial
void exibirDadosSerial(float temperatura, float umidade, float umidadeSolo, float waterLevel) 
{
  Serial.println("===== Dados dos Sensores =====");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
  Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
  Serial.print("Umidade do Solo: "); Serial.print(umidadeSolo); Serial.println(" %");
  Serial.print("Nível de Água: "); Serial.print(waterLevel); Serial.println(" cm");
  Serial.println("===============================");
  Serial.println();
}

// Função para controle de temperatura
void controlarTemperatura(float temperatura) 
{
  if (temperatura < TEMP_MIN) {
    digitalWrite(VENTOINHA, HIGH);    // Desliga a ventoinha
  } 
  else if (temperatura > TEMP_MAX) {
    digitalWrite(VENTOINHA, LOW);   // Liga a ventoinha
  } 
  else {
    digitalWrite(VENTOINHA, HIGH);    // Desliga a ventoinha
  }
}

// Função para controle de irrigação
void controlarIrrigacao(float umidadeSolo, float waterLevel) 
{
  if (umidadeSolo < UMIDADE_SOLO_MIN && waterLevel >= NIVEL_AGUA_MIN) {
    digitalWrite(BOMBA2, LOW); // Liga a bomba de irrigação
  } else {
    digitalWrite(BOMBA2, HIGH);  // Desliga a bomba de irrigação
  }

  if (waterLevel < NIVEL_AGUA_MIN) {
    digitalWrite(BOMBA1, LOW); // Liga a bomba de reposição de água
  } else {
    digitalWrite(BOMBA1, HIGH);  // Desliga a bomba de reposição de água
  }
}

// Função principal
void setup() 
{
  iniciarHardware();
  iniciarRequisicao();
}

void loop() 
{
  float temperatura, umidade, umidadeSolo, waterLevel;

  if (selectedId != -1 && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullUrl = String(apiUrlGetById) + String(selectedId); // Monta a URL com o ID
    http.begin(fullUrl);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String response = http.getString();
      Serial.println("Resposta do GetById: " + response);

      // Processar JSON para atualizar as variáveis
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);

      TEMP_MIN = doc["maxTemp"].as<float>();
      TEMP_MAX = doc["minTemp"].as<float>();
      UMIDADE_SOLO_MIN = doc["minSoilMoisture"].as<float>();
      NIVEL_AGUA_MIN = doc["minWaterLevel"].as<float>();

      // Exibe as variáveis no monitor serial
      Serial.print("TEMP_MIN: "); Serial.println(TEMP_MIN);
      Serial.print("TEMP_MAX: "); Serial.println(TEMP_MAX);
      Serial.print("UMIDADE_SOLO_LIMIAR: "); Serial.println(UMIDADE_SOLO_LIMIAR);
      Serial.print("NIVEL_AGUA_MIN: "); Serial.println(NIVEL_AGUA_MIN);
      delay(20000);

    } else {
      Serial.println("Erro ao obter dados do GetById.");
      delay(20000);
    }

    http.end();
  }

  lerSensores(temperatura, umidade, umidadeSolo, waterLevel);
  exibirDadosSerial(temperatura, umidade, umidadeSolo, waterLevel); 
  controlarTemperatura(temperatura);
  controlarIrrigacao(umidadeSolo, waterLevel);

  delay(2000);  // Delay para evitar leituras muito rápidas
}