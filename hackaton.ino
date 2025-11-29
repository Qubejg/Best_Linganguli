#include <LiquidCrystal.h>

// Definicja pinów podłączenia czujnika HC-SR04
const int trigPin = 7; 
const int echoPin = 6; 

// Zmienne globalne
long duration; 
int distanceCm; 
const int bin_height = 30; // Wysokość pojemnika w cm (ustawione jako stała)

// Użyjemy typu enum, aby zdefiniować i śledzić stany (więcej czytelności)
enum BinState {
    STATE_EMPTY,
    STATE_LOW,
    STATE_HALF_EMPTY,
    STATE_HALF_FULL,
    STATE_FULL,
    STATE_ERROR // Dystans poza zakresem kosza
};

// Zmienna przechowująca OSTATNIO WYŚWIETLONY stan
BinState previousState = STATE_ERROR; 

// Definicja niestandardowego znaku PEŁNEGO BLOKU (indeks 0)
byte pelny_blok[8] = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 };

// Inicjalizacja biblioteki LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  lcd.begin(16, 2);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Wgranie definicji pełnego bloku do CGRAM pod indeksem 0
  lcd.createChar(0, pelny_blok); 
  
  // Inicjalizacja komunikacji szeregowej
  Serial.begin(9600);
  Serial.println("HC-SR04 czujnik gotowy.");
  
  // Początkowy komunikat
  lcd.setCursor(0, 0);
  lcd.print("Trash level meter");
  delay(2000);
  lcd.clear(); // Wyczyść po starcie
}

// Funkcja do czyszczenia całego wiersza
void clearRow(int row_num) {
  lcd.setCursor(0, row_num); 
  for (int i = 0; i < 16; i++) {
    lcd.print(" ");
  }
}

// Funkcja do mierzenia odległości
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH, 30000); // Dodanie timeout
  
  // Zwraca odległość w cm lub -1 jeśli odczyt jest niepoprawny
  if (duration == 0) return -1; 
  return duration / 58;
}

// Funkcja do aktualizacji paska wypełnienia (tylko w drugim wierszu)
void updateBar(int bars) {
    clearRow(1); // Wyczyść tylko wiersz paska
    lcd.setCursor(0, 1);
    
    // Wypisanie bloku na podstawie wyliczonej liczby
    for(int i = 0; i < bars; i++) {
        lcd.write(byte(0));
    }
}

void loop() {
    distanceCm = getDistance();
    
    // Użyj bin_height (30 cm) jako odległości do pustego dna kosza
    // Poziom wypełnienia to bin_height - distanceCm
    float filled = (float)bin_height - distanceCm;
    
    BinState currentState;
    int barsToShow = 0; // Ile bloków (0-16) ma być wyświetlonych

    // --- 1. USTALENIE BIEŻĄCEGO STANU ---

    if (distanceCm < 0 || distanceCm > bin_height * 1.5) { // Błąd odczytu lub poza koszem
        currentState = STATE_ERROR;
    } else if (distanceCm >= bin_height) { // Czujnik widzi dno lub jest poza koszem (pusty)
        currentState = STATE_EMPTY;
        barsToShow = 0;
    } else { // W koszu jest coś, oblicz poziom
        
        
        // Określenie stanu na podstawie procentu wypełnienia (dla wiadomości tekstowej)
        if (filled <= bin_height * 0.3) {
            currentState = STATE_LOW;
        } else if (filled <= bin_height * 0.5) {
            currentState = STATE_HALF_EMPTY;
        } else if (filled <= bin_height * 0.7) {
            currentState = STATE_HALF_FULL;
        } else {
            currentState = STATE_FULL;
        }
    }

    // --- 2. AKTUALIZACJA WYŚWIETLACZA (TYLKO JEŚLI STAN SIĘ ZMIENIŁ) ---
    
    if (currentState != previousState) {
        
        // Wyczyszczenie wiersza tekstu i wiersza paska
        clearRow(0);
        clearRow(1);
        
        lcd.setCursor(0, 0);
        
        switch (currentState) {
            case STATE_ERROR:
                lcd.print("Brak odczytu!");
                barsToShow=0;
                break;
            case STATE_EMPTY:
                lcd.print("Pusto!");
                barsToShow=0;
                break;
            case STATE_LOW:
                lcd.print("Low level");
                barsToShow=4;
                break;
            case STATE_HALF_EMPTY:
                lcd.print("Half empty");
                barsToShow=8;
                break;
            case STATE_HALF_FULL:
                lcd.print("Half full");
                barsToShow=12;
                break;
            case STATE_FULL:
                lcd.print("full!!");
                barsToShow=16;
                break;
        }
        
        // Ustawienie paska wypełnienia (zawsze, gdy stan się zmieni)
        updateBar(barsToShow);
        
        // Zapisz nowy stan
        previousState = currentState;
    }
    
    // Mimo braku zmiany stanu, zawsze aktualizujemy sam pasek, 
    // aby pokazać płynne przejścia w obrębie danego stanu (np. z 5 do 6 bloków)
    // UWAGA: Lepsza optymalizacja to porównanie barsToShow z poprzednią wartością bars
    // updateBar(barsToShow); 
    
    // Jeśli nie potrzebujesz płynnego paska (tylko 4 progi), zostaw aktualizację 
    // paska TYLKO w bloku 'if (currentState != previousState)'
    
    Serial.print("Dystans: ");
    Serial.print(distanceCm);
    Serial.print(" cm | Wyp.: ");
    Serial.print(filled);
    Serial.print(" cm | Stan: ");
    Serial.println(currentState);

    delay(500); // Pomiary co pół sekundy
}