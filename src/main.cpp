#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Configuration des rubans LED
#define PIN1 9          // Broche de données ruban 1
#define NUM_LEDS1 14    // Nombre de LEDs ruban 1
#define PIN2 7          // Broche de données ruban 2
#define NUM_LEDS2 14    // Nombre de LEDs ruban 2
#define PIN3 12         // Broche de données ruban 3
#define NUM_LEDS3 15    // Nombre de LEDs ruban 3
#define BRIGHTNESS 50   // Luminosité (0-255)

// Configuration des boutons
#define BUTTON1 2       // Bouton pour ruban 1
#define BUTTON2 3       // Bouton pour ruban 2
#define BUTTON3 4       // Bouton pour ruban 3

// Initialisation des rubans LED
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS1, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS2, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3 = Adafruit_NeoPixel(NUM_LEDS3, PIN3, NEO_GRB + NEO_KHZ800);

// Variables pour l'animation
unsigned long previousMillis = 0;
const long interval = 150;  // Vitesse de défilement (ms)

// Positions des tuiles (blocs de LEDs)
int tile1Position = -1;
int tile2Position = -1;
int tile3Position = -1;

const int tileSize = 3;  // Taille d'une tuile
const int hitZone = 2;   // Zone de détection en fin de ruban (dernières LEDs)

// Variables de jeu
int score = 0;
bool gameOver = false;
unsigned long spawnTimer = 0;
const long spawnInterval = 1500;  // Temps entre l'apparition des tuiles

// Variables anti-rebond
unsigned long lastButton1Press = 0;
unsigned long lastButton2Press = 0;
unsigned long lastButton3Press = 0;
const long debounceDelay = 200;

void setup() {
  Serial.begin(9600);
  
  // Initialisation des rubans
  strip1.begin();
  strip1.setBrightness(BRIGHTNESS);
  strip1.show();
  
  strip2.begin();
  strip2.setBrightness(BRIGHTNESS);
  strip2.show();

  strip3.begin();
  strip3.setBrightness(BRIGHTNESS);
  strip3.show();
  
  // Configuration des boutons avec résistance pull-up interne
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  
  Serial.println("=== PIANO TILES LED ===");
  Serial.println("Appuyez sur les boutons quand les tuiles arrivent en bas!");
  Serial.println("Score: 0");
  
  randomSeed(analogRead(A0));
}

void loop() {
  if (gameOver) {
    showGameOver();
    return;
  }
  
  unsigned long currentMillis = millis();
  
  // Gestion de l'apparition des tuiles
  if (currentMillis - spawnTimer >= spawnInterval) {
    spawnTimer = currentMillis;
    spawnRandomTile();
  }
  
  // Gestion du défilement
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Efface tout
    strip1.clear();
    strip2.clear();
    strip3.clear();
    
    // Dessine et déplace les tuiles
    moveTile(strip1, tile1Position, NUM_LEDS1, strip1.Color(0, 100, 255));  // Bleu
    moveTile(strip2, tile2Position, NUM_LEDS2, strip2.Color(255, 0, 150));  // Rose
    moveTile(strip3, tile3Position, NUM_LEDS3, strip3.Color(0, 255, 0));    // Vert
    
    // Affiche les changements
    strip1.show();
    strip2.show();
    strip3.show();
    
    // Avance les tuiles
    if (tile1Position >= 0) tile1Position++;
    if (tile2Position >= 0) tile2Position++;
    if (tile3Position >= 0) tile3Position++;
    
    // Vérifie si une tuile est ratée (sort du ruban sans être cliquée)
    if (tile1Position >= NUM_LEDS1) {
      gameOver = true;
      Serial.println("GAME OVER - Tuile ratée sur le ruban 1!");
    }
    if (tile2Position >= NUM_LEDS2) {
      gameOver = true;
      Serial.println("GAME OVER - Tuile ratée sur le ruban 2!");
    }
    if (tile3Position >= NUM_LEDS3) {
      gameOver = true;
      Serial.println("GAME OVER - Tuile ratée sur le ruban 3!");
    }
  }
  
  // Gestion des boutons
  checkButton(BUTTON1, tile1Position, NUM_LEDS1, lastButton1Press, currentMillis, "Ruban 1");
  checkButton(BUTTON2, tile2Position, NUM_LEDS2, lastButton2Press, currentMillis, "Ruban 2");
  checkButton(BUTTON3, tile3Position, NUM_LEDS3, lastButton3Press, currentMillis, "Ruban 3");
}

void moveTile(Adafruit_NeoPixel &strip, int position, int numLeds, uint32_t color) {
  if (position < 0) return;
  
  for (int i = 0; i < tileSize; i++) {
    int pos = position + i;
    if (pos >= 0 && pos < numLeds) {
      strip.setPixelColor(pos, color);
    }
  }
}

void spawnRandomTile() {
  int lane = random(1, 4);  // Choisit aléatoirement entre 1, 2 ou 3
  
  if (lane == 1 && tile1Position < 0) {
    tile1Position = 0;
  } else if (lane == 2 && tile2Position < 0) {
    tile2Position = 0;
  } else if (lane == 3 && tile3Position < 0) {
    tile3Position = 0;
  }
}

void checkButton(int buttonPin, int &tilePosition, int numLeds, unsigned long &lastPress, unsigned long currentMillis, const char* stripName) {
  if (digitalRead(buttonPin) == LOW && (currentMillis - lastPress) > debounceDelay) {
    lastPress = currentMillis;
    
    // Vérifie si la tuile est dans la zone de hit (fin du ruban)
    // La tuile doit être dans les dernières positions
    int tileEnd = tilePosition + tileSize - 1;  // Dernière LED de la tuile
    int hitZoneStart = numLeds - hitZone - 1;   // Début de la zone acceptable
    
    if (tilePosition >= 0 && tileEnd >= hitZoneStart && tilePosition < numLeds) {
      // Bon timing!
      score++;
      tilePosition = -1;  // Supprime la tuile
      Serial.print("HIT! Score: ");
      Serial.println(score);
    } else if (tilePosition >= 0) {
      // Il y a une tuile active mais mauvais timing - Game Over
      gameOver = true;
      Serial.print("GAME OVER - Mauvais timing sur ");
      Serial.println(stripName);
    }
    // Si pas de tuile active (tilePosition == -1), on ignore le clic
  }
}

void showGameOver() {
  // Allume tous les rubans en rouge
  for (int i = 0; i < NUM_LEDS1; i++) {
    strip1.setPixelColor(i, strip1.Color(255, 0, 0));
  }
  for (int i = 0; i < NUM_LEDS2; i++) {
    strip2.setPixelColor(i, strip2.Color(255, 0, 0));
  }
  for (int i = 0; i < NUM_LEDS3; i++) {
    strip3.setPixelColor(i, strip3.Color(255, 0, 0));
  }
  
  strip1.show();
  strip2.show();
  strip3.show();
  
  Serial.println("===================");
  Serial.print("SCORE FINAL: ");
  Serial.println(score);
  Serial.println("===================");
  
  // Reste bloqué ici
  while(true) {
    // Game over permanent
  }
}