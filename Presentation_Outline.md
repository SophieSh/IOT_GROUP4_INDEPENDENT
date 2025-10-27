# Project Presentation Outline

**Total Time: ~20-23 minutes**

---

## 1. System Overview (2-3 minutes) Sofiya
- Introduction to the application
- Hardware setup (ESP32, display, SD card)
- Basic game flow and user interaction
- What makes it educational for children

---

## 2. Edge Cases & Challenges (2-3 minutes)
- ESP32 memory limitations (~520KB RAM)
- How you worked around memory constraints
- Watchdog timer management
- Initial difficulties and debugging

---

## 3. Canvas Feature - Yonatan (3 minutes)
- Touch drawing implementation
- Real-time drawing feedback
- Clear button functionality
- Sending canvas data to server via WiFi
- User experience for writing practice

---

## 4. Tab System / Sliding - Team (3 minutes)
- Tab-based UI organization
- Multiple input methods (letter buttons, keyboard, canvas)
- Navigation flow
- LVGL TabView implementation

---

## 5. Multilingual Support (3-5 minutes)
**Speaker: Sophie**
- Hebrew and English language selection
- Font handling (DejaVu for Hebrew, Montserrat for English)
- Custom Hebrew keyboard implementation
- SD card directory structure (`S:/images/english` vs `S:/images/hebrew`)
- UTF-8 multi-byte character handling
- LVGL configuration for BIDI text

---

## 6. Timer Bar Feature (3-5 minutes)
- Visual countdown implementation
- LVGL animation system
- 30-second countdown with color progression
- Auto-advance on timeout
- Sound feedback integration

---

## 7. Scoring Feature (3-5 minutes)
**Recently Added - Highlight this!**
- Correct vs wrong answer tracking
- Results screen with score breakdown
- Timeouts counted as incorrect answers
- "Play Again" functionality
- Game completion detection

---

## 8. Project Management (3 minutes)
- Initial challenges and getting stuck
- How you overcame obstacles
- Team coordination and work division
- Development timeline
- Lessons learned

---

## 9. Questions & Answers (3 minutes)
- Open to audience questions
- Be prepared to explain:
  - Technical implementation details
  - Design decisions
  - Future improvements

---

## Tips for Success
- **Practice the timing** - aim for 20-23 minutes total
- **Highlight scoring** as the newest feature
- **Demonstrate the hardware** if possible (live or video)
- **Be ready to dig deeper** on any technical aspect
- **Show enthusiasm** for the educational impact

