/*
 * CoffeeGuard Initialisierung by Ramon Hofer Kraner und OST Fachhochschule
 * 
 * Dieser Sketch hilft beim Einrichten der CoffeeGuard Waage. 
 * Es müssen die beiden Parameter 
 * 
 * Basisgewicht
 * Umrechungsfaktor 
 * 
 * bestimmt werden. 
 * 
 * Dieser Sketch basiert zu grossen Teilen auf Code von anderen. 
 * Es wurden diverse Libraries und Funktionen von folgenden Ressourcen genutzt:
 * 
 * 1) HX711 Library by Bogdan Necula:        https://github.com/bogde/HX711
 * 2) ESP Board installieren: 
      Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
           http://arduino.esp8266.com/stable/package_esp8266com_index.json
      - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
      - Select your ESP8266 in "Tools -> Board"


* Der Code wurde auf einem 
* NodeMCU ESP8266 NoName Board (https://de.aliexpress.com/item/4000160133215.html) mit einer 
* JOY-IT Wägezelle (https://www.conrad.ch/de/p/joy-it-sen-hx711-05-waegezelle-1-st-2475884.html)
* getestet. 
* 
**/



#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
const int LED_PIN = 2;


HX711 scale;

// *************************************************
// Diese beiden Paramter müssen bestimmt und hier angepasst werden:

long Basisgewicht = 109400; // 603938 109400; //1 x Karton = 9480
float Umrechnungsfaktor = 436.07f; //436.07f;

// *************************************************


long scalezero = 0;
int i=0;
float Input; 
float Eichgewichtwert;
long Sensorwert = 0;



float WaitForInput(){
  
  while (!Serial.available()){
        while (Serial.available() > 0){
      
            Input = Serial.parseFloat();
            
            if (Serial.read() == '\n') {
                Serial.println(Input);
                Serial.println();
                return Input; 
            }
          
            delay(10);  // damit der watchdog nicht aktiv wird
        
        }
  }

}



void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Willkommen zur Kalibrierung");
  Serial.println();
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(346.16f);
}



void loop() {

    Serial.println();
    Serial.println();
    Serial.println("Folgende Schritte werden durchlaufen: ");
    Serial.println();
    Serial.println("Schritt 1: Richtung bestimmen");
    Serial.println("Schritt 2: Basisgewicht bestimmen");
    Serial.println("Schritt 3: Basisgewicht überprüfen");
    Serial.println("Schritt 4: Umrechungsfaktor bestimmen");
    Serial.println("Schritt 5: Umrechungsfaktor überprüfen");
    Serial.println("");
 

    Serial.println("Bitte die Waage konstant belasten und bliebige Eingabe machen");
    Serial.print("Weiter mit beliebiger Zahl");
    Serial.println("");

    Input = WaitForInput();

    Serial.println();
    Serial.println();
    Serial.println("Schritt 1: Richtung bestimmen");
    Serial.println("Die konstante Belastung sollte eine"); 
    Serial.println("POSITIVE Veränderung des Sensorwerts bewirken");
    Serial.println("Falls dies nicht der Fall ist, Wäägezelle umdrehen");
    Serial.print("Folgender Sensorwert sollte POSITIV sein: ");
    Serial.println();
    Sensorwert = scale.read_average(20);
    Serial.print(Sensorwert);
    Serial.println();
    Serial.print("Weiter mit beliebiger Zahl");

    Input = WaitForInput();

    Serial.println();
    Serial.println();
    Serial.println("Schritt 2: Basisgewicht bestimmen");
    Serial.println("Den Teller und alles was als Basis auf"); 
    Serial.println("die Waage kommt, montieren.");
        
    Serial.println(); 
    
    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();
    
    Serial.println(); 
    Serial.println("Der folgende Wert ist nun das Basisgewicht:");
    Serial.println();
    Serial.print("BASISGEWICHT (als Sensorwert) =  ");
    Basisgewicht = scale.read_average(20);
    Serial.println(Basisgewicht);
    Serial.println();
    Serial.println("Dies ist das BASISGEWICHT und kann im Code eingetragen werden");
    Serial.println(); 
    
    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();
    
    Serial.println();
    Serial.println(); 
    Serial.println("Schritt 3: Basisgewicht überprüfen");
    Serial.println("Der Sensorwert minus Basisgewicht sollte nun ungefähr um NULL schwanken");
    Serial.println(); 
    Serial.print("Sensorwert =  ");
    Sensorwert = scale.read_average(20);
    Serial.println(Sensorwert);
    Serial.print("BASISGEWICHT =  ");
    Serial.println(Basisgewicht);
    Serial.print("Sensorwert - BASISGEWICHT =  ");
    Serial.println(Sensorwert - Basisgewicht);
    Serial.println();
    Serial.println();
    
    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();


    Serial.println();
    Serial.println(); 
    Serial.println("Schritt 4: Umrechnungsfaktor bestimmen");
    Serial.println("Ein bekanntes Eichgewicht auf die Waage legen (z.B. Münzen)."); 
    Serial.println();
    
    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();
    
    Serial.println("Der Ausschlag des Sensorwerts minus BASISGEWICHT entspricht nun dem Eichgewicht");
    Serial.println();
    Serial.println("Eichgewichtwert = Sensorwert - BASISGEWICHT = ");
    Sensorwert = scale.read_average(20);
    Eichgewichtwert = Sensorwert - Basisgewicht ;
    Serial.print(Eichgewichtwert);
    Serial.print(" = ");
    Serial.print(Sensorwert);
    Serial.print(" - ");
    Serial.println(Basisgewicht);
    Serial.println();
    Serial.println("Wie schwer war dein Eichgewicht?");
    Serial.print("Bitte gib das Gewicht in Gramm (Komma erlaubt) ein: ");
    
    Input = WaitForInput();

    Serial.println("Nun kann der UMRECHUNGSFAKTOR berechnet werden:");
    Serial.println("UMRECHUNGSFAKTOR = Eichgewichtwert / Gewicht (in g)");

    Umrechnungsfaktor = Eichgewichtwert / Input;
    Serial.print(Umrechnungsfaktor, 3);
    Serial.print(" = ");
    Serial.print(Eichgewichtwert);
    Serial.print(" / ");
    Serial.println(Input);

    Serial.println("Nun sind beide Werte bekannt und können im Code eingetragen werden:");

    Serial.println();
    Serial.println("--------------");
    Serial.print("BASISGEWICHT =  ");
    Serial.println(Basisgewicht);
    Serial.print("UMRECHUNGSFAKTOR =  ");
    Serial.println(Umrechnungsfaktor, 3);
    Serial.println("--------------");
    
    Serial.println();
    Serial.println();

    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();

    Serial.println();
    Serial.println();
    
    Serial.println("Schritt 5: Umrechungsfaktor überprüfen");
    Serial.println("Einen Gegenstand mit bekanntem Gewicht auf die Waage legen.");
    
    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();

    Serial.print("Gemessenes Gewicht: ");
    Serial.println((scale.read_average(20) - Basisgewicht)/Umrechnungsfaktor);
    Serial.println();
    Serial.println("Falls dies stimmt ist alles super!");

    Serial.print("Weiter mit beliebiger Zahl: ");
    Input = WaitForInput();

    Serial.println("Schritt 6: LED testen");
    
    Serial.println("Die LED sollte nach der Eingabe blinken");

    Serial.println("Gib die Anzahl Blinks ein: ");
    Input = WaitForInput();
    
    for (int i=0; i<Input; i++){
      Serial.print("b.");
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }

    Serial.println();
    Serial.println();
    Serial.println("Einrichtung fertig!");
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();

    
       
}
