# Colour Mixing Game with LED Feedback

## Overview
This project is an interactive **colour mixing game** using a touchscreen, PWM-controlled RGB LED, and an LCD display. The LED first displays a **random target colour**, and users try to match it by selecting **quantities of five available colours** (Red, Yellow, Blue, White, Black) on the touchscreen. The game calculates a score based on **human visual perception**, rewarding mixes that closely resemble the target LED colour rather than pure comparison to RGB.

---

## Hardware Requirements
- STM32F429ZI Discovery Board  
- LCD_DISCO_F429ZI for display  
- TS_DISCO_F429ZI touchscreen for colour selection  
- RGB LED (pins PE_8, PE_12, PE_14)  
- User button (BUTTON1) to switch between game states  

---

## Features
1. **Touchscreen colour selection**: Users tap coloured boxes to increment the amount of each colour.  
2. **LED colour feedback**: The selected mixture is displayed on the RGB LED.  
3. **Score calculation**: Pressing the user button shows how closely the user’s mixture matches the target LED colour.  
4. **Perceptually accurate scoring**: Accounts for how humans perceive colour, not just RGB differences.

---

## Colour Mixing Logic

### **1. RGB → RYB Conversion**
- Computers use **RGB** (Red-Green-Blue), which is **additive light mixing**.  
- Humans intuitively mix colours like **physical pigments** (Red-Yellow-Blue, RYB).  
- Mixing in RGB often produces unexpected results for humans (e.g., green + red in RGB gives yellow instead of brown which we would expect with paint).  

**Solution:**
1. Convert user-selected RGB colours → **RYB**  
2. Mix colours in **RYB** space  
3. Convert mixed RYB → RGB for LED display  

This ensures that the LED reflects what humans expect when mixing “paint-like” colours.



### **2. CIExy Colour Space for Scoring**
- **RGB is not perceptually uniform**: Some numerically distant colours look similar to humans, and some close colours look different.  
- **CIExy (CIE 1931 colour space)** accounts for human perception:
  - Colours perceived as similar are close in CIExy  
  - Colours perceived as different are farther apart  

**Scoring workflow:**
1. Convert **target LED RGB** → CIExy  
2. Convert **user mixed RGB** → CIExy  
3. Calculate **distance in CIExy space**  
4. Convert distance → **percentage score**  

This results in a score that aligns with **how a human would perceive colour similarity**.

---
## Colour Perception Comparison

| <img src="https://github.com/user-attachments/assets/47a9a11c-f422-4132-a3ed-135f329e191a" height="200"/> | <img src="https://github.com/user-attachments/assets/1550bf22-8ee6-4a3e-bd67-e1be45c88ffb" height="200"/> |
|:---:|:---:|
| **Figure 1:** RGB Scale | **Figure 2:** CIExy Colour Space |

*In the RGB scale, colours like blue and light blue appear far apart numerically, even though humans perceive them as similar. In contrast, the CIExy colour space clusters perceptually similar colours closer together, so human-perceived similarity is better reflected.*

---


## Touchscreen Colour Selection
- **Colour boxes**: Red, Yellow, Blue, White, Black  
- **Counters**: Each tap increments the corresponding colour count.  
- **LED update**: The LED updates in real-time to show the mixed colour.  

---

## Game Flow Diagram
```text
Random LED target → display RGB on LED
        │
        ▼
User taps colours → counters increment
        │
        ▼
Convert user RGB selection → RYB
        │
        ▼
Mix colours in RYB
        │
        ▼
Convert mixed RYB → RGB → update LED
        │
        ▼
Convert both target RGB & user RGB → CIExy
        │
        ▼
Calculate distance → Percentage score → display
