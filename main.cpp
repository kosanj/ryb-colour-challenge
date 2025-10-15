#include "LCD_DISCO_F429ZI.h"
#include "TS_DISCO_F429ZI.h"
#include "mbed.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdlib> 
#define COMMON_ANODE

LCD_DISCO_F429ZI LCD;
TS_DISCO_F429ZI TS;

InterruptIn userButton(BUTTON1);

std::vector<int> LED_RGB; //colour that the LED is set to
int flag = 0; // state
int score = 0;

PwmOut led_red(PE_8);
PwmOut led_green(PE_12);
PwmOut led_blue(PE_14);

// Function: Set colour of LED given RGB values
void SetColour(int red, int green, int blue) {
#ifdef COMMON_ANODE
  red = 255 - red;
  green = 255 - green;
  blue = 255 - blue;
#endif
  led_red.pulsewidth_us(red);
  led_green.pulsewidth_us(green);
  led_blue.pulsewidth_us(blue);
}

// Forward declaration of the selectColoursDisplay() function
void selectColoursDisplay();

// Function: Count the number of times each colour is pressed and display it
int red_counter = 0, yellow_counter = 0, blue_counter = 0, white_counter = 0, black_counter = 0;
void displayColourCounts () {
    LCD.SetTextColor(LCD_COLOR_BLACK);
    uint8_t text[80];
    sprintf((char *)text, "R:%d Y:%d B:%d", red_counter, yellow_counter, blue_counter);
    LCD.DisplayStringAt(0, 220, (uint8_t *)&text, CENTER_MODE);
    uint8_t text2[80];
    sprintf((char *)text2, "W:%d Bk:%d", white_counter, black_counter);
    LCD.DisplayStringAt(0, 250, (uint8_t *)&text2, CENTER_MODE);
}

// Colour boxes coordinates of each corner
int rx1 = 20, ry1 = 80, rx2 = rx1 + 50, ry2 = ry1 + 50;
int yx1 = 100, yy1 = 80, yx2 = yx1 + 50, yy2 = yy1 + 50;
int bx1 = 180, by1 = 80, bx2 = bx1 + 50, by2 = by1 + 50;
int wx1 = 60, wy1 = 160, wx2 = wx1 + 50, wy2 = wy1 + 50;
int bkx1 = 150, bky1 = 160, bkx2 = bkx1 + 50, bky2 = bky1 + 50;
// Function: Check if a colour was pressed and call for the screen to update if so
void handleColourPress(uint16_t tsX, uint16_t tsY){
    if (tsX > rx1 && tsX < rx2 && tsY > ry1 && tsY < ry2) { //If red colour is pressed
        red_counter++;
        displayColourCounts();
        thread_sleep_for(300);
    } else if (tsX > yx1 && tsX < yx2 && tsY > yy1 && tsY < yy2) { //If yellow colour is pressed
        yellow_counter++;
        displayColourCounts();
        thread_sleep_for(300);
    } else if (tsX > bx1 && tsX < bx2 && tsY > by1 && tsY < by2) { //If blue colour is pressed
        blue_counter++;
        displayColourCounts();
        thread_sleep_for(300);
    } else if (tsX > wx1 && tsX < wx2 && tsY > wy1 && tsY < wy2) { //If white colour is pressed
        white_counter++;
        displayColourCounts();
        thread_sleep_for(300);
    } else if (tsX > bkx1 && tsX < bkx2 && tsY > bky1 && tsY < bky2) { //If black colour is pressed
        black_counter++;
        displayColourCounts();
        thread_sleep_for(300);
    }
}

