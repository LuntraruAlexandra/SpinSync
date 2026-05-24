#include <Arduino.h>
#include <AccelStepper.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7789.h> 
#include <SPI.h>
#include <RF24.h>

#define BUT1 A0
#define BUT2 A1
#define BUT3 A2


#define IR_RECEIVER A4
#define BUZ         A5


#define DIR         2
#define STP         3


#define CS          4
#define DC          5
#define RST         6
#define BL          9  

RF24 radio(7, 8); 
const byte adresa[6] = "00001"; 


const int STEPS_PER_REV = 200; 
const int NUM_LEDS = 8;

const int TARGET_POSITIONS[NUM_LEDS] = {
    0,                             
    (1 * STEPS_PER_REV) / NUM_LEDS,  
    (2 * STEPS_PER_REV) / NUM_LEDS,  
    (3 * STEPS_PER_REV) / NUM_LEDS,  
    (4 * STEPS_PER_REV) / NUM_LEDS,  
    (5 * STEPS_PER_REV) / NUM_LEDS,  
    (6 * STEPS_PER_REV) / NUM_LEDS,  
    (7 * STEPS_PER_REV) / NUM_LEDS   
};

const int TOLERANCE = 15; 

AccelStepper stepper(AccelStepper::DRIVER, STP, DIR);
Adafruit_ST7789 display = Adafruit_ST7789(CS, DC, RST);


enum GameState { START_MENU, CALIBRATION, SELECT_MODE, PLAYING, GAME_OVER, WIN };
GameState currentState = START_MENU;

int score = 0;
float gameSpeed = 40.0;     
float gameAccel = 15.0;     

bool ledStates[NUM_LEDS] = {false, false, false, false, false, false, false, false};

unsigned long gameStartTime = 0;
unsigned long gameDurationMs = 0;


void triggerBuzzer(int frequency, int duration) {
    tone(BUZ, frequency, duration);
}

void updateDisplay(String title, String msg1, String msg2, uint16_t color) {
    display.fillScreen(ST77XX_BLACK);
    display.setCursor(10, 20);
    display.setTextColor(color);
    display.setTextSize(2);
    display.println(title);
    
    display.setCursor(10, 60);
    display.setTextColor(ST77XX_WHITE);
    display.setTextSize(1);
    display.println(msg1);
    
    display.setCursor(10, 100);
    display.println(msg2);
}

void drawScore() {
    display.fillRect(10, 140, 220, 40, ST77XX_BLACK); 
    display.setCursor(10, 140);
    display.setTextColor(ST77XX_YELLOW);
    display.setTextSize(2);
    display.print("SCOR: ");
    display.print(score);
}

void resetLeds() {
    for (int i = 0; i < NUM_LEDS; i++) {
        ledStates[i] = false;
    }
    int resetCmd = 0;
    radio.write(&resetCmd, sizeof(resetCmd));
}


void setup() {
    Serial.begin(115200);
    
    radio.begin();
    radio.openWritingPipe(adresa);
    radio.setPALevel(RF24_PA_LOW); 
    radio.stopListening(); 

    pinMode(BUT1, INPUT_PULLUP);
    pinMode(BUT2, INPUT_PULLUP);
    pinMode(BUT3, INPUT_PULLUP);
    pinMode(IR_RECEIVER, INPUT);
    pinMode(BUZ, OUTPUT);
    
    if (BL != -1) {
        pinMode(BL, OUTPUT);
        digitalWrite(BL, HIGH); 
    }

    display.init(240, 240);
    display.setRotation(0);
    
    stepper.setMaxSpeed(40.0);        
    stepper.setAcceleration(15.0);    
    
    updateDisplay("SpinSync", "Apasă BUT1 (START)", "Pregătește calibrarea...", ST77XX_GREEN);
}

