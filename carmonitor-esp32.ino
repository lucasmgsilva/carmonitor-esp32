#include <WiFi.h>
#include <TinyGPS++.h>
#include <Firebase_ESP_Client.h>
#include "secrets.h"

/* API */
#define DATABASE_URL "carmonitor-4fc29-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyC01dpBT00At1GwJUwJAPzKBu-mYNX4Z4E"

/* Configurações do GPS */
#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;

/* Configurações do Buzzer */
#define BUZZER 18

/* Configurações do Carro */
#define CAR_ID "MRN-5208"

/* Firebase */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

double lastLat = 0.0;
double lastLng = 0.0;

void connectToWiFi(){
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.println("Conectando-se ao Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Conectado com o IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void tone(byte pin, int freq) {
  ledcSetup(0, 2000, 8);
  ledcAttachPin(pin, 0);
  ledcWriteTone(0, freq);
}

void setupFirebase(){
  Serial.printf("Versão do cliente firebase: %s\n", FIREBASE_CLIENT_VERSION);
  
  /* Assign the database URL(required) */
  config.database_url = DATABASE_URL;
  
  config.signer.test_mode = true;

  /* Initialize the library with the Firebase authen and config. */
  Firebase.begin(&config, &auth);

  /* Comment or pass false value when WiFi reconnection will control by your code or third party library */
  Firebase.reconnectWiFi(true);
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2); //gps baud
  setupFirebase();
}

void sendCurrentLocationToRTDB (double lat, double lng, double speed){
  Serial.println("Enviando localização atual do carro para o Firebase RTDB.");
  
  if(WiFi.status() == WL_CONNECTED){
    if (speed <= 5) speed = 0;
    
    json.add("lat", lat);
    json.add("lng", lng);
    json.add("speed", speed);

    char path[128];
    //sprintf(path, "/cars/%s/locations/", CAR_ID);
    sprintf(path, "/cars/%s/location/", CAR_ID);

    /*if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {*/
    if (Firebase.RTDB.updateNode(&fbdo, path, &json)) {
      Serial.println("Localização registrada com sucesso.");
      Serial.println(fbdo.dataPath());

      lastLat = lat;
      lastLng = lng;
    } else {
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Desconectado do Wi-Fi");
    connectToWiFi();
  }
  Serial.println();
}

void getCurrentLocation(){
  Serial.println("Tentando obter a localização atual do carro...");
  while (Serial1.available()) {
     gps.encode(Serial1.read());
  }
  
  if (gps.location.isUpdated()){
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    double speed = gps.speed.kmph();

    if (fabs(lat - lastLat) > 0.0001 || fabs(lng - lastLng) > 0.0001){
      sendCurrentLocationToRTDB(lat, lng, speed);
    } else {
      Serial.println("Baixa variação da latitude e/ou longitude da localização do carro.");
      Serial.println();
    }
  }
}

void shouldPlayAlarmSound(){
  if(WiFi.status() == WL_CONNECTED){
    char path[128];
    sprintf(path, "/cars/%s/playAlarmSound/", CAR_ID);

    if (Firebase.RTDB.getBool(&fbdo, path)) {
      if(fbdo.to<bool>()){
        tone(BUZZER,1500);
      } else {
        tone(BUZZER,0);
      }
    } else {
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Desconectado do Wi-Fi");
    connectToWiFi();
  }
}

void loop() {
  if (Firebase.ready()){
    getCurrentLocation();
    shouldPlayAlarmSound();
  }
  
 delay(500);
}
