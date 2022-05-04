#include <WiFi.h>
#include <TinyGPS++.h>
#include <Firebase_ESP_Client.h>
#include "secrets.h"

/* Configurações do GPS */
#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;

/* Configurações do Carro */
#define CAR_ID "61cf536fe2d293f2a0766871"

/* Firebase */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

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

void setupFirebase(){
  Serial.printf("Versão do cliente firebase: %s\n", FIREBASE_CLIENT_VERSION);
  
  /* Assign the database URL(required) */
  config.database_url = DATABASE_URL;
  
  config.signer.test_mode = true;

  //Initialize the library with the Firebase authen and config.
  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2); //gps baud
  setupFirebase();
}

void sendCurrentLocationToAPI (double lat, double lng, double speed){
  Serial.println("Enviando localização atual do carro para o Firebase RTDB.");
  
  if(WiFi.status() == WL_CONNECTED){
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
      Serial.println(fbdo.pushName());
      Serial.println(fbdo.dataPath() + "/"+ fbdo.pushName());
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

    sendCurrentLocationToAPI(lat, lng, speed);
  } /*else {
    Serial.println("O carro não está se movimentando!");
  }*/
}

void loop() {
  if (Firebase.ready()){
    //Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&fbdo, F("/"), 27) ? "ok" : fbdo.errorReason().c_str());
    getCurrentLocation();
  }
  
  delay(5000);
}
