
#include <Arduino.h>
#include <math.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>  // pour client MQTT
#include <DHT.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//pour le switch
#define device "client_esp5"
#define DHTPIN D4
#define DHTTYPE DHT11


bool trigSwitch = 0 ;
bool readSwitch;
int nb_bouton = 0;
float h = 0.0;
float t = 0.0;


// Pour le wifi
const char* ssid = "Raspberry";
const char* password = "Framboise";

//Pour MQTT

#define mqtt_server "192.168.4.1"
#define temperature_topic "esp5/temp"  //Topic température
#define humidity_topic "esp5/humid"        //Topic humidité
#define change_topic "esp5/change"

int nbMesures = 10; //moyennage avant envoi
int num = 1; // numero de mesure
float tempSum = 0, humidSum = 0;
long lastMsg = 0;   //Horodatage du dernier message publié sur MQTT

//Buffer qui permet de décoder les messages MQTT reçus
char message_buff[100];

//déclaration mqtt
WiFiClient espClient;
PubSubClient client(espClient);
//capteur de temp/humid
DHT dht(DHTPIN, DHTTYPE);

const unsigned char myBitmap [] PROGMEM = {
  0x00, 0xfc, 0x00, 0x03, 0xe0, 0x03, 0xff, 0x00, 0x0f, 0xf8, 0x07, 0xff, 0x80, 0x1f, 0xfc, 0x0f,
  0xff, 0xc0, 0x3f, 0xfe, 0x0f, 0xff, 0xe0, 0x3f, 0xfe, 0x1f, 0xc7, 0xe0, 0x7e, 0x3f, 0x1f, 0x03,
  0xf0, 0x7c, 0x1f, 0x1f, 0x01, 0xf0, 0x7c, 0x1f, 0x3f, 0x01, 0xf0, 0x7c, 0x1f, 0x3f, 0x01, 0xf0,
  0x7e, 0x3f, 0x3f, 0x01, 0xf0, 0x3f, 0xfe, 0x3f, 0x01, 0xf0, 0x3f, 0xfe, 0x3f, 0x01, 0xf0, 0x1f,
  0xfc, 0x3f, 0x01, 0xf0, 0x0f, 0xf8, 0x3f, 0x01, 0xf0, 0x03, 0xe0, 0x3f, 0x01, 0xf0, 0x00, 0x00,
  0x3f, 0x01, 0xf0, 0x00, 0x00, 0x3f, 0x01, 0xf0, 0x00, 0x00, 0x3f, 0x01, 0xf0, 0x00, 0x00, 0x3f,
  0x01, 0xf0, 0x00, 0x00, 0x3f, 0x01, 0xf0, 0x00, 0x00, 0x3f, 0x01, 0xf0, 0x00, 0x00, 0x3f, 0x01,
  0xf8, 0x00, 0x00, 0x7f, 0x31, 0xf8, 0x00, 0x00, 0x7e, 0x38, 0xfc, 0x00, 0x00, 0x7c, 0x78, 0xfc,
  0x00, 0x00, 0xf8, 0xfc, 0x7c, 0x00, 0x00, 0xf8, 0xfe, 0x7c, 0x00, 0x00, 0xf9, 0xfe, 0x7e, 0x00,
  0x00, 0xf8, 0xfe, 0x7e, 0x00, 0x00, 0xf8, 0xfe, 0x7c, 0x00, 0x00, 0xfc, 0x7c, 0x7c, 0x00, 0x00,
  0x7c, 0x00, 0xfc, 0x00, 0x00, 0x7e, 0x01, 0xf8, 0x00, 0x00, 0x3f, 0x83, 0xf8, 0x00, 0x00, 0x3f,
  0xff, 0xf0, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x00, 0x00, 0x0f, 0xff, 0xc0, 0x00, 0x00, 0x03, 0xff,
  0x80, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00
};

const unsigned char myBitmapp [] PROGMEM = {
  0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00,
  0x1f, 0x80, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x3f,
  0xc0, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0xff, 0xe0,
  0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x01, 0xff, 0xf8, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00,
  0x00, 0x03, 0xff, 0xfc, 0x00, 0x00, 0x07, 0xff, 0xfc, 0x00, 0x00, 0x0f, 0xff, 0xfe, 0x00, 0x00,
  0x0f, 0xff, 0xff, 0x00, 0x00, 0x1f, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xff, 0xff, 0x80, 0x00, 0x3f,
  0xff, 0xff, 0xc0, 0x00, 0x7f, 0xff, 0xff, 0xc0, 0x00, 0x7f, 0xff, 0xff, 0xc0, 0x00, 0x7f, 0xff,
  0xff, 0xe0, 0x00, 0xff, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xff, 0xe0, 0x00, 0xf9, 0xff, 0xff,
  0xe0, 0x00, 0xf8, 0xff, 0xff, 0xe0, 0x00, 0xf8, 0xff, 0xff, 0xe0, 0x00, 0xf8, 0xff, 0xff, 0xe0,
  0x00, 0x7c, 0xff, 0xff, 0xe0, 0x00, 0x7c, 0x7f, 0xff, 0xe0, 0x00, 0x7e, 0x3f, 0xff, 0xc0, 0x00,
  0x3f, 0x07, 0xff, 0xc0, 0x00, 0x3f, 0x81, 0xff, 0x80, 0x00, 0x1f, 0xe1, 0xff, 0x00, 0x00, 0x0f,
  0xff, 0xff, 0x00, 0x00, 0x07, 0xff, 0xfe, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x00, 0x00, 0xff,
  0xf0, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00
};
String change = "";


