/*
 * CoffeeGuard by Ramon Hofer Kraner und OST Fachhochschule
 * 
 * Dieser Sketch basiert zu grossen Teilen auf Code von anderen. 
 * Es wurden diverse Libraries und Funktionen von folgenden Ressourcen genutzt:
 * 
 * 1) MQTT PubSub Library by Nick O'Leary:                https://github.com/knolleary/pubsubclient
 * 2) ESP8266 Sending Email by Rui Santos:                https://randomnerdtutorials.com/esp8266-nodemcu-send-email-smtp-server-arduino/
 * 3) ESP Mail Client Library by K. Suwatchai (Mobizt).:  https://github.com/mobizt/ESP-Mail-Client
 * 4) HX711 Library by Bogdan Necula:                     https://github.com/bogde/HX711
 * 5) ESP Board installieren: 
      Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
           http://arduino.esp8266.com/stable/package_esp8266com_index.json
      - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
      - Select your ESP8266 in "Tools -> Board"


* Der Code wurde auf einem 
* ESP8266 NoName Board (https://de.aliexpress.com/item/4000160133215.html) mit einer 
* JOY-IT Wägezelle (https://www.conrad.ch/de/p/joy-it-sen-hx711-05-waegezelle-1-st-2475884.html)
* getestet. 
* 
**/


//Includes
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ESP_Mail_Client.h>

// Pins
const int LED_PIN = 2;  // D4 on Board siehe https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const int LOADCELL_DOUT_PIN = 4; // D2 on Board siehe https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const int LOADCELL_SCK_PIN = 5; //  D1 on Board

// Diverse Objekte
HX711 scale;  // Waage
WiFiClient espClient; // WIFI
PubSubClient client(espClient); // for MQTT
SMTPSession smtp; // Email smtp

void smtpCallback(SMTP_Status status); /* Callback function to get the Email sending status */


// ******************* Zu konfigurierende Parameter: ******************
// Netzwerk
char MQTTserver[] = "test.mosquitto.org";           // MQTT Broker
char MQTTTopicTime [] = "CG/Timer";             // Topic für den Timer
char MQTTTopicPads [] = "CG/Pads";              // Topic für die aktuelle Padanzahl
char MQTT_CLIENT_ID [] = "CG_123123";               // Muss für jeden Client eindeutig sein

const char* ssid = "";
const char* password = "";

//Email
#define SMTP_HOST ""                                  // SMTP Server
#define AUTHOR_PASSWORD ""                            // Server Passwort
#define SMTP_PORT 465

#define AUTHOR_EMAIL ""                               // Absender Email
#define AUTHOR_NAME  "CoffeeBot"                      // Absender Email

#define RECIPIENT_EMAIL  ""                           // Empfänger Email
#define RECIPIENT_NAME  "MAKE"                        // Empfänger Name

#define ALARM_EMAIL_TITEL  "Kaffee Alarm!"            // Titel in der Alarm Email
#define REFILL_EMAIL_TITEL "Kaffee wieder OK!"        // Titel in der Refill Email


// Waage
float basisGewicht = 109400;            // Sensorwert mit allem was auf der Waage ohne Pads liegt
float umrechnungsFaktor =  436.07f;     // Damit eine korrekte Umrechung in Gramm funktioniert.

int messIntervall = 5000;               // Messintervall in ms
int kaffeeKapselLimit = 10;             // Unter diesem Wert werden Emails verschickt
float padGewicht = 6.4f;                // Ein Kaffeepad
int multiMessungSchwelle = 10;          // Wie viel Mal gemessen werden muss, bis der Alarm als valabel gilt


// *****************************************************************



// Diverse Variablen
int ledState = HIGH;
float gewicht = 0;
int nofPads = 1000;
long initDelayStart = 0;
long zeitSeitStart = 0;
long now = 0;
long letzteMessZeit = 0, lastTimeMQTT = 0;
char msg[50], buf[10];
bool emailGesendet = false;
int multiMessung = 0;
long zyklusCounter = 1;
long letzterZyklus = 0;




/* WIFI Initialisierung ----------------------------------------------------- 
 * Verbindung zum WLAN aufbauen
 */
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);
    delay(50);
    Serial.print(".");
    digitalWrite(LED_PIN, HIGH);
    delay(50);
  }

  //randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}


