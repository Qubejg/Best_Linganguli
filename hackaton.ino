#include <LiquidCrystal.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// ==========================================
// KONFIGURACJA KOMUNIKACJI Z ESP32
// RX = A2 (pusty), TX = A3 (TU PODŁĄCZ KABEL DO ESP32!)
// ==========================================
SoftwareSerial espSerial(A2, A3); 

// Piny
const int servoPin = 10; 
int sequence=0;

// --- Czujnik 1: Wysokość śmieci ---
const int trigPin_height = 7; 
const int echoPin_height = 6; 
const int bin_height = 30; // Wysokość pojemnik a w cm

// --- Czujnik 2: Wykrywanie obecności na pokrywie ---
const int trigPin_lid = 8; // Nowy pin Trigger dla czujnika pokrywy
const int echoPin_lid = 9; // Nowy pin Echo dla czujnika pokrywy
const int LID_TRIGGER_DISTANCE = 7; // Maksymalna odległość w cm, przy której uznajemy, że coś leży na pokrywie

// Piny LCD (Zgodnie z Twoim kodem: RS=12, EN=8)
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

bool IS_TILT=0;

Servo myservo; 
long duration; 
int trash_distanceCm; 
int lid_distanceCm; // Nowa zmienna dla odległości od pokrywy

// Znak pełnego bloku (pozostawiam Twój wzór)
byte pelny_blok[8] = { B00100, B11111, B00000, B11111, B10101, B10101, B10101, B11111 };

enum BinState {
    STATE_EMPTY,
    STATE_LOW,
    STATE_HALF_EMPTY,
    STATE_HALF_FULL,
    STATE_FULL,
    STATE_ERROR 
};

BinState previousState = STATE_ERROR; 

void setup() {
    // 1. Start komunikacji
    Serial.begin(9600);       // USB do komputera
    espSerial.begin(9600);    // Do ESP32 na pinie A3!

    Serial.println("Start systemu...");

    // 2. Konfiguracja urządzeń
    myservo.attach(servoPin);
    myservo.write(90); // Pozycja zamknięta / neutralna
    
    // Konfiguracja Czujnika 1 (Wysokość śmieci)
    pinMode(trigPin_height, OUTPUT);
    pinMode(echoPin_height, INPUT);
    
    // Konfiguracja Czujnika 2 (Pokrywa)
    pinMode(trigPin_lid, OUTPUT);
    pinMode(echoPin_lid, INPUT);
    
    lcd.begin(16, 2);
    lcd.createChar(0, pelny_blok); 
    
    lcd.setCursor(0, 0);
    lcd.print("Trash Monitor");
    delay(1000);
    lcd.clear(); 
}

// --- Funkcje pomocnicze ---
void clearRow(int row_num) {
    lcd.setCursor(0, row_num); 
    for (int i = 0; i < 16; i++) { lcd.print(" "); }
}

void updateBar(int bars) {
    clearRow(1);
    lcd.setCursor(0, 1);
    for(int i = 0; i < bars; i++) { lcd.write(byte(0)); }
}

/**
 * Funkcja mierząca odległość dla dowolnej pary pinów Trig/Echo
 */
int getDistance(int trigPin,int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH, 30000); 
    if (duration == 0) return -1; 
    return duration / 58;
}

void loop() {
    // Odczyt czujnika wysokości (Śmieci)
    trash_distanceCm = getDistance(trigPin_height,echoPin_height);
    
    // Odczyt czujnika pokrywy (Nowy czujnik)
    lid_distanceCm = getDistance(trigPin_lid, echoPin_lid);

    // ==================================================
    // 1. LOGIKA CZUJNIKA POKRYWY I SERWA (NOWA FUNKCJONALNOŚĆ)
    // ==================================================
    
    if (lid_distanceCm > 0 && lid_distanceCm <= LID_TRIGGER_DISTANCE && IS_TILT==0) {
        // Wykryto obiekt leżący na pokrywie/przed czujnikiem (np. smieć/ręka)
        Serial.println("DETECTED! Otwieram pokrywę (180 deg).");
        Serial.println(lid_distanceCm);
        if (sequence==0)
        {
          myservo.write(160);  
        }else if (sequence==1)
        {
          myservo.write(160);  
        }else if (sequence==2)
        {
            myservo.write(30); 
            sequence=-1;
        }
        
        IS_TILT=1; 
        sequence++;     
        
    } else {
        // Brak wykrycia (serwo domyślnie wraca do zamknięcia)
        myservo.write(90); // Ustaw serwo w pozycji ZAMKNIĘTEJ/NEUTRALNEJ
        delay(100);
        IS_TILT=0;                  
    }


    // ==================================================
    // 2. WYSYŁANIE DO ESP32
    // ==================================================
    if (trash_distanceCm > 0 && trash_distanceCm < 100) {
        espSerial.println(trash_distanceCm); 
        Serial.print("Wysylam do ESP: "); 
        Serial.println(trash_distanceCm);
    } else {
        Serial.println("Blad odczytu - nie wysylam");
    }

    // ==================================================
    // 3. LOGIKA LCD I STANÓW
    // ==================================================
    float filled = (float)bin_height - trash_distanceCm;
    BinState currentState;
    int barsToShow = 0; 

    if (trash_distanceCm < 0 || trash_distanceCm > bin_height * 1.5) { 
        currentState = STATE_ERROR;
    } else if (trash_distanceCm >= bin_height) { 
        currentState = STATE_EMPTY;
    } else { 
        if (filled <= bin_height * 0.3) { currentState = STATE_LOW; } 
        else if (filled <= bin_height * 0.5) { currentState = STATE_HALF_EMPTY; } 
        else if (filled <= bin_height * 0.7) { currentState = STATE_HALF_FULL; } 
        else { currentState = STATE_FULL; }
    }

    if (currentState != previousState) {
        clearRow(0);
        clearRow(1);
        lcd.setCursor(0, 0);
        
        switch (currentState) {
            case STATE_ERROR: lcd.print("Blad!"); barsToShow=0; break;
            case STATE_EMPTY: lcd.print("Pusto!"); barsToShow=0; break;
            case STATE_LOW: lcd.print("Niski poziom"); barsToShow=4; break;
            case STATE_HALF_EMPTY: lcd.print("Polowa pusta"); barsToShow=8; break;
            case STATE_HALF_FULL: lcd.print("Polowa pelna"); barsToShow=12; break;
            case STATE_FULL: lcd.print("PELNY!"); barsToShow=16; break;
        }
        updateBar(barsToShow);
        previousState = currentState;
    }
    
    // Opóźnienie, żeby nie zalać ESP danymi i uniknąć migotania
    delay(500); // Zmienione na 500ms, bo jest więcej do zrobienia
}