void displayTemp() {
  display.clearDisplay();

  display.setTextSize(1.5);
  display.setCursor(35, 0);
  display.println("Temperature");
  display.display();

  display.drawLine(0, 15, display.width() - 1, 15, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.drawBitmap(10, 20, myBitmap, 40, 40, WHITE);
  display.display();

  display.setTextSize(2);
  display.setCursor(52, 32);
  display.println(String(t) + "C");
  display.display();

}

void refreshDisplayTemp(){
    Serial.println("Actu");
  display.fillRect(51, 31, display.width()-1, display.height()-1, BLACK);
  display.display();
  
  display.setTextSize(2);
  display.setCursor(52, 32);
  display.println(String(t) + "C");
  display.display();
}


void displayHumid() {
  display.clearDisplay();

  display.setTextSize(1.5);
  display.setCursor(40, 0);
  display.println("Humidite");
  display.display();

  display.drawLine(0, 15, display.width() - 1, 15, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.drawBitmap(24, 20, myBitmapp, 40, 40, WHITE);
  display.display();

  display.setTextSize(3);
  display.setCursor(66, 32);
  display.println("" + String((int)h) + "%");
  display.display();
}

void refreshDisplayHumid(){
  Serial.println("Actu");
  display.fillRect(65, 31, display.width()-1, display.height()-1, BLACK);
  display.display();
  
  display.setTextSize(3);
  display.setCursor(66, 32);
  display.println("" + String((int)h) + "%");
  display.display();
}

void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect(device/*, mqtt_user, mqtt_password*/)) {
      Serial.println("OK");
      client.subscribe(change_topic);
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" On attend 5 secondes avant de recommencer");
      delay(5000);
    }
  }
}
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    Serial.print(receivedChar);
    if (receivedChar == 'H') {
      displayHumid();
      change = "H";
    }
    if (receivedChar == 'T') {
      displayTemp();
      change = "T";
    }
  }

  Serial.println();
  Serial.println("-----------------------");

}




String sendMQTT (float temp, float humid) {
  int tentatives = 0;
  if (!client.connected() and tentatives < 5) {
    reconnect();
    tentatives = tentatives + 1;
  }
  //client.loop();
  delay(1000);
  client.publish(temperature_topic, String(temp).c_str(), true);   //Publie la température sur le topic temperature_topic
  delay(1000);
  client.publish(humidity_topic, String(humid).c_str(), true);
  return ("MQTT done");

  // delay(60000);
}

//String sendONOFFMQTT (int signal) {
//  int tentatives=0;
//if (!client.connected() and tentatives < 5) {
//      reconnect();
//      tentatives = tentatives +1;
//      }
//    //client.loop();
//   client.publish(button_topic, String(signal).c_str(), true);
//
//  return ("MQTT done");
//
// // delay(60000);
//}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("=> Addresse IP : ");
  Serial.print(WiFi.localIP());
}




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setup_wifi();           //On se connecte au réseau wifi
  client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);

  display.setTextSize(3);
  display.setTextColor(WHITE);

  displayTemp();
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  long now = millis();
  //lecture toutes les 5 secondes
  if (now - lastMsg > 1000 * 5) {
    lastMsg = now;
    //Lecture de l'humidité ambiante
    h = dht.readHumidity();
    // Lecture de la température en Celcius
    t = dht.readTemperature();
        if(change == "H"){
      //displayHumid();
      refreshDisplayHumid();
    }
    else{
      //displayTemp();
      refreshDisplayTemp();
    }
    Serial.print("temperature :");
    Serial.println(t);
    //Inutile d'aller plus loin si le capteur ne renvoi rien
    if ( isnan(t) || isnan(h)) {
      Serial.println("Echec de lecture ! Verifiez votre capteur DHT");
      return;
    }
    tempSum = tempSum + t;
    humidSum = humidSum + h;
    num = num + 1;
  }
  if (num == nbMesures) {
    //client.publish(temperature_topic, String(tempSum/num).c_str(), true);   //Publie la température sur le topic temperature_topic
    //client.publish(humidity_topic, String(humidSum/num).c_str(), true);      //Et l'humidité
    Serial.println(sendMQTT(tempSum / nbMesures, humidSum / nbMesures));

    num = 0;
    tempSum = 0;
    humidSum = 0;
  }
  delay(100);
}
