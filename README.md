## TODO
Complete installation instructions.

## AlphaLearn Project by : Yonatan Rappoport, Gital Monisov, Sofiya Shterenson
AlphaLearn is a compact educational game that helps children take their first steps in learning to read, write, and recognize letters. Built with LVGL, it runs on small displays and supports multiple learning styles — including visual learning through images, typing with a keyboard, and practicing on a handwriting canvas.

The interface is child-friendly and easy to navigate, encouraging playful exploration. AlphaLearn provides real-time feedback through both sound and visuals to help children track their progress and correct mistakes instantly. The app includes several difficulty levels that gradually increase in complexity, supporting a step-by-step learning experience.

## Detailed description :
When the user (the child) opens the app, they can choose between two languages — Hebrew and English — and then the game begins with the first image displayed. The user is required to provide the first letter of the object shown in the image. There are several ways to play the game:

1. Selecting a letter from a set of eight letters displayed on the screen.
2. Choosing a letter from a keyboard that includes all the letters of the alphabet.
3. Writing a letter on a canvas (without an option to check it). The marks made by the user are displayed in real time, and there is also an option to erase them.
   
Each game mode is time-limited (30 seconds per answer), with a progress bar shown at the bottom of the screen. Options 1 and 2 provide real-time feedback, displaying a pop-up message to inform the user whether their answer was correct or incorrect, accompanied by a sound indicating success or failure, respectively.
 
## Folder description :
* ESP32: source code for the esp side (firmware), and necessary header files
* Documentation: wiring curcuit diagram image + basic operating instructions
* Unit Tests: tests for individual hardware components: input/output devices
* Parameters: description for settings which you can be modified IN THE CODE

## Arduino/ESP32 libraries used in this project:
* LVGL - version 8.3.11
* GFX Library for Arduino - version 1.6.1

## HardWare:
* ESP32-2432S032 (CYD LCD touch display)
* Simple buzzer


## Connection diagram:
![alt text]("Documentation/connection diagram/circuit_diagram.png")

## Project Poster:
 
This project is part of ICST - The Interdisciplinary Center for Smart Technologies, Taub Faculty of Computer Science, Technion
https://icst.cs.technion.ac.il/
