// Bibliotecas
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <PubSubClient.h> 
#include <WiFi.h>

// Definindo Topicos
#define TOPICO_SUBSCRIBE    "/TEF/lamp109/cmd"        //Tópico MQTT de escuta
#define TOPICO_PUBLISH      "/TEF/lamp109/attrs"      //Tópico MQTT de envio de informações para Broker
#define TOPICO_PUBLISH_3    "/TEF/lamp109/attrs/u"    //Tópico MQTT dos dados de Umidade
#define TOPICO_PUBLISH_4    "/TEF/lamp109/attrs/t"    //Tópico MQTT dos dados de Temperatura
#define ID_MQTT  "fiware_109"   //id mqtt (para identificação de sessão)
                                
// WIFI
const char* SSID = "Wokwi-GUEST"; // SSID / nome da rede WI-FI 
const char* PASSWORD = ""; // Senha da rede WI-FI
  
// MQTT
const char* BROKER_MQTT = "46.17.108.113"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;                    // Porta do Broker MQTT

// Variáveis e objetos globais
WiFiClient espClient;          // Cria o objeto espClient
PubSubClient MQTT(espClient);  // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';        // Variável que armazena o estado atual da saída

// Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);

// Display LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT11 ou DHT22
#define DHTPIN 2
#define DHTTYPE DHT22
//#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//----------------------------------------------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  // Inicializações:
  InitOutput();
  initSerial();
  initWiFi();
  initMQTT();
  delay(5000);

  Serial.println(F("DHTxx Test!"));
}

void initSerial() 
{
  Serial.begin(115200);
}

// Função: Inicializa e conecta-se na rede WI-FI desejada
void initWiFi() 
{
    delay(10);
    Serial.println("----- Conexão WI-FI -----");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde...");
     
    reconectWiFi();
}
  
// Função: Inicializa parâmetros de conexão MQTT(endereço do broker, porta e seta função de callback)
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
    MQTT.setCallback(mqtt_callback);
}

//----------------------------------------------------------------------------------------------------------

// Função: Função de callback: Chamada toda vez que uma informação de um dos tópicos subescritos chega
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);
  
    // Toma ação dependendo da string recebida:
    // Verifica se deve colocar nivel alto de tensão na saída D0:
    if (msg.equals("lamp109@on|"))
    {
        digitalWrite(2, HIGH);
        EstadoSaida = '1';
    }
 
    //Verifica se deve colocar nivel alto de tensão na saída D0:
    if (msg.equals("lamp109@off|"))
    {
        digitalWrite(2, LOW);
        EstadoSaida = '0';
    }
    
}

//----------------------------------------------------------------------------------------------------------

// Função: Reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao Broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no Broker.");
            Serial.println("Nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

// Função: Reconecta-se ao WiFi
void reconectWiFi() 
{
  // Se já está conectado a rede WI-FI, nada é feito. Caso contrário, ocorrerá tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
      return;
        
  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(100);
      Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Conectado com sucesso na rede: ");
  Serial.print(SSID);
  Serial.print("\n");
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());
}
 
// Função: Verifica o estado das conexões WiFI e ao broker MQTT. Em caso de desconexão, a conexão é refeita.
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //Se não há conexão com o Broker, a conexão é refeita
     
     reconectWiFi(); //Se não há conexão com o WiFI, a conexão é refeita
}

//----------------------------------------------------------------------------------------------------------

// Função: Envia ao Broker o estado atual do output 
void EnviaEstadoOutputMQTT(void)
{
    if (EstadoSaida == '1')
    {
      MQTT.publish(TOPICO_PUBLISH, "s|on");
      Serial.println("- Led Ligado");
    }
    if (EstadoSaida == '0')
    {
      MQTT.publish(TOPICO_PUBLISH, "s|off");
      Serial.println("- Led Desligado");
    }
    Serial.println("- Estado do LED enviado ao Broker!");
    delay(1000);
}
 
// Função: Inicializa o output em nível lógico baixo
void InitOutput(void)
{
    // IMPORTANTE: O Led já contido na placa é acionado com lógica invertida (ou seja,
    // enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    
    boolean toggle = false;

    for(int i = 0; i <= 10; i++)
    {
        toggle = !toggle;
        digitalWrite(2,toggle);
        delay(200);           
    }

    digitalWrite(2, LOW);
}

//----------------------------------------------------------------------------------------------------------

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);

  char msgBuffer[4];
  // Garante funcionamento das conexões WiFi e ao broker MQTT
  VerificaConexoesWiFIEMQTT();

  // Envia o status de todos os outputs para o Broker no protocolo esperado
  EnviaEstadoOutputMQTT();

  // keep-alive da comunicação com broker MQTT
  MQTT.loop();

  // Leitura DHT
  float h = dht.readHumidity();
  dtostrf(h, 4, 2, msgBuffer);
  MQTT.publish(TOPICO_PUBLISH_3,msgBuffer);

  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  dtostrf(t, 4, 2, msgBuffer);
  MQTT.publish(TOPICO_PUBLISH_4,msgBuffer);

  if (isnan(h) || isnan(t) || isnan(f)){
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  // Iniciando LCD
  lcd.init();
  lcd.backlight();

  lcd.print("Umidade: ");
  lcd.print(h);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.print(" C");

  delay(2000);

  if (t >= 30 && h >= 60){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Umidade:Alta");
    lcd.setCursor(0, 1);
    lcd.print("Temperatura:Alta");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0 );
    lcd.print("ALERTA DE");
    lcd.setCursor(0, 1);
    lcd.print("ENCHENTE !!!");
  }

  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Umidade: OK");
    lcd.setCursor(0, 1);
    lcd.print("Temperatura: OK ");
  }


  Serial.print(F("Umidade:  "));
  Serial.print(h);
  Serial.print(F("% || Temperatura: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F || Indice de calor: "));
  Serial.print(hic);
  Serial.print(F(" °C "));
  Serial.print(hif);
  Serial.println(F(" °F "));

}