// Function: Convert RGB to RYB format
std::vector<double> RGB2RYB(const std::vector<double>& RGBmatrix) {
    // Scaling from 0-255 to 0-1
    std::vector<double> RGBmatrix_scaled = RGBmatrix;
    if (*std::max_element(RGBmatrix_scaled.begin(), RGBmatrix_scaled.end()) > 1.0) {
        for (auto& val : RGBmatrix_scaled) {
            val /= 255.0;
        }
    }

    // Deconstruct vector to components
    double R_rgb = RGBmatrix_scaled[0];
    double G_rgb = RGBmatrix_scaled[1];
    double B_rgb = RGBmatrix_scaled[2];

    // "Remove white" since RGB is additive to get to white
    double I_w = std::min({R_rgb, G_rgb, B_rgb});
    double r_rgb = R_rgb - I_w;
    double g_rgb = G_rgb - I_w;
    double b_rgb = B_rgb - I_w;

    // Calculate ryb values
    double r_ryb = r_rgb - std::min(r_rgb, g_rgb);
    double y_ryb = 0.5 * (g_rgb + std::min(r_rgb, g_rgb));
    double b_ryb = 0.5 * (b_rgb + g_rgb - std::min(r_rgb, g_rgb));
    
    // Normalize values
    double max_ryb = std::max({r_ryb, y_ryb, b_ryb});
    double max_rgb = std::max({r_rgb, g_rgb, b_rgb});

    double n = (max_rgb != 0.0) ? max_ryb / max_rgb : 1.0; // Check if the denominator is zero
    if (n == 0) n = 1;

    double rp_ryb = r_ryb / n;
    double yp_ryb = y_ryb / n;
    double bp_ryb = b_ryb / n;

    // Add "black" since RYB is subtractive to get to black
    double I_b = std::min({1 - R_rgb, 1 - G_rgb, 1 - B_rgb});
    double R_ryb = rp_ryb + I_b;
    double Y_ryb = yp_ryb + I_b;
    double B_ryb = bp_ryb + I_b;

    return {R_ryb, Y_ryb, B_ryb};
}

// Function: Convert RYB to RGB format
std::vector<double> RYB2RGB(const std::vector<double>& RYBmatrix) {
    // Scaling from 0-255 to 0-1
    std::vector<double> RYBmatrix_scaled = RYBmatrix;
    if (*std::max_element(RYBmatrix_scaled.begin(), RYBmatrix_scaled.end()) > 1.0) {
        for (auto& val : RYBmatrix_scaled) {
            val /= 255.0;
        }
    }

    // Deconstruct vector to colour components
    double R_ryb = RYBmatrix_scaled[0];
    double Y_ryb = RYBmatrix_scaled[1];
    double B_ryb = RYBmatrix_scaled[2];

    // Remove "black" since RGB is additive
    double I_b = std::min({R_ryb, Y_ryb, B_ryb});
    double r_ryb = R_ryb - I_b;
    double y_ryb = Y_ryb - I_b;
    double b_ryb = B_ryb - I_b;

    // Calculate rgb values
    double r_rgb = r_ryb + y_ryb - std::min(y_ryb, b_ryb);
    double g_rgb = y_ryb + std::min(y_ryb, b_ryb);
    double b_rgb = 2 * (b_ryb - std::min(y_ryb, b_ryb));
    
    // Normalize values
    double max_ryb = std::max({r_ryb, y_ryb, b_ryb});
    double max_rgb = std::max({r_rgb, g_rgb, b_rgb});
    
    double n = (max_ryb != 0.0) ? max_rgb / max_ryb : 1.0; // Check if the denominator is zero
    if (n == 0) n = 1;
    
    double rp_rgb = r_rgb / n;
    double gp_rgb = g_rgb / n;
    double bp_rgb = b_rgb / n;

    // Add white component since RYB is subtractive
    double I_w = std::min({1 - R_ryb, 1 - Y_ryb, 1 - B_ryb});
    double R_rgb = rp_rgb + I_w;
    double G_rgb = gp_rgb + I_w;
    double B_rgb = bp_rgb + I_w;

    return {R_rgb, G_rgb, B_rgb};
}

// Function: Scale RYB to [0, 255] range like RGB format
std::vector<int> scaleTo255(const std::vector<double>& values) {
    std::vector<int> scaled_values;
    for (double val : values) {
        scaled_values.push_back(static_cast<int>(val * 255));
    }
    return scaled_values;
}

