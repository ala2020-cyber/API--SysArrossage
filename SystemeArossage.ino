#include "DHT.h"
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>


int servoPin = 13;
bool humidite; // 1 si l'humidité est satisfait, 0 si elle est basse
bool precipitation = false; // initialiser precipitation
bool temperature = false; // initialiser remperature
int  seuilHumidite; // seuil d'humidité pour l'arrosage
float tempMeteo;
float humidityMeteo;
float humidityCapteur;
const int seuilTemperature = 20; // Seuil de température pour l'arrosage
String typeDeSol = ""; // Variable pour stocker le type de sol


// const char* ssid = "Hexagone";       // Remplacez par votre SSID
// const char* password = "i5jhn7p8!A"; // Remplacez par votre mot de passe Wi-Fi

const char* ssid = "Redmi Note 13 Pro 5G";       
const char* password = "g4h9u2m6475zu2b";

const char* apiKey = "267f4f39235d4790c32e0cb0016dec70"; 
const char* city = "Versailles";
const char* country = "FR"; 

DHT dht(26, DHT11); 
Servo myServo;
WebServer server(80);




void adjustSeuilHumidite() {
  if(typeDeSol == "Sableux") {
    seuilHumidite = 20;
  } else if(typeDeSol == "Limouneux") {
    seuilHumidite = 40;
  } else {
    seuilHumidite = 60;
  }
}


void handleRoot(){

    String page = "<html><body>"
                "<h1>Type de Sol</h1>"
                "<form action=\"/setTypeDeSol\" method=\"post\">"
                "<input type=\"radio\" name=\"typeDeSol\" value=\"Sableux\"> Sableux<br>"
                "<input type=\"radio\" name=\"typeDeSol\" value=\"Limouneux\"> Limouneux<br>"
                "<input type=\"radio\" name=\"typeDeSol\" value=\"Argileux\"> Argileux<br>"
                "<input type=\"submit\" value=\"Envoyer\">"
                "</form>"
                "</body></html>";
  server.send(200,"text/html",page);

}

void handleSetTypeDeSol() {
  if (server.hasArg("typeDeSol")) {
    typeDeSol = server.arg("typeDeSol");
    Serial.print("Type de sol reçu: ");
    Serial.println(typeDeSol);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleNotFound(){
  server.send(404,"text/plain","404 : Not found!");
}


bool lireHumidite(int seuil) {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Erreur de lecture du capteur DHT!");
    return false; // Erreur de lecture
  }

  Serial.print("Humidité: ");
  Serial.print(h);
  Serial.println(" %");
  humidityCapteur = h;
  return h > seuil ? true : false;
}


bool lireTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Erreur de lecture du capteur DHT!");
    return false; // Erreur de lecture
  }
  Serial.println("***Température avec capteur DHT11***");
  Serial.print("Température: ");
  Serial.print(t);
  Serial.println(" °C");
  return t < seuilTemperature ? true : false;
}

bool AnalyseDonneesMeteo() {
  if ((WiFi.status() == WL_CONNECTED)) { // Vérifie si l'ESP32 est connecté au WiFi
    HTTPClient http;

    String serverPath = String("http://api.openweathermap.org/data/2.5/weather?q=") + city  + "&appid=" + apiKey + "&units=metric";
    
    http.begin(serverPath.c_str());
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      // Serial.println(payload);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      tempMeteo = doc["main"]["temp"];
      humidityMeteo = doc["main"]["humidity"];
      
      Serial.println();
      Serial.println("***Analyse des données météo*** ");
      Serial.print("Température météo: ");
      Serial.println(tempMeteo);
      Serial.print("Humidité météo: ");
      Serial.println(humidityMeteo);

      temperature = (tempMeteo < 20);
      precipitation = (humidityMeteo >= 70); 


      
      http.end();
      return true;
    } else {
      Serial.print("Erreur lors de la requête HTTP: ");
      Serial.println(httpResponseCode);
      http.end();
      return false;
    }
  } else {
    Serial.println("WiFi non connecté");
    return false;
  }
}


void activerArrosage() {
  Serial.println("Activation de l'arrosage");
  myServo.write(90); // Ouvrir la vanne

  // Déterminer le délai en fonction du type de sol
  int delayDuration;
  if (typeDeSol == "Sableux") {
    delayDuration = 5000; // 5 secondes pour le sol sableux
  } else if (typeDeSol == "Limouneux") {
    delayDuration = 10000; // 10 secondes pour le sol limoneux
  } else if (typeDeSol == "Argileux") {
    delayDuration = 20000; // 20 secondes pour le sol argileux
  } else {
    delayDuration = 5000; // Valeur par défaut si le type de sol n'est pas spécifié
  }

  delay(delayDuration); // Maintenir la vanne ouverte pour la durée déterminée

  myServo.write(0); // Fermer la vanne
  Serial.println("Arrosage terminé");
}



void envoyerDonnees(float temperatureMeteo, float humidityMeteo, float humidityCapteur) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://127.0.0.1:3000/api/sensor-data");
    http.addHeader("Content-Type", "text/plain");

    String payload = "{\"temperatureMeteo\":" + String(temperatureMeteo) + ",\"humidityMeteo\":" + String(humidityMeteo) + ",\"humidityCapteur\":" + String(humidityCapteur) +"}";

    Serial.println("Payload: " + payload); 
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Erreur lors de l'envoi des données! ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi non connecté");
  }
}









void setup() {
  Serial.begin(115200);

  // lancer lecture du capteur humidité
  dht.begin();

  // Micro Servo en attente
  myServo.attach(servoPin);

  // connecter sur le wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connexion WiFi...");
  }

  Serial.println("WiFi connecté!");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());



  // lancer une page web pour que utilisateur indiquer le type du sol

  server.on("/",handleRoot);
  server.on("/setTypeDeSol", HTTP_POST, handleSetTypeDeSol);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server web actif!");

  
  delay(2000);
}




void loop() {
  
  delay(5000);

  server.handleClient();

  if(typeDeSol != "") {
    adjustSeuilHumidite();
    do {
      if (AnalyseDonneesMeteo()) {
          Serial.println("***Humidité de sol avec capteur DHT11***");
          lireHumidite(seuilHumidite);
        if (temperature && precipitation) {
          delay(5000);
        } else  {
          // Envoyer les données à MongoDB
          envoyerDonnees(tempMeteo, humidityMeteo, humidityCapteur);
          activerArrosage();
          typeDeSol = ""; // Réinitialiser le type de sol 
          break; // Sortir de la boucle do-while après l'arrosage
        }
      }
    } while (lireHumidite(seuilHumidite) == false);
  }
}




