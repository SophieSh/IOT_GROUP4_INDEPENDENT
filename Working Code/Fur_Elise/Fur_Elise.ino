
// -------------------------------------------------------------------
// 1. NOTE FREQUENCY DEFINITIONS (Normally in pitches.h)
//    These #define directives map musical notes to their specific frequencies (Hz).
// -------------------------------------------------------------------

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622 // D#5 is crucial for this melody
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_R   0 // Define R for Rest

// -------------------------------------------------------------------
// 2. SONG DATA: Constants, Melody (Notes), and Tempo (Durations)
// -------------------------------------------------------------------

// Pin the buzzer is connected to
const int melodyPin = 26; 

// Base duration for a whole note (4000ms = 4 seconds, for a quarter note tempo of 60 BPM).
// This is used to calculate all other note durations based on the tempo array.
const long wholenoteDuration = 4000; 

// Note Frequencies for Section A of Fur Elise (Right Hand)
int melody[] = {
  // Bar 1-4
  NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_B4, NOTE_D5, NOTE_C5, NOTE_A4,
  // Bar 5-8
  NOTE_C4, NOTE_E4, NOTE_A4, NOTE_B4, NOTE_E4, NOTE_GS4, NOTE_B4, NOTE_C5,
  // Bar 9-12
  NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_B4, NOTE_D5, NOTE_C5, NOTE_A4,
  // Bar 13-16
  NOTE_C4, NOTE_E4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_R 
};

// Durations of each note (4 = quarter, 8 = eighth, 16 = sixteenth).
// This is critical for getting the rhythm correct.
int tempo[] = {
  // Bar 1-4
  16, 16, 16, 16, 16, 16, 16, 16, 8,
  // Bar 5-8
  8, 8, 8, 8, 8, 8, 8, 8,
  // Bar 9-12
  16, 16, 16, 16, 16, 16, 16, 16, 8,
  // Bar 13-16
  8, 8, 8, 8, 8, 8, 8, 4 // The last note is a rest (R), with a duration of a quarter note (4)
};


// -------------------------------------------------------------------
// 3. ARDUINO SKETCH LOGIC
// -------------------------------------------------------------------

void setup() {
  pinMode(melodyPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Playing Für Elise...");
  
  // Calculate the total number of notes
  int notes = sizeof(melody) / sizeof(melody[0]);
  
  // Iterate through the notes
  for (int thisNote = 0; thisNote < notes; thisNote++) {
    
    // Calculate the total time this note should occupy (sounding time + rest time)
    long noteTotalDuration = wholenoteDuration / tempo[thisNote]; 
    
    // Determine the sounding time (90% of the total duration)
    // Using 0.9 (90%) creates a small, defined gap between notes for separation.
    long soundingTime = (long)(noteTotalDuration * 0.9);
    
    // Determine the rest time (10% of the total duration)
    long restTime = noteTotalDuration - soundingTime;
    
    int noteFrequency = melody[thisNote]; 

    // Play the note using the non-blocking tone() function (two arguments)
    if (noteFrequency > 0) {
      tone(melodyPin, noteFrequency);
    }

    // Wait for the note to sound for the calculated duration
    delay(soundingTime); 

    // Stop the tone playing (critical for separating notes)
    noTone(melodyPin); 

    // Wait for the rest time (the silent gap before the next note)
    delay(restTime);
  }

  Serial.println("Melody finished.");
}

void loop() {
  // The setup() function executes the melody once. 
  // If you want the song to repeat, move the for-loop logic from setup() into loop().
}