// Function: Mix the selected colours
std::vector<int> mixColours(){

    // RGB codes for the available colours
    std::vector<double> RGB_red = {255,0,0};
    std::vector<double> RGB_yellow = {255,255,0};
    std::vector<double> RGB_blue = {0,0,255};
    std::vector<double> RGB_white = {255,255,255};
    std::vector<double> RGB_black = {0,0,1}; //To prevent / by 0 error

    // RYB codes for the available colours
    std::vector<double> RYB_red = RGB2RYB(RGB_red);
    std::vector<double> RYB_yellow = RGB2RYB(RGB_yellow);
    std::vector<double> RYB_blue = RGB2RYB(RGB_blue);
    std::vector<double> RYB_white = RGB2RYB(RGB_white);
    std::vector<double> RYB_black = RGB2RYB(RGB_black);

    // Mix the RYB codes
    int N = red_counter + yellow_counter + blue_counter + white_counter + black_counter;
    std::vector<double> RYB_mix = {(RYB_red[0]*red_counter + RYB_yellow[0]*yellow_counter + RYB_blue[0]*blue_counter + RYB_white[0]*white_counter + RYB_black[0]*black_counter) / N,
                                    (RYB_red[1]*red_counter + RYB_yellow[1]*yellow_counter + RYB_blue[1]*blue_counter + RYB_white[1]*white_counter + RYB_black[1]*black_counter) / N,
                                    (RYB_red[2]*red_counter + RYB_yellow[2]*yellow_counter + RYB_blue[2]*blue_counter + RYB_white[2]*white_counter + RYB_black[2]*black_counter) / N};

    // Convert the averaged RYB value to RGB
    std::vector<double> averaged_RGB = RYB2RGB(RYB_mix);
    std::vector<int> scaled_RGB = scaleTo255(averaged_RGB);

    return scaled_RGB;
}

// Function (part of CIE conversion): Gamma correction function
double sRGB_to_linear(double c) {
    if (c <= 0.04045)
        return c / 12.92;
    else
        return pow((c + 0.055) / 1.055, 2.4);
}

// Function (part of CIE conversion): Conversion matrix
const double RGBtoXYZ[3][3] = {
    {0.4124564, 0.3575761, 0.1804375},
    {0.2126729, 0.7151522, 0.0721750},
    {0.0193339, 0.1191920, 0.9503041}
};

// Function (part of CIE conversion): Convert RGB to CIE XY format
void RGBtoXY(int rgb[3], double& x, double& y) {
    //Separate r,g,b
    int r = rgb[0];
    int g = rgb[1];
    int b = rgb[2];
    
    // Normalize and gamma correct the RGB values
    double R = sRGB_to_linear(r / 255.0);
    double G = sRGB_to_linear(g / 255.0);
    double B = sRGB_to_linear(b / 255.0);

    // Convert normalized and corrected RGB to CIE_XY format
    double X = RGBtoXYZ[0][0] * R + RGBtoXYZ[0][1] * G + RGBtoXYZ[0][2] * B;
    double Y = RGBtoXYZ[1][0] * R + RGBtoXYZ[1][1] * G + RGBtoXYZ[1][2] * B;
    double Z = RGBtoXYZ[2][0] * R + RGBtoXYZ[2][1] * G + RGBtoXYZ[2][2] * B;

    // Normalize XY values
    double sum = X + Y + Z;
    X /= sum;
    Y /= sum;

    // Output xy values to the memory locations passed in the function definition
    x = X;
    y = Y;
}

// Function: Calculate a colour proximity scpre by comparing 2 CIExy colours
int cmpCIE(double CIE1[2], double CIE2[2]) {
    double deltaX = CIE1[0] - CIE2[0];
    double deltaY = CIE1[1] - CIE2[1];
    
    double distance = sqrt(pow(deltaX,2) + pow(deltaY,2)); //distance between colours in the CIExy colour space
    
    double x, y;
    int rgbmax[] = {255,255,255}; //technically not the maximum location -  sqrt(2) is the max distance between colours but the scores are not harsh enough for my liking
    RGBtoXY(rgbmax, x, y);
    double CIEMAX[] = {x,y};
    double CIEMIN[] = {0,0}; //0,0,0 technically not the minimum location
    double maxDistance = sqrt(pow(CIEMAX[0]-CIEMIN[0],2) + pow(CIEMAX[1]-CIEMIN[1],2));
    
    if(distance!=0){
        return abs((int) ((1-distance/maxDistance)*100)); //calculate percentage difference
    } else {
        return 100; //if distance is 0, it means exact match
    }
}

