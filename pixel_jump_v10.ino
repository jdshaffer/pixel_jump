//------------------------------------------------------------------------------------
// Pixel Jump (v1.0)
// for HW-364a and HW-364b development boards 
// Jeffrey D. Shaffer & Google Gemini
// 2025-08-17
//
//------------------------------------------------------------------------------------
// A simple side-scroller game.
//
// Use the FLASH button to jump.
// Hold the button to jump higher.
//
//------------------------------------------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Configuration
#define SCREEN_WIDTH 128          // OLED display width, in pixels
#define SCREEN_HEIGHT 64          // OLED display height, in pixels
#define OLED_RESET -1             // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C       // The I2C address of the display
#define OLED_SDA_PIN 14           // Correct SDA pin for your wiring (D6 on most boards)
#define OLED_SCL_PIN 12           // Correct SCL pin for your wiring (D5 on most boards)

// Button pin
#define BUTTON_PIN 0              // Use the "Flash" button (GPIO0)

// The display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Game constants
const int playerWidth = 8;
const int playerHeight = 8;
const int groundY = SCREEN_HEIGHT - 10;
const int groundHeight = 5;
const float jumpVelocity = -5.0;  // This value is now the maximum possible jump force.
const float gravity = 0.2;        // Downward pull

// Player variables
int playerX = SCREEN_WIDTH / 4;
int playerY = groundY - playerHeight;
float playerVelY = 0;
bool jumpActive = false;

// Obstacle variables
const int maxObstacles = 5;
int obstacleX[maxObstacles];
int obstacleWidth[maxObstacles];
bool obstacleActive[maxObstacles];
int obstacleSpeed = 3;            // Increased speed for challenge
long score = 0;
long highScore = 0;

// For blinking text
unsigned long blinkTimer = 0;
bool showText = true;
const unsigned long blinkInterval = 500; // Blink every 500 milliseconds

// Game state management
enum GameState { START_SCREEN, PLAYING, GAME_OVER };
GameState currentState;
unsigned long gameOverTime; // To store the time when the game over screen appears

// --- Function Declarations ---
void resetGame();
void updateGame();
void drawGame();
void drawGameOver();
void drawStartScreen();

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // A failed display is a fatal error for this device, so we halt
    for (;;);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  resetGame();
}

void loop() {
  switch (currentState) {
    case START_SCREEN:
      drawStartScreen();
      if (digitalRead(BUTTON_PIN) == LOW) {
        delay(500);
        currentState = PLAYING;
      }
      break;
    case PLAYING:
      updateGame();
      drawGame();
      break;
    case GAME_OVER:
      drawGameOver();
      // On GameOver, ignore the jump button for one second (to prevent accidental restarts)
      if (millis() - gameOverTime > 1000 && digitalRead(BUTTON_PIN) == LOW) {
        delay(500);
        resetGame();
        currentState = PLAYING;   // This starts a new game immediately (skips startup screen)
      }
      break;
  }
}

// --- Game Logic Functions ---
void updateGame() {
  // Check for button press to initiate a jump
  if (digitalRead(BUTTON_PIN) == LOW && !jumpActive) {
    jumpActive = true;
    playerVelY = -2.0; // A small, initial jump velocity
  }

  // If in the air and holding the button, apply jump boost
  if (jumpActive && digitalRead(BUTTON_PIN) == LOW) {
    if (playerY > 5) { // Cap height to prevent going off-screen
      playerVelY += -0.1; // A small, continuous negative boost
    }
  }

  // Apply velocity and gravity
  if (jumpActive) {
    playerY += playerVelY;
    playerVelY += gravity;

    // Check if player has landed
    if (playerY >= groundY - playerHeight) {
      playerY = groundY - playerHeight;
      jumpActive = false;
      playerVelY = 0;
    }
  }

  // Move obstacles
  for (int i = 0; i < maxObstacles; i++) {
    if (obstacleActive[i]) {
      obstacleX[i] -= obstacleSpeed;

      // Check for collision (Treating obstacles as holes)
      if (playerX > obstacleX[i] &&  // Player left must barely touch before hole (major overhang)
          playerX + playerWidth < obstacleX[i] + obstacleWidth[i] &&   // Player right must barely touch after hole
          playerY + playerHeight >= groundY) {                         // Player bottom
          currentState = GAME_OVER;
          gameOverTime = millis();
          if (score > highScore) {
              highScore = score;
          }
      }

      // Check if obstacle is off-screen
      if (obstacleX[i] + obstacleWidth[i] < 0) {
        obstacleActive[i] = false;
        score++;
      }
    }
  }

  // Generate new obstacles
  for (int i = 0; i < maxObstacles; i++) {
    if (!obstacleActive[i]) {
      if (random(0, 250) < 5) { // Adjusted second number to change frequency
        obstacleActive[i] = true;
        obstacleX[i] = SCREEN_WIDTH;
        obstacleWidth[i] = random(10, 40);
        break; // Only spawn one at a time
      }
    }
  }
}

// --- Drawing Functions ---
void drawGame() {
  display.clearDisplay();

  // Draw the ground
  display.fillRect(0, groundY, SCREEN_WIDTH, groundHeight, WHITE);

  // Draw the player
  display.fillRect(playerX, playerY, playerWidth, playerHeight, WHITE);

  // Draw obstacles
  for (int i = 0; i < maxObstacles; i++) {
    if (obstacleActive[i]) {
      display.fillRect(obstacleX[i], groundY, obstacleWidth[i], groundHeight, BLACK);
    }
  }

  // Draw the score
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(5, 5);
  display.print("Score: ");
  display.print(score);

  display.display();
}

void drawGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(15, 0);
  display.println("GAME OVER");

  display.setTextSize(1);
  display.setCursor(20, 22);
  display.print("Score: ");
  display.print(score);
  display.setCursor(20, 32);
  display.print("High Score: ");
  display.print(highScore);
  // Blinking "Press to Restart" message -------------
  if (millis() - blinkTimer > blinkInterval) {
    blinkTimer = millis();
    showText = !showText;
  }
  if (showText) {
    display.setCursor(20, 50);
    display.println("Press to Restart");
  }
  // ----------------------------------------------
  display.display();
}

void drawStartScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(5, 0);
  display.println("PIXEL JUMP");
  display.setTextSize(1);
  display.setCursor(20, 22);
  display.println("by Jds & Gemini");
  // Blinking "Press to Play" message -------------
  if (millis() - blinkTimer > blinkInterval) {
    blinkTimer = millis();
    showText = !showText;
  }
  if (showText) {
    display.setCursor(24, 44);
    display.println("Press to Play");
  }
  // ----------------------------------------------
  display.display();
}

// --- Utility Functions ---
void resetGame() {
  currentState = START_SCREEN;
  score = 0;
  playerY = groundY - playerHeight;
  playerVelY = 0;
  jumpActive = false;

  for (int i = 0; i < maxObstacles; i++) {
    obstacleActive[i] = false;
  }
}