/* Email senden ----------------------------------------------------- 
 * Verbindung zum WLAN aufbauen
 * Input: nofpads, Anzahl Pads
 * Input: empty, Soll ein Email für den Zustand Empty oder Full geschickt werden? Je nachdem wird ein anderer Text gesendet
 */
void send_email(int nofpads, bool empty){

  /* Declare the session config data */
  ESP_Mail_Session session;
  /* Declare the message class */
  SMTP_Message message;

  /** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1
  */
  smtp.debug(0);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
  message.sender.name = AUTHOR_NAME;
  message.sender.email = AUTHOR_EMAIL;

  String htmlMsg;

  if (empty == true){
      /* Set the message headers */
      message.subject = ALARM_EMAIL_TITEL;
      message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);
    
      /*Send HTML message*/
      htmlMsg = "<div style=\"color:#2f4468;\">Der Bestand an Kapseln in der Werft nähert sich einem furchterregenden Stand an<p>Es hat nur noch: <p><h1>";
      htmlMsg += nofpads; //toInt()
      htmlMsg += "</h1>Pads!";
      htmlMsg += "<p>direkt aus der Werft gesendet ;)</p></div>";
  }

  else {

      /* Set the message headers */
      message.subject = REFILL_EMAIL_TITEL;
      message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);
    
      /*Send HTML message*/
      htmlMsg = "<div style=\"color:#2f4468;\">Der Bestand an Kapseln in der Werft ist wieder auf: <p><h1>";
      htmlMsg += nofpads; //toInt()
      htmlMsg += "</h1> aufgefüllt worden. Danke dem Spender!";
      htmlMsg += "<p>direkt aus der Werft gesendet ;)</p></div>";
    
  }

  
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  

  /* Set the custom message header */
  //message.addHeader("Message-ID: <abcde.fghij@gmail.com>");

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

}


/* Email Callback  ----------------------------------------------------- 
 * Callback Funktion zum Senden der Emails
 * Input: status
 */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}




/* MQTT Message senden senden ----------------------------------------------------- 
 * Verbindung zum WLAN aufbauen
 * Input: message, Die Message
 * Input: Topic, Das Thema unter dem gepublished werden soll
 */
void SendMQTTMessage(String message, String Topic){

    if (!client.connected()) {
      client.connect(MQTT_CLIENT_ID);
    }
      
    Serial.print("Pub MQTT: ");
    Serial.print(message);
    Serial.print(" - Topic: ");
    Serial.println(Topic);
 
    
    int str_len1 = message.length() + 1; 
    char char_array1[str_len1];
    message.toCharArray(char_array1, str_len1);

    int str_len2 = Topic.length() + 1; 
    char char_array2[str_len2];
    Topic.toCharArray(char_array2, str_len2);
         
    client.publish(char_array2, char_array1);
}





/* Gewicht Messen  ----------------------------------------------------- 
 * Das totale Gewicht auf der Waage wird bestimmt. 
 * Es wird das Gewicht abzüglich allen Komponenten auf der Waage zurückgegeben
 */
float get_Weight(){
  float weight;
  weight = scale.read_average(20);
  return (weight - basisGewicht) / umrechnungsFaktor;
}


/* Anzahl Pads zurückgeben   ----------------------------------------------------- 
 * Das Gewicht wird durch das padGewicht geteilt.
 * Rückgabe: Anzahl Pads
 */
int get_NofPads(float weight){
  int nofPads;
  nofPads = round ((weight) / padGewicht);              
  return nofPads;
}

/* multiMessungsLogik ----------------------------------------------------- 
 * Überprüfen, ob 10 Mal nacheinander positiv gemessen wurde. 
 * Falls wir keinen Messzyklus ausgelassen haben, wird der Zykluszähler aufgezählt
 * ansonsten wird der Messzykluszähler wieder zurückgesetzt
 * Verwendet globale Variablen
 */
void CheckmultiMessung(){

   Serial.print("   - multiMessung: ");
   Serial.print(multiMessung);
   Serial.print(" / ");
   Serial.println(multiMessungSchwelle);
   
   Serial.print("   - Zyklus: ");
   Serial.print(zyklusCounter);
   Serial.print(" (");
   Serial.print(letzterZyklus);
   Serial.println(") ");
   Serial.println();

  // Falls wir keinen Zyklus verpasst haben multiMessung raufzählen, ansonsten wieder bei Null starten
   if (letzterZyklus == zyklusCounter - 1){
      multiMessung++; // Multi Messung raufzählen
   }
   else {
      multiMessung = 0;
   }

   letzterZyklus = zyklusCounter;           
}