// Function: ISR to switch between states (either to display score, or go back to main display)
int i = 0;
void userButtonISR(){
    if(i==0){
        i++;

        // Display Score
        flag = 1;
    } else {
        i--;

        // Next round
        flag = 2; 
    }
}

// Function: User colour selection LCD display
void selectColoursDisplay(){
    LCD.Clear(LCD_COLOR_GRAY);

    LCD.DisplayStringAt(0, 40, (uint8_t *)"Mix Colours", CENTER_MODE);
    
    LCD.SetTextColor(LCD_COLOR_RED); //Another colour: 0xFFA95847 (OxFF then the html color code)
    LCD.FillRect(rx1, ry1, 50, 50);
    LCD.SetTextColor(LCD_COLOR_YELLOW);
    LCD.FillRect(yx1, yy1, 50, 50);
    LCD.SetTextColor(LCD_COLOR_BLUE);
    LCD.FillRect(bx1, by1, 50, 50);
    LCD.SetTextColor(LCD_COLOR_WHITE);
    LCD.FillRect(wx1, wy1, 50, 50);
    LCD.SetTextColor(LCD_COLOR_BLACK);
    LCD.FillRect(bkx1, bky1, 50, 50);

    displayColourCounts();
}

// Function: generate a random RGB value between [0,255] for each colour component and set the LED_RGB variable
void generateRandomRGB() {
    // Generate random values for each color component [0, 255]
    int lred = rand() % 256;
    int lgreen = rand() % 256;
    int lblue = rand() % 256;

    LED_RGB.clear();
    LED_RGB.push_back(lred);
    LED_RGB.push_back(lgreen);
    LED_RGB.push_back(lblue);
}

// Main method
int main() {
    // Interrupt ISR initialization
    userButton.fall(&userButtonISR);

    // PWM for LED initialization
    led_red.period_us(255);
    led_green.period_us(255);
    led_blue.period_us(255);

    // Set initial RGB to LED
    generateRandomRGB();
    SetColour(LED_RGB[0], LED_RGB[1], LED_RGB[2]);

    // Touchscreen initialization
    TS_StateTypeDef tsState;
    uint16_t tsX, tsY;

    // Initial LCD formatting
    LCD.SetFont(&Font24);
    LCD.Clear(LCD_COLOR_GRAY);
    LCD.SetTextColor(LCD_COLOR_DARKBLUE);

    // Initial LCD display
    selectColoursDisplay();

    // Main function loop
    while (true) {

        if(flag == 1){ //Display score
            flag = 0;

            // Format scaled_RGB (user selected colour) as an array
            std::vector<int> scaled_RGB = mixColours();
            int scaled_RGB_Array[3];
            for (int i = 0; i < 3; ++i) {
                scaled_RGB_Array[i] = scaled_RGB[i];
            }

            // Format LED_RGB colour as an array
            int LED_RGB_Array[3];
            for (int i = 0; i < 3; ++i) {
                LED_RGB_Array[i] = LED_RGB[i];
            }

            // Calculate score
            double x, y;
            RGBtoXY(scaled_RGB_Array, x, y);
            double CIE1[] = {x,y};
            RGBtoXY(LED_RGB_Array, x, y);
            double CIE2[] = {x,y};    
            score = cmpCIE(CIE1, CIE2);
            printf("SCORE: %d\n", score);

            // Show score
            LCD.Clear(LCD_COLOR_GRAY);
            uint8_t scoreBuff[30];
            sprintf((char *)scoreBuff, "Score: %d%%", score);
            LCD.DisplayStringAt(0, 40, (uint8_t *)&scoreBuff, CENTER_MODE);

        } else if(flag == 2){ //Go back to mixing colours screen
            flag = 0;

            // Reset all colour counts
            red_counter = 0;
            yellow_counter = 0;
            blue_counter = 0;
            black_counter = 0;
            white_counter = 0;

            // New LED colour
            generateRandomRGB();
            SetColour(LED_RGB[0], LED_RGB[1], LED_RGB[2]);
            ThisThread::sleep_for(10ms);

            selectColoursDisplay();
        }

        TS.GetState(&tsState);
        if (tsState.TouchDetected) { //Get the xy coordinates of the spot touched
            tsX = tsState.X;
            tsY = 320 - tsState .Y;

            handleColourPress(tsX, tsY); 
        }
    }
}