void loop() {
  
    if (currentState == PLAYING || currentState == CALIBRATION) {
        stepper.run();
    }

    switch (currentState) {
        
        case START_MENU:
            if (digitalRead(BUT1) == LOW) {
                delay(200); 
                triggerBuzzer(1000, 100);
                score = 0; 
                resetLeds();

                stepper.stop();
                stepper.setCurrentPosition(0);
               
                stepper.setMaxSpeed(30.0);
                stepper.setSpeed(30.0);
                stepper.moveTo(40000); 
                
                updateDisplay("CALIBRARE", "Când LED 1 e la stativ", "Apasă iar BUT1!", ST77XX_ORANGE);
                currentState = CALIBRATION;
            }
            break;

        case CALIBRATION:
            if (digitalRead(BUT1) == LOW) {
                delay(200);
                stepper.stop(); 
                stepper.setCurrentPosition(0); 
                triggerBuzzer(1500, 300);
                
                display.fillScreen(ST77XX_BLACK);
                display.setCursor(10, 20);
                display.setTextColor(ST77XX_MAGENTA);
                display.setTextSize(2);
                display.println("ALEGE VITEZA:");
                display.setTextColor(ST77XX_WHITE);
                display.setTextSize(1);
                display.setCursor(10, 60);  display.println("BUT1 -> LENT (Usor)");
                display.setCursor(10, 85);  display.println("BUT2 -> MEDIU (Normal)");
                display.setCursor(10, 110); display.println("BUT3 -> RAPID (Greu)");
                
                currentState = SELECT_MODE;
            }
            break;

        case SELECT_MODE:
            if (digitalRead(BUT1) == LOW) {
                gameSpeed = 35.0; 
                gameAccel = 500.0; 
                triggerBuzzer(800, 150); delay(200);
                currentState = PLAYING;
            } 
            else if (digitalRead(BUT2) == LOW) {
                gameSpeed = 65.0; 
                gameAccel = 600.0; 
                triggerBuzzer(1000, 150); delay(200);
                currentState = PLAYING;
            } 
            else if (digitalRead(BUT3) == LOW) {
                gameSpeed = 110.0; 
                gameAccel = 45.0;  
                triggerBuzzer(1200, 150); delay(200);
                currentState = PLAYING;
            }

            if (currentState == PLAYING) {
                stepper.setCurrentPosition(0);
                stepper.setMaxSpeed(gameSpeed);     
                stepper.setAcceleration(gameAccel);  
                stepper.moveTo(40000); 
                stepper.run(); 
                
                updateDisplay("JOC PORNIT!", "Trage cu telecomanda IR", "când un LED e la țintă", ST77XX_BLUE);
                drawScore();
                
                gameStartTime = millis();
            }
            break;

        case PLAYING:
            
            if (stepper.distanceToGo() < 200) {
                stepper.moveTo(stepper.currentPosition() + 10000);
            }

            if (digitalRead(IR_RECEIVER) == LOW) {
                long currentPos = stepper.currentPosition();
                long normalizedPos = currentPos % STEPS_PER_REV;
                if (normalizedPos < 0) normalizedPos += STEPS_PER_REV;

                bool hit = false;
                int targetedLedIndex = -1;
                
                for (int i = 0; i < NUM_LEDS; i++) {
                    long targetLedPos = TARGET_POSITIONS[i];
                    long diff = abs(normalizedPos - targetLedPos);
                    if (diff > STEPS_PER_REV / 2) {
                        diff = STEPS_PER_REV - diff;
                    }

                    if (diff <= TOLERANCE) {
                        hit = true;
                        targetedLedIndex = i;
                        break; 
                    }
                }

                if (hit) {
                    if (ledStates[targetedLedIndex] == false) {
                        ledStates[targetedLedIndex] = true; 
                        score++; 
                        
                        int ledDeAprins = targetedLedIndex + 1; 
                        radio.write(&ledDeAprins, sizeof(ledDeAprins));
                        
                        triggerBuzzer(2000, 60); 
                        drawScore();

                        if (score >= NUM_LEDS) {
                            stepper.stop();
                            gameDurationMs = millis() - gameStartTime;
                            float seconds = gameDurationMs / 1000.0;
                            
                            long finalPerformanceScore = (gameSpeed * 10) + (1000 / seconds);

                            int winCmd = 99;
                            radio.write(&winCmd, sizeof(winCmd));

                            triggerBuzzer(1500, 200); delay(150);
                            triggerBuzzer(1800, 200); delay(150);
                            triggerBuzzer(2200, 500);

                            String timeMsg = "Timp: " + String(seconds, 1) + " secunde";
                            String scoreMsg = "Scor Fin: " + String(finalPerformanceScore);
                            updateDisplay("VICTORIE COMPLETA!", timeMsg, scoreMsg, ST77XX_GREEN);
                            
                            display.setCursor(10, 150);
                            display.setTextColor(ST77XX_ORANGE);
                            display.setTextSize(1);
                            display.println("Apasă ORICE BUTON (1/2/3)");
                            display.setCursor(10, 175);
                            display.println("pentru a reveni la Meniu");

                            currentState = WIN;
                            break;
                        }
                    } 
                    else {
                        triggerBuzzer(1400, 30); 
                    }
                    delay(250); 
                } 
                else {
                    stepper.stop();
                    triggerBuzzer(300, 600); 
                    
                    resetLeds(); 
                    
                    String finalScoreMsg = "Scor final: " + String(score);
                    updateDisplay("AI PIERDUT!", finalScoreMsg, "BUT1 pentru Meniu", ST77XX_RED);
                    
                    currentState = GAME_OVER;
                }
            }
            break;

        case GAME_OVER:
            if (digitalRead(BUT1) == LOW) {
                delay(200);
                currentState = START_MENU;
                resetLeds();
                updateDisplay("SpinSync", "Apasă BUT1 (START)", "Pregătește calibrarea...", ST77XX_GREEN);
            }
            break;

        case WIN:
            if (digitalRead(BUT1) == LOW || digitalRead(BUT2) == LOW || digitalRead(BUT3) == LOW) {
                delay(250); 
                triggerBuzzer(1000, 100);
                resetLeds(); 
                currentState = START_MENU;
                updateDisplay("SpinSync", "Apasă BUT1 (START)", "Pregătește calibrarea...", ST77XX_GREEN);
            }
            break;
    }
}