/* Die Haupt Setup Routine  ----------------------------------------------------- 
 * Serial, MQTT, WIFI, Waage, Timing
 */
void setup() {
  
  Serial.begin(115200);
  client.setServer(MQTTserver, 1883);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  setup_wifi();
    
  // Scale Stuff
  Serial.println("Initializing the scale");    
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(umrechnungsFaktor); // Skalierung definieren

  initDelayStart = millis();  
  
}



/* Main Loop ----------------------------------------------------- 
 * Messung der vergangenen Zeit. Ist das messIntervall abgelaufen, Messung starten 
 * Ist die Anzahl an Kaffeekapsel unter einen Wert gesunken wird ein Email verschickt.
 * Ist die Anzahl wieder gestiegen wird ebenfalls ein Email verschickt.
 * Bei jedem messIntervall wird auch eine MQTT Message abgesetzt.
 * Sind zu wenig Kapseln da mit der LED blinken.
 */
void loop() {

    // Zeitstempel
    now = millis();
    
    // Immer blinken, falls Kaffeekapseln leer
    if (nofPads < kaffeeKapselLimit && emailGesendet == true) {
         Serial.print("b.");
         if (ledState == LOW) {
            ledState = HIGH;
          } else {
            ledState = LOW;
          }
          digitalWrite(LED_PIN, ledState);
          delay(200);
    }


    // Hier nur rein, falls das messIntervall erreicht wurde
    if ( ((now - letzteMessZeit) > messIntervall)) {

      zyklusCounter++;

      letzteMessZeit = now;
      zeitSeitStart = (now - initDelayStart)/1000;

      gewicht = get_Weight();
      nofPads = get_NofPads(gewicht);

      Serial.println();
      Serial.print("Neuer Messzyklus (");
      Serial.print(zyklusCounter);
      Serial.println(") ------------------------");
      
      Serial.print("Weight: ");
      sprintf(msg, "%f", gewicht); // float to char
      Serial.println(msg);
      Serial.print("NOF Pads: ");
      itoa(nofPads, msg, 10); // int to char
      Serial.println(msg);
      
      
      // Timer per MQTT senden
      itoa(zeitSeitStart, buf, 10);
      SendMQTTMessage(buf, MQTTTopicTime);
      
      // NOF Pads per MQTT senden
      itoa(nofPads, buf, 10);
      SendMQTTMessage(buf, MQTTTopicPads);



      // Falls Kapseln unter kaffeeKapselLimit gefallen Email mit der Anzahl Pads senden
      if (nofPads < kaffeeKapselLimit && emailGesendet == false) {

           Serial.println("--> Zu wenig Kapseln");
            
           CheckmultiMessung();
                    
           // Nur falls mehrere Male nacheinander ein zu tiefer Pegel festgestellt wurde ein Email senden. --> Fehlalarme reduzieren
           if (multiMessung > multiMessungSchwelle){
             send_email(nofPads, true);          
             emailGesendet = true; 
             multiMessung = 0; 
             Serial.println(" --> Email wird gesendet (Wenig Bestand)");
             Serial.println();
             
             // Led anschalten, damit diese auch an bleibt nach dem blinken
            digitalWrite(LED_PIN, HIGH);
           }
      }


      // Falls die Kapseln wieder aufgefüllt wurden (kleine Hysterese + 5 Kapsel mehr als Limit)
      if (nofPads > (kaffeeKapselLimit + 5) && emailGesendet == true) {

        Serial.println("--> Kapselanzahl wieder ok");
         
        CheckmultiMessung();

        if (multiMessung > multiMessungSchwelle){
            send_email(nofPads, false);
            emailGesendet = false;
            multiMessung = 0; 
            Serial.println(" --> Email wird gesendet (Bestand wieder OK)");
            Serial.println();
        }

         // Led anschalten, damit diese auch an bleibt nach dem blinken
            digitalWrite(LED_PIN, HIGH);
      } 
    } 
}
