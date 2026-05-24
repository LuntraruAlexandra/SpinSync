#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#define LED1 3   
#define LED2 5   
#define LED3 A5  
#define LED4 A4  
#define LED5 9   
#define LED6 6 
#define LED7 A0  
#define LED8 A1  

int led_array[] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};


int analog_leds[] = {LED3, LED4, LED7, LED8};
int pwm_leds[] = {LED1, LED2, LED5, LED6};

RF24 radio(7, 8); 
const byte adresa[6] = "00001";


bool winMode = false;

unsigned long lastBlinkTime = 0;
bool blinkState = false;
const unsigned long BLINK_INTERVAL = 250; 

unsigned long lastFadeTime = 0;
int fadeAmount = 5;       
int currentFadeValue = 0; 
const unsigned long FADE_INTERVAL = 15;  

void setup() {
    for (int i = 0; i < 8; i++) {
        pinMode(led_array[i], OUTPUT);
        digitalWrite(led_array[i], LOW);
    }
    
    radio.begin();
    radio.openReadingPipe(1, adresa);

radio.setPALevel(RF24_PA_MIN); 
    radio.startListening(); 
}

void loop() {
    if (radio.available()) {
        int ledIndex = 0;
        radio.read(&ledIndex, sizeof(ledIndex));
        
        
        if (ledIndex == 0) {
            winMode = false;
            for (int i = 0; i < 8; i++) {
                digitalWrite(led_array[i], LOW);
            }
        }
        
        else if (ledIndex == 99) {
            winMode = true;
            lastBlinkTime = millis();
            lastFadeTime = millis();
            currentFadeValue = 0;
            blinkState = HIGH;
        }

        else if (!winMode && ledIndex >= 1 && ledIndex <= 8) {
            digitalWrite(led_array[ledIndex - 1], HIGH);
        }
    }


    if (winMode) {
        unsigned long currentTime = millis();

   
        if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            
            for (int i = 0; i < 4; i++) {
                digitalWrite(analog_leds[i], blinkState);
            }
        }


        if (currentTime - lastFadeTime >= FADE_INTERVAL) {
            lastFadeTime = currentTime;
            
            currentFadeValue += fadeAmount;
            

            if (currentFadeValue <= 0 || currentFadeValue >= 255) {
                fadeAmount = -fadeAmount; 
            }
        
            for (int i = 0; i < 4; i++) {
                analogWrite(pwm_leds[i], currentFadeValue);
            }
        }
    }
}