//
// Created by Yonatan, Gital, Sofiya on 11/10/2025.
//

#ifndef SRC_SOUND_H
#define SRC_SOUND_H

#include <Arduino.h> 

// Using Channel 3 to avoid conflict with TFT Backlight (Channel 0)
#define BUZZER_CHANNEL 3
#define BUZZER_RESOLUTION 10 // 10 bits is standard for tone generation
#define BUZZER_FREQ_MAX 5000 // Max frequency

// --- Note Frequencies ---
#define NOTE_C3  131 // Low C
#define NOTE_CS3 139 // C-sharp
#define NOTE_C5  523 // Middle C 
#define NOTE_E5  659 
#define NOTE_G5  784 
#define NOTE_C6  1047 // High C
#define NOTE_R   0    // Rest

// Pin the buzzer is connected to
const int melodyPin = 26;


void _playNote(int frequency, long duration, long restDuration) {
  if (frequency > 0) ledcWriteTone(BUZZER_CHANNEL, frequency);
  else ledcWrite(BUZZER_CHANNEL, 0); 
  
  delay(duration); 
  ledcWrite(BUZZER_CHANNEL, 0); 
  delay(restDuration);
}



void playVictorySound() {
    const long shortDuration = 70;
    const long quickRest = 10;

    _playNote(NOTE_C5, shortDuration, quickRest);
    _playNote(NOTE_E5, shortDuration, quickRest);
    _playNote(NOTE_G5, shortDuration, quickRest);
    _playNote(NOTE_C6, 300, 50); 
}


void playLossSound() {
    const long noteDuration = 150; 
    const long restDuration = 30; // Very short rest for snappy sequence

    _playNote(NOTE_C3, noteDuration, restDuration / 2);  
    _playNote(NOTE_C3, noteDuration, restDuration / 2);  
    _playNote(NOTE_C3, noteDuration, restDuration/ 2 ); 
}

#endif //SRC_SOUND_H