#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <Beacon.h>

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define PIN 11
#define NUMPIXELS 38
#define SPEED 2
#define PIN_2 12
#define PIN_3 13
#define PIN_4 14
#define PIN_LEFT1 15
#define PIN_LEFT2 16
#define PIN_LEFT3 17
#define PIN_RIGHT1 10
#define PIN_RIGHT2 9
#define PIN_RIGHT3 8
#define BUTTON_LEFT 19
#define BUTTON_RIGHT 20
#define BUTTON_ROTATE 21
#define BUTTON_SPEEDUP 18
#define BUTTON_START A0
#define BUTTON_DROP 3
// new pins for shape prediction
#define PINPREL A6
#define PINPRER A7

#define SD_ChipSelectPin 53 // example uses hardware SS pin 53 on Mega2560

Adafruit_NeoPixel col1(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel col2(NUMPIXELS, PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel col3(NUMPIXELS, PIN_3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel col4(NUMPIXELS, PIN_4, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel colL1(NUMPIXELS, PIN_LEFT1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel colL2(NUMPIXELS, PIN_LEFT2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel colL3(NUMPIXELS, PIN_LEFT3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel colR1(NUMPIXELS, PIN_RIGHT1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel colR2(NUMPIXELS, PIN_RIGHT2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel colR3(NUMPIXELS, PIN_RIGHT3, NEO_GRB + NEO_KHZ800);
// new cols for shape prediction
Adafruit_NeoPixel colPL(NUMPIXELS, PINPREL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel colPR(NUMPIXELS, PINPRER, NEO_GRB + NEO_KHZ800);
// L rgb(220,0,0) 1
// I rgb(0,0,255) 2
// O rgb(0,128,0) 3
// Z rgb(255,255,0) 4
// T rgb(255,0,255) 5

enum colours : long
{
    BLACK = 0x000000,
    WHITE = 0xFFFFFF,
    RED = 0xFF0000,
    BLUE = 0x00FFFF,
    GREEN = 0x00FF00,
    YELLOW = 0xFFFF00,
    PURPLE = 0xFF00FF,
};

#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels
int centerx = 0;
int centery1 = PIN;
int centery2 = PIN_2;
int centery3 = PIN_3;
int centery4 = PIN_4;
int highestScore = 0;

// centery1b=centery1before
int centery1b = PIN;
int centery2b = PIN_2;
int centery3b = PIN_3;
int centery4b = PIN_4;

int currentStateL;
int currentStateR; // the current reading from the input pin
int currentRotateState;
int SpeedUp;
int currentStateStart;
int currentStateDrop;
// bool lightArray[38][10];//record the light pixel
int lightArray[38][10]; // record the light pixel
bool currentLanded = true;
long currentShape = -1;
long randNumber = -1;
long randNumberNext = -1;
int score = 0;
int currentL = 0;
int currentT = 0;
int currentZ = 0;
int currentI = 0;

const int ones_score_pin[] = {22, 23, 24, 25, 26, 27, 28};
const int tens_score_pin[] = {29, 30, 31, 32, 33, 34, 35};

const int ones_highscore_pin[] = {36, 37, 38, 39, 40, 41, 42};
const int tens_highscore_pin[] = {43, 44, 45, 46, 47, 48, 49};
//#define button 10

uint8_t current_button_state;
uint8_t previous_button_state;

const int numbers[10][7] =
    {
        {0, 0, 0, 0, 0, 0, 1}, // 0
        {1, 1, 0, 0, 1, 1, 1}, // 1
        {0, 0, 1, 0, 0, 1, 0}, // 2
        {1, 0, 0, 0, 0, 1, 0}, // 3
        {1, 1, 0, 0, 1, 0, 0}, // 4
        {1, 0, 0, 1, 0, 0, 0}, // 5
        {0, 0, 0, 1, 0, 0, 0}, // 6
        {1, 1, 0, 0, 0, 0, 1}, // 7
        {0, 0, 0, 0, 0, 0, 0}, // 8
        {1, 0, 0, 0, 0, 0, 0}, // 9
};
int incomingByte = 0;

bool isGameStarted = false;
bool isGameOver = false;

TMRpcm audio; // create an object for use in this sketch

enum State
{
    game_over,
    add_point,
    background,
    menu,
    stop_sound,
    increase,
    decrease,
    mute_bg,
    unmute_bg,
    stop_all,
    default_null
} audio_state;

static const char wav_addPoint[] PROGMEM = "point22.wav";
static const char wav_gameOver[] PROGMEM = "go3.wav";
static const char wav_backGround[] PROGMEM = "bgm1.wav";
static const char wav_menu[] PROGMEM = "menu.wav";

const char *wav_table[] = {
    wav_addPoint,
    wav_gameOver,
    wav_backGround,
    wav_menu,
};

void setup()
{
    Serial.begin(9600);
    pinMode(BUTTON_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
    pinMode(BUTTON_ROTATE, INPUT_PULLUP);
    pinMode(BUTTON_SPEEDUP, INPUT_PULLUP);
    pinMode(BUTTON_START, INPUT_PULLUP);
    pinMode(BUTTON_DROP, INPUT_PULLUP);
    /*
        ----NOTE----
        INPUT_PULLUP inverts the input of the buttons!!!
        LOW -> PRESSED
        HIGH -> RELEASED

    */
    randomSeed(analogRead(0));
    // randNumber = random(0, 5);
    //  new add random
    // randNumberNext = random(0, 5);

    for (int i = 0; i < 38; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            lightArray[i][j] = 0;
        }
    }

    // put your setup code here, to run once:
    for (int x = 0; x < 7; x++)
    {
        pinMode(ones_score_pin[x], OUTPUT);
        pinMode(tens_score_pin[x], OUTPUT);
        pinMode(ones_highscore_pin[x], OUTPUT);
        pinMode(tens_highscore_pin[x], OUTPUT);
    }

    audio.speakerPin = 5; // 5,6,11 or 46 on Mega, 9 on Uno, Nano, etc
    pinMode(2, OUTPUT);   // Pin pairs: 9,10 Mega: 5-2,6-7,11-12,46-45

    audio.speakerPin2 = 6; // Enable the second set of timer pins
    pinMode(7, OUTPUT);

    audio.setVolume(3);
    audio.quality(1);

    if (!SD.begin(SD_ChipSelectPin))
    {
        return;
    }
    else
    {
        Serial.println("SD OK");
        audio_state = default_null;
    }

    // randomShape(randNumber);
    col1.begin();
    col2.begin();
    col3.begin();
    col4.begin();
    colL1.begin();
    colL2.begin();
    colL3.begin();
    colR1.begin();
    colR2.begin();
    colR3.begin();
    colPL.begin();
    colPR.begin();
}

void loop()
{
    col1.setBrightness(10);
    col2.setBrightness(10);
    col3.setBrightness(10);
    col4.setBrightness(10);
    colL1.setBrightness(10);
    colL2.setBrightness(10);
    colL3.setBrightness(10);
    colR1.setBrightness(10);
    colR2.setBrightness(10);
    colR3.setBrightness(10);
    colPL.setBrightness(10);
    colPR.setBrightness(10);

    // shapeI();
    if (!isGameStarted)
    {
        initGame();
        currentStateStart = digitalRead(BUTTON_START);
        if (!audio.isPlaying(0))
        {
            audio_state = menu;
        }
        if (currentStateStart == LOW)
        {
            startGame();
            randomShape(randNumber);
        }
    }
    else
    {
        if (!audio.isPlaying(0))
        {
            audio_state = background;
        }
        isGameOver = false;

        randomShape(randNumber);
    }
    audioControl();

    displayScore(score);
    displayHighscore(highestScore);
}

void initGame()
{
    for (int i = 0; i < 38; i += 2)
    {
        for (int j = 0; j < 10; j++)
        {
            switch (17 - j)
            {
            case PIN:
                col1.setPixelColor(i, colours::WHITE);
                break;
            case PIN_2:
                col2.setPixelColor(i, colours::WHITE);
                break;
            case PIN_3:
                col3.setPixelColor(i, colours::WHITE);
                break;
            case PIN_4:
                col4.setPixelColor(i, colours::WHITE);
                break;
            case PIN_LEFT1:
                colL1.setPixelColor(i, colours::WHITE);
                break;
            case PIN_LEFT2:
                colL2.setPixelColor(i, colours::WHITE);
                break;
            case PIN_LEFT3:
                colL3.setPixelColor(i, colours::WHITE);
                break;
            case PIN_RIGHT1:
                colR1.setPixelColor(i, colours::WHITE);
                break;
            case PIN_RIGHT2:
                colR2.setPixelColor(i, colours::WHITE);
                break;
            case PIN_RIGHT3:
                colR3.setPixelColor(i, colours::WHITE);
                break;
            }
        }
    }
    colPL.setPixelColor(0, colours::WHITE);
    colPL.setPixelColor(2, colours::WHITE);
    colPL.setPixelColor(4, colours::WHITE);
    colPL.setPixelColor(6, colours::WHITE);
    colPR.setPixelColor(0, colours::WHITE);
    colPR.setPixelColor(2, colours::WHITE);
    colPR.setPixelColor(4, colours::WHITE);
    colPR.setPixelColor(6, colours::WHITE);
    showCol();
}

void startGame()
{
    audio_state = stop_all;

    for (int i = 0; i < 38; i += 2)
    {
        for (int j = 0; j < 10; j++)
        {
            switch (17 - j)
            {
            case PIN:
                col1.setPixelColor(i, colours::BLACK);
                break;
            case PIN_2:
                col2.setPixelColor(i, colours::BLACK);
                break;
            case PIN_3:
                col3.setPixelColor(i, colours::BLACK);
                break;
            case PIN_4:
                col4.setPixelColor(i, colours::BLACK);
                break;
            case PIN_LEFT1:
                colL1.setPixelColor(i, colours::BLACK);
                break;
            case PIN_LEFT2:
                colL2.setPixelColor(i, colours::BLACK);
                break;
            case PIN_LEFT3:
                colL3.setPixelColor(i, colours::BLACK);
                break;
            case PIN_RIGHT1:
                colR1.setPixelColor(i, colours::BLACK);
                break;
            case PIN_RIGHT2:
                colR2.setPixelColor(i, colours::BLACK);
                break;
            case PIN_RIGHT3:
                colR3.setPixelColor(i, colours::BLACK);
                break;
            }
        }
    }
    colPL.setPixelColor(0, colours::BLACK);
    colPL.setPixelColor(2, colours::BLACK);
    colPL.setPixelColor(4, colours::BLACK);
    colPL.setPixelColor(6, colours::BLACK);
    colPR.setPixelColor(0, colours::BLACK);
    colPR.setPixelColor(2, colours::BLACK);
    colPR.setPixelColor(4, colours::BLACK);
    colPR.setPixelColor(6, colours::BLACK);
    showCol();

    // randomSeed(analogRead(0));
    generateNumber();

    for (int i = 0; i < 38; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            lightArray[i][j] = 0;
        }
    }
    score = 0;
    // randomShape(randNumber);
    isGameStarted = true;
}

// show all col
void showCol()
{
    col1.show();
    col2.show();
    col3.show();
    col4.show();
    colL1.show();
    colL2.show();
    colL3.show();
    colR1.show();
    colR2.show();
    colR3.show();
    colPR.show();
    colPL.show();
}

void generateNumber()
{
    if (randNumber == -1)
    {
        randNumber = random(0, 5);
    }
    else
    {
        randNumber = randNumberNext;
    }

    randNumberNext = random(0, 5);
}

void randomShape(int currentShape)
{
    bool isGameOver = false;
    switch (currentShape)
    {
    case 0:
        if (lightArray[1][4] == 0 && lightArray[1][5] == 0)
        {
            shapeO();
        }
        else
        {
            isGameOver = true;
        }

        break;
    case 1:
        if ((lightArray[0][5] == 0) && (lightArray[1][4] == 0 && lightArray[1][5] == 0) && (lightArray[2][4] == 0))
        {
            shapeZ();
        }
        else
        {
            isGameOver = true;
        }

        break;
    case 2:
        if ((lightArray[0][5] == 0) && (lightArray[1][5] == 0) && (lightArray[2][5] == 0 && lightArray[2][4] == 0))
        {
            shapeL();
        }
        else
        {
            isGameOver = true;
        }

        break;
    case 3:
        if (lightArray[0][4] == 0 && (lightArray[1][4] == 0 && lightArray[1][5] == 0) || (lightArray[2][4] == 0))
        {
            shapeT();
        }
        else
        {
            isGameOver = true;
        }

        break;
    case 4:
        if ((lightArray[0][5] == 0) && lightArray[1][5] == 0 && lightArray[2][5] == 0 && lightArray[3][5] == 0)
        {
            shapeI();
        }
        else
        {
            isGameOver = true;
        }

        break;
    }

    if (isGameOver)
    {
        endGame();
    }

    currentLanded = false;
}

// Game Over function
// all pixels set to white
void endGame()
{
    audio_state = stop_all;
    audioControl();
    audio_state = game_over;
    audioControl();

    initGame();

    for (int i = 0; i < 38; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            lightArray[i][j] = 0;
        }
    }
    // 10.20新加
    centerx = 0;
    if (highestScore <= score)
    {
        highestScore = score;
    }
    isGameStarted = false;
    isGameOver = true;
}

void shapeO()
{
    centery1b = centery1;
    centery2b = centery2;
    centery3b = centery3;
    centery4b = centery4;
    lightO(centerx);
    switch (randNumberNext)
    {
    case 0:
        lightOPre();
        break;
    case 1:
        lightZPre();
        break;
    case 2:
        lightLPre();
        break;
    case 3:
        lightTPre();
        break;
    case 4:
        lightIPre();
        break;
    }
    showCol();

    currentStateL = digitalRead(BUTTON_LEFT);
    currentStateR = digitalRead(BUTTON_RIGHT);
    SpeedUp = digitalRead(BUTTON_SPEEDUP);
    currentStateDrop = digitalRead(BUTTON_DROP);

    if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0))
    {

        if (centery3 < PIN_LEFT3)
        {
            centery1b = centery1;
            centery2b = centery2;
            centery3b = centery3;
            centery4b = centery4;
            centery1 += 1;
            centery2 += 1;
            centery3 += 1;
            centery4 += 1;
        }
    }
    if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + SPEED + 2][17 - centery2 + 1] == 0))
    {

        if (centery2 > PIN_RIGHT3)
        {
            centery1b = centery1;
            centery2b = centery2;
            centery3b = centery3;
            centery4b = centery4;
            centery1 -= 1;
            centery2 -= 1;
            centery3 -= 1;
            centery4 -= 1;
        }
    }
    // Change Speed
    if (SpeedUp == LOW)
    {
        delay(100);
    }
    else
    {
        delay(500);
    }
    if (currentStateDrop == LOW)
    {
        turnoffO(centerx);
        while ((centerx < NUMPIXELS - 4) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
        {
            centerx = centerx + SPEED;
            // showCol();
        }

        lightArray[centerx][17 - centery2] = 3;
        lightArray[centerx + 2][17 - centery2] = 3;
        lightArray[centerx][17 - centery3] = 3;
        lightArray[centerx + 2][17 - centery3] = 3;
        lightO(centerx);
        centerx = 0;
        centery1 = PIN;
        centery2 = PIN_2;
        centery3 = PIN_3;
        centery4 = PIN_4;
        checkLine();
        switch (randNumberNext)
        {
        case 0:
            turnoffOPre();
            break;
        case 1:
            turnoffZPre();
            break;
        case 2:
            turnoffLPre();
            break;
        case 3:
            turnoffTPre();
            break;
        case 4:
            turnoffIPre();
            break;
        }
        showCol();
        generateNumber();
        // randNumber = randNumberNext;
        // randNumberNext = random(0, 5);
    }

    else if ((centerx < NUMPIXELS - 4) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
    {

        turnoffO(centerx);

        centerx = centerx + SPEED;
        showCol();
    }
    else
    {

        lightArray[centerx][17 - centery2] = 3;
        lightArray[centerx + 2][17 - centery2] = 3;
        lightArray[centerx][17 - centery3] = 3;
        lightArray[centerx + 2][17 - centery3] = 3;

        centerx = 0;
        centery1 = PIN;
        centery2 = PIN_2;
        centery3 = PIN_3;
        centery4 = PIN_4;
        checkLine();
        switch (randNumberNext)
        {
        case 0:
            turnoffOPre();
            break;
        case 1:
            turnoffZPre();
            break;
        case 2:
            turnoffLPre();
            break;
        case 3:
            turnoffTPre();
            break;
        case 4:
            turnoffIPre();
            break;
        }
        showCol();
        generateNumber();
        // randNumber = randNumberNext;
        // randNumberNext = random(0, 5);
    }
}

// shapeL
void shapeL()
{
    centery1b = centery1;
    centery2b = centery2;
    centery3b = centery3;
    centery4b = centery4;
    lightL(centerx);
    switch (randNumberNext)
    {
    case 0:
        lightOPre();
        break;
    case 1:
        lightZPre();
        break;
    case 2:
        lightLPre();
        break;
    case 3:
        lightTPre();
        break;
    case 4:
        lightIPre();
        break;
    }
    showCol();

    currentStateL = digitalRead(BUTTON_LEFT);
    currentStateR = digitalRead(BUTTON_RIGHT);
    currentRotateState = digitalRead(BUTTON_ROTATE); //记录是否按下rotate button
    SpeedUp = digitalRead(BUTTON_SPEEDUP);
    currentStateDrop = digitalRead(BUTTON_DROP);
    // need to add more conditions 不同形态的
    if (currentL == 0)
    {
        // move left
        if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery3 - 1] == 0))
        {

            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        // move right
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 4][17 - centery2 + 1] == 0))
        {

            if (centery2 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (centery2 > PIN_RIGHT3) && (lightArray[centerx + 2][17 - centery2] == 0) && (lightArray[centerx + 2][17 - centery1] == 0))
        {
            // turnoff current state
            turnoffL(centerx);
            // light next state
            currentL = (currentL + 1) % 4;
            lightL(centerx);
        }
    }
    else if (currentL == 1)
    {
        if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0))
        {

            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery1 + 1] == 0) && (lightArray[centerx + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + SPEED][17 - centery3 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 + 1] == 0))
        {

            if (centery1 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (lightArray[centerx + 2][17 - centery2] == 0) && (lightArray[centerx + 4][17 - centery2] == 0) && (lightArray[centerx + 6][17 - centery2] == 0))
        {
            // turnoff current state
            turnoffL(centerx);
            // light next state
            currentL = (currentL + 1) % 4;
            lightL(centerx);
        }
    }
    else if (currentL == 2)
    {
        if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2 - 1] == 0))
        {

            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2 + 1] == 0))
        {

            if (centery2 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (centery2 > PIN_RIGHT3) && (lightArray[centerx + 2][17 - centery1] == 0) && (lightArray[centerx + 4][17 - centery1] == 0) && (lightArray[centerx + 4][17 - centery3] == 0))
        {
            // turnoff current state
            turnoffL(centerx);
            // light next state
            currentL = (currentL + 1) % 4;
            lightL(centerx);
        }
    }
    else if (currentL == 3)
    {
        if (currentStateL == LOW && (lightArray[centerx + 2 + SPEED][17 - centery1 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0))
        {

            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery1 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery1 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 + 1] == 0))
        {

            if (centery1 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (lightArray[centerx + 4][17 - centery3] == 0) && (lightArray[centerx + 6][17 - centery3] == 0) && (lightArray[centerx + 6][17 - centery2] == 0))
        {
            // turnoff current state
            turnoffL(centerx);
            // light next state
            currentL = (currentL + 1) % 4;
            lightL(centerx);
        }
    }
    // Change Speed
    if (SpeedUp == LOW)
    {
        delay(100);
    }
    else
    {
        delay(500);
    }
    // Drop
    // need to add more conditions 不同形态
    // 0
    if (currentL == 0)
    {
        if (currentStateDrop == LOW)
        {
            turnoffL(centerx);
            while ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
                // showCol();
            }
            lightArray[centerx][17 - centery3] = 1;
            lightArray[centerx + 2][17 - centery3] = 1;
            lightArray[centerx + 4][17 - centery3] = 1;
            lightArray[centerx + 4][17 - centery2] = 1;
            lightL(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
        else if ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery3] == 0))
        {
            turnoffL(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery3] = 1;
            lightArray[centerx + 2][17 - centery3] = 1;
            lightArray[centerx + 4][17 - centery3] = 1;
            lightArray[centerx + 4][17 - centery2] = 1;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
    }
    // 1
    if (currentL == 1)
    {
        if (currentStateDrop == LOW)
        {
            turnoffL(centerx);
            while ((centerx < NUMPIXELS - 4) && (lightArray[centerx + SPEED][17 - centery1] == 0) && (lightArray[centerx + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery1] = 1;
            lightArray[centerx][17 - centery2] = 1;
            lightArray[centerx][17 - centery3] = 1;
            lightArray[centerx + 2][17 - centery3] = 1;
            lightL(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
        else if ((centerx < NUMPIXELS - 4) && (lightArray[centerx + SPEED][17 - centery1] == 0) && (lightArray[centerx + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
        {
            turnoffL(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery1] = 1;
            lightArray[centerx][17 - centery2] = 1;
            lightArray[centerx][17 - centery3] = 1;
            lightArray[centerx + 2][17 - centery3] = 1;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
    }
    if (currentL == 2)
    {
        if (currentStateDrop == LOW)
        {
            turnoffL(centerx);
            while ((centerx < NUMPIXELS - 6) && (lightArray[centerx + SPEED][17 - centery3] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery3] = 1;
            lightArray[centerx][17 - centery2] = 1;
            lightArray[centerx + 2][17 - centery2] = 1;
            lightArray[centerx + 4][17 - centery2] = 1;
            lightL(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
        else if ((centerx < NUMPIXELS - 6) && (lightArray[centerx + SPEED][17 - centery3] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0))
        {
            turnoffL(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery3] = 1;
            lightArray[centerx][17 - centery2] = 1;
            lightArray[centerx + 2][17 - centery2] = 1;
            lightArray[centerx + 4][17 - centery2] = 1;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
    }
    if (currentL == 3)
    {
        if (currentStateDrop == LOW)
        {
            turnoffL(centerx);
            while ((centerx < NUMPIXELS - 4) && (lightArray[centerx + 2 + SPEED][17 - centery1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery1] = 1;
            lightArray[centerx + 2][17 - centery1] = 1;
            lightArray[centerx + 2][17 - centery2] = 1;
            lightArray[centerx + 2][17 - centery3] = 1;
            lightL(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
        else if ((centerx < NUMPIXELS - 4) && (lightArray[centerx + 2 + SPEED][17 - centery1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
        {
            turnoffL(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery1] = 1;
            lightArray[centerx + 2][17 - centery1] = 1;
            lightArray[centerx + 2][17 - centery2] = 1;
            lightArray[centerx + 2][17 - centery3] = 1;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentL = 0;
        }
    }
}

// shape Z
void shapeZ()
{
    centery1b = centery1;
    centery2b = centery2;
    centery3b = centery3;
    centery4b = centery4;
    lightZ(centerx);
    switch (randNumberNext)
    {
    case 0:
        lightOPre();
        break;
    case 1:
        lightZPre();
        break;
    case 2:
        lightLPre();
        break;
    case 3:
        lightTPre();
        break;
    case 4:
        lightIPre();
        break;
    }
    showCol();

    currentStateL = digitalRead(BUTTON_LEFT);
    currentStateR = digitalRead(BUTTON_RIGHT);
    currentRotateState = digitalRead(BUTTON_ROTATE);
    SpeedUp = digitalRead(BUTTON_SPEEDUP);
    currentStateDrop = digitalRead(BUTTON_DROP);

    if (currentZ == 0)
    {
        if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 - 1] == 0))
        {

            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED + 2][17 - centery2 + 1] == 0) && (lightArray[centerx + SPEED + 4][17 - centery2 + 1] == 0))
        {
            // clear(centerx-SPEED);
            if (centery2 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }

        if ((currentRotateState == LOW) && (centery2 > PIN_RIGHT3) && (lightArray[centerx + 4][17 - centery3] == 0) && (lightArray[centerx + 2][17 - centery1] == 0))
        {
            // turnoff current state
            turnoffZ(centerx);
            // light next state
            currentZ = (currentZ + 1) % 2;
            lightZ(centerx);
        }
    }
    else if (currentZ == 1)
    {
        if (currentStateL == LOW && (lightArray[centerx + 2 + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0))
        {

            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED + 2][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 2][17 - centery2 + 1] == 0) && (lightArray[centerx + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + SPEED][17 - centery1 + 1] == 0))
        {
            // clear(centerx-SPEED);
            if (centery1 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }

        if ((currentRotateState == LOW) && (lightArray[centerx + 4][17 - centery3] == 0) && (lightArray[centerx + 4][17 - centery2] == 0) && (lightArray[centerx + 6][17 - centery2] == 0))
        {
            // turnoff current state
            turnoffZ(centerx);
            // light next state
            currentZ = (currentZ + 1) % 2;
            lightZ(centerx);
        }
    }

    if (SpeedUp == LOW)
    {
        delay(100);
    }
    else
    {
        delay(500);
    }
    if (currentZ == 0)
    {
        if (currentStateDrop == LOW)
        {
            turnoffZ(centerx);
            while ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx + 2][17 - centery2] = 4;
            lightArray[centerx + 4][17 - centery2] = 4;
            lightArray[centerx][17 - centery3] = 4;
            lightArray[centerx + 2][17 - centery3] = 4;
            lightZ(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentZ = 0;
        }
        else if ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
        {

            turnoffZ(centerx);

            centerx = centerx + SPEED;
            showCol();
        }
        else
        {

            lightArray[centerx + 2][17 - centery2] = 4;
            lightArray[centerx + 4][17 - centery2] = 4;
            lightArray[centerx][17 - centery3] = 4;
            lightArray[centerx + 2][17 - centery3] = 4;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentZ = 0;
        }
    }
    if (currentZ == 1)
    {
        if (currentStateDrop == LOW)
        {
            turnoffZ(centerx);
            while ((centerx < NUMPIXELS - 4) && (lightArray[centerx + SPEED][17 - centery1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx + 2][17 - centery3] = 4;
            lightArray[centerx][17 - centery2] = 4;
            lightArray[centerx + 2][17 - centery2] = 4;
            lightArray[centerx][17 - centery1] = 4;
            lightZ(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentZ = 0;
        }
        else if ((centerx < NUMPIXELS - 4) && (lightArray[centerx + SPEED][17 - centery1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
        {

            turnoffZ(centerx);

            centerx = centerx + SPEED;
            showCol();
        }
        else
        {

            lightArray[centerx + 2][17 - centery3] = 4;
            lightArray[centerx][17 - centery2] = 4;
            lightArray[centerx + 2][17 - centery2] = 4;
            lightArray[centerx][17 - centery1] = 4;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentZ = 0;
        }
    }
}
// shapeT
void shapeT()
{
    centery1b = centery1;
    centery2b = centery2;
    centery3b = centery3;
    centery4b = centery4;
    lightT(centerx);
    switch (randNumberNext)
    {
    case 0:
        lightOPre();
        break;
    case 1:
        lightZPre();
        break;
    case 2:
        lightLPre();
        break;
    case 3:
        lightTPre();
        break;
    case 4:
        lightIPre();
        break;
    }
    showCol();

    currentStateL = digitalRead(BUTTON_LEFT);
    currentStateR = digitalRead(BUTTON_RIGHT);
    currentRotateState = digitalRead(BUTTON_ROTATE);
    SpeedUp = digitalRead(BUTTON_SPEEDUP);
    currentStateDrop = digitalRead(BUTTON_DROP);

    if (currentT == 0)
    {
        // move left
        if ((currentStateL == LOW) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2 - 1] == 0))
        {
            if (centery3 < PIN_LEFT3)
            {
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        // move right
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery2 + 1] == 0))
        {
            if (centery2 > PIN_RIGHT3)
            {
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (centery3 < PIN_LEFT3) && (lightArray[centerx + 4][17 - centery3] == 0) && (lightArray[centerx + 4][17 - centery4] == 0))
        {
            // turnoff current state
            turnoffT(centerx);
            // light next state
            currentT = (currentT + 1) % 4;
            lightT(centerx);
        }
    }
    else if (currentT == 1)
    {
        // move left
        if ((currentStateL == LOW) && (lightArray[centerx + 2 + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery4 - 1] == 0))
        {
            if (centery4 < PIN_LEFT3)
            {
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        // move right
        if ((currentStateR == LOW) && (lightArray[centerx + 2 + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery4 + 1] == 0))
        {
            if (centery2 > PIN_RIGHT3)
            {
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (lightArray[centerx + 4][17 - centery3] == 0) && (lightArray[centerx + 4][17 - centery2] == 0) && (lightArray[centerx + 6][17 - centery3] == 0))
        {
            // turnoff current state
            turnoffT(centerx);
            // light next state
            currentT = (currentT + 1) % 4;
            lightT(centerx);
        }
    }
    else if (currentT == 2)
    {
        // move left
        if ((currentStateL == LOW) && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery3 - 1] == 0))
        {
            if (centery3 < PIN_LEFT3)
            {
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        // move right
        if ((currentStateR == LOW) && (lightArray[centerx + 2 + SPEED][17 - centery3 + 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery3 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2 + 1] == 0))
        {
            if (centery2 > PIN_RIGHT3)
            {
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (centery3 < PIN_LEFT3) && (lightArray[centerx + 2][17 - centery4] == 0))
        {
            // turnoff current state
            turnoffT(centerx);
            // light next state
            currentT = (currentT + 1) % 4;
            lightT(centerx);
        }
    }
    else if (currentT == 3)
    {
        // move left
        if ((currentStateL == LOW) && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + SPEED][17 - centery4 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0))
        {
            if (centery4 < PIN_LEFT3)
            {
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        // move right
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED][17 - centery2 + 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 + 1] == 0))
        {
            if (centery2 > PIN_RIGHT3)
            {
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (lightArray[centerx + 2][17 - centery2] == 0) && (lightArray[centerx + 4][17 - centery2] == 0) && (lightArray[centerx + 6][17 - centery2] == 0) && (lightArray[centerx + 4][17 - centery3] == 0))
        {
            // turnoff current state
            turnoffT(centerx);
            // light next state
            currentT = (currentT + 1) % 4;
            lightT(centerx);
        }
    }

    // Change Speed
    if (SpeedUp == LOW)
    {
        delay(100);
    }
    else
    {
        delay(500);
    }
    if (currentT == 0)
    {
        if (currentStateDrop == LOW)
        {
            turnoffT(centerx);
            while ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery2] = 5;
            lightArray[centerx + 2][17 - centery2] = 5;
            lightArray[centerx + 4][17 - centery2] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            lightT(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
        else if ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0))
        {
            turnoffT(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery2] = 5;
            lightArray[centerx + 2][17 - centery2] = 5;
            lightArray[centerx + 4][17 - centery2] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
    }
    if (currentT == 1)
    {
        if (currentStateDrop == LOW)
        {
            turnoffT(centerx);
            while ((centerx < NUMPIXELS - 4) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery4] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery2] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery4] = 5;
            lightT(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
        else if ((centerx < NUMPIXELS - 4) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery4] == 0))
        {
            turnoffT(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery2] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery4] = 5;
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
    }
    if (currentT == 2)
    {
        if (currentStateDrop == LOW)
        {
            turnoffT(centerx);
            while ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery3] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            lightArray[centerx + 4][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery2] = 5;
            lightT(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
        else if ((centerx < NUMPIXELS - 6) && (lightArray[centerx + 4 + SPEED][17 - centery3] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery2] == 0))
        {
            turnoffT(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            lightArray[centerx + 4][17 - centery3] = 5;
            lightArray[centerx + 2][17 - centery2] = 5;
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
    }
    if (currentT == 3)
    {
        if (currentStateDrop == LOW)
        {
            turnoffT(centerx);
            while ((centerx < NUMPIXELS - 4) && (lightArray[centerx + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0) && (lightArray[centerx + SPEED][17 - centery4] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery2] = 5;
            lightArray[centerx][17 - centery3] = 5;
            lightArray[centerx][17 - centery4] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            lightT(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
        else if ((centerx < NUMPIXELS - 4) && (lightArray[centerx + SPEED][17 - centery2] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3] == 0) && (lightArray[centerx + SPEED][17 - centery4] == 0))
        {
            turnoffT(centerx);
            centerx = centerx + SPEED;
            showCol();
        }
        else
        {
            lightArray[centerx][17 - centery2] = 5;
            lightArray[centerx][17 - centery3] = 5;
            lightArray[centerx][17 - centery4] = 5;
            lightArray[centerx + 2][17 - centery3] = 5;
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentT = 0;
        }
    }
}

// shape I
void shapeI()
{
    centery1b = centery1;
    centery2b = centery2;
    centery3b = centery3;
    centery4b = centery4;
    lightI(centerx);
    switch (randNumberNext)
    {
    case 0:
        lightOPre();
        break;
    case 1:
        lightZPre();
        break;
    case 2:
        lightLPre();
        break;
    case 3:
        lightTPre();
        break;
    case 4:
        lightIPre();
        break;
    }
    showCol();

    currentStateL = digitalRead(BUTTON_LEFT);
    currentStateR = digitalRead(BUTTON_RIGHT);
    currentRotateState = digitalRead(BUTTON_ROTATE);
    SpeedUp = digitalRead(BUTTON_SPEEDUP);
    currentStateDrop = digitalRead(BUTTON_DROP);

    if (currentI == 0)
    {
        if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 2 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 4 + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + 6 + SPEED][17 - centery3 - 1] == 0))
        {
            if (centery3 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED + 2][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 4][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 6][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 6][17 - centery3 + 1] == 0))
        {
            if (centery3 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (centery3 > PIN_RIGHT2) && (centery3 < PIN_LEFT3) && (lightArray[centerx + 2][17 - centery4] == 0) && (lightArray[centerx + 2][17 - centery2] == 0) && (lightArray[centerx + 2][17 - centery1] == 0))
        {
            // turnoff current state
            turnoffI(centerx);
            // light next state
            currentI = (currentI + 1) % 2;
            lightI(centerx);
        }
    }
    else if (currentI == 1)
    {
        if (currentStateL == LOW && (lightArray[centerx + SPEED][17 - centery1 - 1] == 0) && (lightArray[centerx + SPEED][17 - centery2 - 1] == 0) && (lightArray[centerx + SPEED][17 - centery3 - 1] == 0) && (lightArray[centerx + SPEED][17 - centery4 - 1] == 0))
        {
            if (centery4 < PIN_LEFT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 += 1;
                centery2 += 1;
                centery3 += 1;
                centery4 += 1;
            }
        }
        if ((currentStateR == LOW) && (lightArray[centerx + SPEED + 2][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 4][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 6][17 - centery3 + 1] == 0) && (lightArray[centerx + SPEED + 6][17 - centery3 + 1] == 0))
        {
            if (centery1 > PIN_RIGHT3)
            {
                centery1b = centery1;
                centery2b = centery2;
                centery3b = centery3;
                centery4b = centery4;
                centery1 -= 1;
                centery2 -= 1;
                centery3 -= 1;
                centery4 -= 1;
            }
        }
        // rotate
        if ((currentRotateState == LOW) && (lightArray[centerx + 2][17 - centery3] == 0) && (lightArray[centerx + 4][17 - centery3] == 0) && (lightArray[centerx + 6][17 - centery3] == 0))
        {
            // turnoff current state
            turnoffI(centerx);
            // light next state
            currentI = (currentI + 1) % 2;
            lightI(centerx);
        }
    }

    // Change Speed
    if (SpeedUp == LOW)
    {
        delay(100);
    }
    else
    {
        delay(500);
    }

    if (currentI == 0)
    {
        if (currentStateDrop == LOW)
        {
            turnoffI(centerx);
            while ((centerx < NUMPIXELS - 8) && (lightArray[centerx + 6 + SPEED][17 - centery3] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery3] = 2;
            lightArray[centerx + 2][17 - centery3] = 2;
            lightArray[centerx + 4][17 - centery3] = 2;
            lightArray[centerx + 6][17 - centery3] = 2;
            lightI(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentI = 0;
        }
        else if ((centerx < NUMPIXELS - 8) && (lightArray[centerx + 6 + SPEED][17 - centery3] == 0))
        {

            turnoffI(centerx);

            centerx = centerx + SPEED;
            showCol();
        }
        else
        {

            lightArray[centerx][17 - centery3] = 2;
            lightArray[centerx + 2][17 - centery3] = 2;
            lightArray[centerx + 4][17 - centery3] = 2;
            lightArray[centerx + 6][17 - centery3] = 2;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentI = 0;
        }
    }
    if (currentI == 1)
    {
        if (currentStateDrop == LOW)
        {
            turnoffI(centerx);
            while ((centerx < NUMPIXELS) && (lightArray[centerx + SPEED][17 - centery1] == 0) && (lightArray[centerx + SPEED][17 - centery2] == 0) && (lightArray[centerx + SPEED][17 - centery3] == 0) && (lightArray[centerx + SPEED][17 - centery4] == 0))
            {
                centerx = centerx + SPEED;
            }
            lightArray[centerx][17 - centery1] = 2;
            lightArray[centerx][17 - centery2] = 2;
            lightArray[centerx][17 - centery3] = 2;
            lightArray[centerx][17 - centery4] = 2;
            lightI(centerx);
            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentI = 0;
        }
        else if ((centerx < NUMPIXELS) && (lightArray[centerx + SPEED][17 - centery1] == 0) && (lightArray[centerx + SPEED][17 - centery2] == 0) && (lightArray[centerx + SPEED][17 - centery3] == 0) && (lightArray[centerx + SPEED][17 - centery4] == 0))
        {

            turnoffI(centerx);

            centerx = centerx + SPEED;
            showCol();
        }
        else
        {

            lightArray[centerx][17 - centery1] = 2;
            lightArray[centerx][17 - centery2] = 2;
            lightArray[centerx][17 - centery3] = 2;
            lightArray[centerx][17 - centery4] = 2;

            centerx = 0;
            centery1 = PIN;
            centery2 = PIN_2;
            centery3 = PIN_3;
            centery4 = PIN_4;
            checkLine();
            switch (randNumberNext)
            {
            case 0:
                turnoffOPre();
                break;
            case 1:
                turnoffZPre();
                break;
            case 2:
                turnoffLPre();
                break;
            case 3:
                turnoffTPre();
                break;
            case 4:
                turnoffIPre();
                break;
            }
            showCol();
            generateNumber();
            // randNumber = randNumberNext;
            // randNumberNext = random(0, 5);
            currentI = 0;
        }
    }
}

// check whether to clear the line
void checkLine()
{
    audio_state = mute_bg;
    audioControl();
    bool playAudioOnce = false;
    int product = 1;
    for (int i = 36; i > 0; i = i - 2)
    {
        product = 1;
        for (int j = 0; j < 10; j = j + 1)
        {
            product = product * lightArray[i][j];
        }

        if (product != 0)
        {
            score += 1;
            if (!playAudioOnce)
            {
                audio_state = add_point;
                audioControl();
                playAudioOnce = true;
            }

            // delay(10);
            blink(i);
            arrayUpdate(i);
            // blink(i);
            i = i + 2;
        }
    }
    audio_state = unmute_bg;
    audioControl();
    arrayLight();
}
void blink(int line)
{
    // unsigned long previousMillis = 0;  // will store last time LED was updated
    // const long interval = 50;  // interval at which to blink (milliseconds)
    // unsigned long currentMillis = millis();
    //  if(currentMillis - previousMillis >= interval){
    //    previousMillis = currentMillis;
    //  }
    for (int i = 0; i < 4; i++)
    {
        if (i % 2 == 0)
        {
            col1.setPixelColor(line, colours::BLACK);
            col2.setPixelColor(line, colours::BLACK);
            col3.setPixelColor(line, colours::BLACK);
            col4.setPixelColor(line, colours::BLACK);
            colL1.setPixelColor(line, colours::BLACK);
            colL2.setPixelColor(line, colours::BLACK);
            colL3.setPixelColor(line, colours::BLACK);
            colR1.setPixelColor(line, colours::BLACK);
            colR2.setPixelColor(line, colours::BLACK);
            colR3.setPixelColor(line, colours::BLACK);
        }
        else
        {
            col1.setPixelColor(line, colours::WHITE);
            col2.setPixelColor(line, colours::WHITE);
            col3.setPixelColor(line, colours::WHITE);
            col4.setPixelColor(line, colours::WHITE);
            colL1.setPixelColor(line, colours::WHITE);
            colL2.setPixelColor(line, colours::WHITE);
            colL3.setPixelColor(line, colours::WHITE);
            colR1.setPixelColor(line, colours::WHITE);
            colR2.setPixelColor(line, colours::WHITE);
            colR3.setPixelColor(line, colours::WHITE);
        }
        showCol();
    }
}

void arrayUpdate(int line)
{

    for (int i = line; i > 0; i = i - 2)
    {
        for (int j = 0; j < 10; j = j + 1)
        {
            lightArray[i][j] = lightArray[i - 2][j];
        }
    }
    for (int j = 0; j < 10; j = j + 1)
    {
        lightArray[0][j] = 0;
    }
}
// according to the lightArray to set pixel color
void arrayLight()
{
    for (int i = 0; i < 38; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            switch (17 - j)
            {
            case PIN:
                col1.setPixelColor(i, colours::BLACK);
                break;
            case PIN_2:
                col2.setPixelColor(i, colours::BLACK);
                break;
            case PIN_3:
                col3.setPixelColor(i, colours::BLACK);
                break;
            case PIN_4:
                col4.setPixelColor(i, colours::BLACK);
                break;
            case PIN_LEFT1:
                colL1.setPixelColor(i, colours::BLACK);
                break;
            case PIN_LEFT2:
                colL2.setPixelColor(i, colours::BLACK);
                break;
            case PIN_LEFT3:
                colL3.setPixelColor(i, colours::BLACK);
                break;
            case PIN_RIGHT1:
                colR1.setPixelColor(i, colours::BLACK);
                break;
            case PIN_RIGHT2:
                colR2.setPixelColor(i, colours::BLACK);
                break;
            case PIN_RIGHT3:
                colR3.setPixelColor(i, colours::BLACK);
                break;
            }
            if (lightArray[i][j] == 1)
            {
                switch (17 - j)
                {
                case PIN:
                    col1.setPixelColor(i, colours::RED);
                    break;
                case PIN_2:
                    col2.setPixelColor(i, colours::RED);
                    break;
                case PIN_3:
                    col3.setPixelColor(i, colours::RED);
                    break;
                case PIN_4:
                    col4.setPixelColor(i, colours::RED);
                    break;
                case PIN_LEFT1:
                    colL1.setPixelColor(i, colours::RED);
                    break;
                case PIN_LEFT2:
                    colL2.setPixelColor(i, colours::RED);
                    break;
                case PIN_LEFT3:
                    colL3.setPixelColor(i, colours::RED);
                    break;
                case PIN_RIGHT1:
                    colR1.setPixelColor(i, colours::RED);
                    break;
                case PIN_RIGHT2:
                    colR2.setPixelColor(i, colours::RED);
                    break;
                case PIN_RIGHT3:
                    colR3.setPixelColor(i, colours::RED);
                    break;
                }
            }
            if (lightArray[i][j] == 2)
            {
                switch (17 - j)
                {
                case PIN:
                    col1.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_2:
                    col2.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_3:
                    col3.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_4:
                    col4.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_LEFT1:
                    colL1.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_LEFT2:
                    colL2.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_LEFT3:
                    colL3.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_RIGHT1:
                    colR1.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_RIGHT2:
                    colR2.setPixelColor(i, colours::BLUE);
                    break;
                case PIN_RIGHT3:
                    colR3.setPixelColor(i, colours::BLUE);
                    break;
                }
            }
            if (lightArray[i][j] == 3)
            {
                switch (17 - j)
                {
                case PIN:
                    col1.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_2:
                    col2.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_3:
                    col3.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_4:
                    col4.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_LEFT1:
                    colL1.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_LEFT2:
                    colL2.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_LEFT3:
                    colL3.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_RIGHT1:
                    colR1.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_RIGHT2:
                    colR2.setPixelColor(i, colours::GREEN);
                    break;
                case PIN_RIGHT3:
                    colR3.setPixelColor(i, colours::GREEN);
                    break;
                }
            }
            if (lightArray[i][j] == 4)
            {
                switch (17 - j)
                {
                case PIN:
                    col1.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_2:
                    col2.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_3:
                    col3.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_4:
                    col4.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_LEFT1:
                    colL1.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_LEFT2:
                    colL2.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_LEFT3:
                    colL3.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_RIGHT1:
                    colR1.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_RIGHT2:
                    colR2.setPixelColor(i, colours::YELLOW);
                    break;
                case PIN_RIGHT3:
                    colR3.setPixelColor(i, colours::YELLOW);
                    break;
                }
            }
            if (lightArray[i][j] == 5)
            {
                switch (17 - j)
                {
                case PIN:
                    col1.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_2:
                    col2.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_3:
                    col3.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_4:
                    col4.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_LEFT1:
                    colL1.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_LEFT2:
                    colL2.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_LEFT3:
                    colL3.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_RIGHT1:
                    colR1.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_RIGHT2:
                    colR2.setPixelColor(i, colours::PURPLE);
                    break;
                case PIN_RIGHT3:
                    colR3.setPixelColor(i, colours::PURPLE);
                    break;
                }
            }
        }
    }
}

// turnoff
void turnoffO(int centerx)
{
    switch (centery1b)
    {
    case PIN:

        col2.setPixelColor(centerx, colours::BLACK);
        col2.setPixelColor(centerx + 2, colours::BLACK);
        col3.setPixelColor(centerx, colours::BLACK);
        col3.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_2:

        col3.setPixelColor(centerx, colours::BLACK);
        col3.setPixelColor(centerx + 2, colours::BLACK);
        col4.setPixelColor(centerx, colours::BLACK);
        col4.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_3:

        col4.setPixelColor(centerx, colours::BLACK);
        col4.setPixelColor(centerx + 2, colours::BLACK);
        colL1.setPixelColor(centerx, colours::BLACK);
        colL1.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_4:

        colL1.setPixelColor(centerx, colours::BLACK);
        colL1.setPixelColor(centerx + 2, colours::BLACK);
        colL2.setPixelColor(centerx, colours::BLACK);
        colL2.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_LEFT1:

        colL2.setPixelColor(centerx, colours::BLACK);
        colL2.setPixelColor(centerx + 2, colours::BLACK);
        colL3.setPixelColor(centerx, colours::BLACK);
        colL3.setPixelColor(centerx + 2, colours::BLACK);
        break;
    case PIN_RIGHT1:

        col1.setPixelColor(centerx, colours::BLACK);
        col1.setPixelColor(centerx + 2, colours::BLACK);
        col2.setPixelColor(centerx, colours::BLACK);
        col2.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_RIGHT2:

        colR1.setPixelColor(centerx, colours::BLACK);
        colR1.setPixelColor(centerx + 2, colours::BLACK);
        col1.setPixelColor(centerx, colours::BLACK);
        col1.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_RIGHT3:

        colR2.setPixelColor(centerx, colours::BLACK);
        colR2.setPixelColor(centerx + 2, colours::BLACK);
        colR1.setPixelColor(centerx, colours::BLACK);
        colR1.setPixelColor(centerx + 2, colours::BLACK);

        break;
    case PIN_RIGHT3 - 1:
        colR3.setPixelColor(centerx, colours::BLACK);
        colR3.setPixelColor(centerx + 2, colours::BLACK);
        colR2.setPixelColor(centerx, colours::BLACK);
        colR2.setPixelColor(centerx + 2, colours::BLACK);

        break;
    }
}

void turnoffL(int centerx)
{
    if (currentL == 0)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx + 4, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_2:

            col3.setPixelColor(centerx + 4, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_3:

            col4.setPixelColor(centerx + 4, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_4:

            colL1.setPixelColor(centerx + 4, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx + 4, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx + 4, colours::BLACK);
            break;
        case PIN_RIGHT1:

            col1.setPixelColor(centerx + 4, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR1.setPixelColor(centerx + 4, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR2.setPixelColor(centerx + 4, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 4, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 4, colours::BLACK);

            break;
        }
    }
    if (currentL == 1)
    {
        switch (centery1b)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        }
    }
    if (currentL == 2)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 4, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_2:

            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 4, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_3:

            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 4, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_4:

            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 4, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 4, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);
            break;
        case PIN_RIGHT1:

            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 4, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 4, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 4, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT3 - 1:

            colR3.setPixelColor(centerx, colours::BLACK);
            colR3.setPixelColor(centerx + 2, colours::BLACK);
            colR3.setPixelColor(centerx + 4, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);

            break;
        }
    }
    if (currentL == 3)
    {
        switch (centery1b)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::BLACK);
            colR3.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        }
    }
}

void turnoffZ(int centerx)
{
    if (currentZ == 0)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 4, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_2:

            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 4, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_3:

            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 4, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_4:

            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 4, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 4, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT1:

            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 4, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 4, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 4, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 2, colours::BLACK);
            colR3.setPixelColor(centerx + 4, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        }
    }
    if (currentZ == 1)
    {
        switch (centery1b)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);

            break;
        }
    }
}
void turnoffT(int centerx)
{
    if (currentT == 0)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 4, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_2:
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 4, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_3:
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 4, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 4, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 4, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 4, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 4, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 4, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx, colours::BLACK);
            colR3.setPixelColor(centerx + 2, colours::BLACK);
            colR3.setPixelColor(centerx + 4, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            break;
        }
    }
    if (currentT == 1)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_2:
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_3:
            col4.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);

            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            break;
        }
    }
    if (currentT == 2)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_2:
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 4, colours::BLACK);
            break;
        case PIN_3:
            col4.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 4, colours::BLACK);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_LEFT1:
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx + 4, colours::BLACK);

            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 4, colours::BLACK);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 4, colours::BLACK);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 4, colours::BLACK);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 4, colours::BLACK);
            break;
        }
    }
    if (currentT == 3)
    {
        switch (centery1b)
        {
        case PIN:

            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_2:
            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            break;
        case PIN_3:
            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            break;
        }
    }
}

void turnoffI(int centerx)
{
    if (currentI == 0)
    {
        switch (centery1b)
        {
        case PIN:

            col3.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx + 2, colours::BLACK);
            col3.setPixelColor(centerx + 4, colours::BLACK);
            col3.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_2:

            col4.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx + 2, colours::BLACK);
            col4.setPixelColor(centerx + 4, colours::BLACK);
            col4.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_3:

            colL1.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx + 2, colours::BLACK);
            colL1.setPixelColor(centerx + 4, colours::BLACK);
            colL1.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_4:

            colL2.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx + 2, colours::BLACK);
            colL2.setPixelColor(centerx + 4, colours::BLACK);
            colL2.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_LEFT1:

            colL3.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx + 2, colours::BLACK);
            colL3.setPixelColor(centerx + 4, colours::BLACK);
            colL3.setPixelColor(centerx + 6, colours::BLACK);
            break;
        case PIN_RIGHT1:

            col2.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx + 2, colours::BLACK);
            col2.setPixelColor(centerx + 4, colours::BLACK);
            col2.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_RIGHT2:

            col1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx + 2, colours::BLACK);
            col1.setPixelColor(centerx + 4, colours::BLACK);
            col1.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR1.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx + 2, colours::BLACK);
            colR1.setPixelColor(centerx + 4, colours::BLACK);
            colR1.setPixelColor(centerx + 6, colours::BLACK);

            break;
        case PIN_RIGHT3 - 1:
            colR2.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);
            colR2.setPixelColor(centerx + 4, colours::BLACK);
            colR2.setPixelColor(centerx + 2, colours::BLACK);

            break;
        }
    }
    if (currentI == 1)
    {
        switch (centery1b)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::BLACK);
            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::BLACK);
            colL1.setPixelColor(centerx, colours::BLACK);
            colL2.setPixelColor(centerx, colours::BLACK);
            colL3.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);
            col3.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);
            col2.setPixelColor(centerx, colours::BLACK);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::BLACK);
            colR2.setPixelColor(centerx, colours::BLACK);
            colR1.setPixelColor(centerx, colours::BLACK);
            col1.setPixelColor(centerx, colours::BLACK);

            break;
        }
    }
}
// light
void lightO(int centerx)
{
    switch (centery1)
    {
    case PIN:
        col2.setPixelColor(centerx, colours::GREEN);
        col2.setPixelColor(centerx + 2, colours::GREEN);
        col3.setPixelColor(centerx, colours::GREEN);
        col3.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_2:
        col3.setPixelColor(centerx, colours::GREEN);
        col3.setPixelColor(centerx + 2, colours::GREEN);
        col4.setPixelColor(centerx, colours::GREEN);
        col4.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_3:
        col4.setPixelColor(centerx, colours::GREEN);
        col4.setPixelColor(centerx + 2, colours::GREEN);
        colL1.setPixelColor(centerx, colours::GREEN);
        colL1.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_4:
        colL1.setPixelColor(centerx, colours::GREEN);
        colL1.setPixelColor(centerx + 2, colours::GREEN);
        colL2.setPixelColor(centerx, colours::GREEN);
        colL2.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_LEFT1:
        colL2.setPixelColor(centerx, colours::GREEN);
        colL2.setPixelColor(centerx + 2, colours::GREEN);
        colL3.setPixelColor(centerx, colours::GREEN);
        colL3.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_RIGHT1:
        col1.setPixelColor(centerx, colours::GREEN);
        col1.setPixelColor(centerx + 2, colours::GREEN);
        col2.setPixelColor(centerx, colours::GREEN);
        col2.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_RIGHT2:
        colR1.setPixelColor(centerx, colours::GREEN);
        colR1.setPixelColor(centerx + 2, colours::GREEN);
        col1.setPixelColor(centerx, colours::GREEN);
        col1.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_RIGHT3:
        colR2.setPixelColor(centerx, colours::GREEN);
        colR2.setPixelColor(centerx + 2, colours::GREEN);
        colR1.setPixelColor(centerx, colours::GREEN);
        colR1.setPixelColor(centerx + 2, colours::GREEN);
        break;
    case PIN_RIGHT3 - 1:
        colR3.setPixelColor(centerx, colours::GREEN);
        colR3.setPixelColor(centerx + 2, colours::GREEN);
        colR2.setPixelColor(centerx, colours::GREEN);
        colR2.setPixelColor(centerx + 2, colours::GREEN);
        break;
    }
}

void lightL(int centerx)
{
    if (currentL == 0)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx + 4, colours::RED);
            col3.setPixelColor(centerx, colours::RED);
            col3.setPixelColor(centerx + 2, colours::RED);
            col3.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_2:

            col3.setPixelColor(centerx + 4, colours::RED);
            col4.setPixelColor(centerx, colours::RED);
            col4.setPixelColor(centerx + 2, colours::RED);
            col4.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_3:

            col4.setPixelColor(centerx + 4, colours::RED);
            colL1.setPixelColor(centerx, colours::RED);
            colL1.setPixelColor(centerx + 2, colours::RED);
            colL1.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_4:

            colL1.setPixelColor(centerx + 4, colours::RED);
            colL2.setPixelColor(centerx, colours::RED);
            colL2.setPixelColor(centerx + 2, colours::RED);
            colL2.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx + 4, colours::RED);
            colL3.setPixelColor(centerx, colours::RED);
            colL3.setPixelColor(centerx + 2, colours::RED);
            colL3.setPixelColor(centerx + 4, colours::RED);
            break;
        case PIN_RIGHT1:

            col1.setPixelColor(centerx + 4, colours::RED);
            col2.setPixelColor(centerx, colours::RED);
            col2.setPixelColor(centerx + 2, colours::RED);
            col2.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_RIGHT2:

            colR1.setPixelColor(centerx + 4, colours::RED);
            col1.setPixelColor(centerx, colours::RED);
            col1.setPixelColor(centerx + 2, colours::RED);
            col1.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_RIGHT3:

            colR2.setPixelColor(centerx + 4, colours::RED);
            colR1.setPixelColor(centerx, colours::RED);
            colR1.setPixelColor(centerx + 2, colours::RED);
            colR1.setPixelColor(centerx + 4, colours::RED);

            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 4, colours::RED);
            colR2.setPixelColor(centerx, colours::RED);
            colR2.setPixelColor(centerx + 2, colours::RED);
            colR2.setPixelColor(centerx + 4, colours::RED);

            break;
        }
    }
    if (currentL == 1)
    {
        switch (centery1)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::RED);
            col2.setPixelColor(centerx, colours::RED);
            col3.setPixelColor(centerx, colours::RED);
            col3.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::RED);
            col3.setPixelColor(centerx, colours::RED);
            col4.setPixelColor(centerx, colours::RED);
            col4.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::RED);
            col4.setPixelColor(centerx, colours::RED);
            colL1.setPixelColor(centerx, colours::RED);
            colL1.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::RED);
            colL1.setPixelColor(centerx, colours::RED);
            colL2.setPixelColor(centerx, colours::RED);
            colL2.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_LEFT1:

            colL1.setPixelColor(centerx, colours::RED);
            colL2.setPixelColor(centerx, colours::RED);
            colL3.setPixelColor(centerx, colours::RED);
            colL3.setPixelColor(centerx + 2, colours::RED);
            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::RED);
            col1.setPixelColor(centerx, colours::RED);
            col2.setPixelColor(centerx, colours::RED);
            col2.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::RED);
            colR1.setPixelColor(centerx, colours::RED);
            col1.setPixelColor(centerx, colours::RED);
            col1.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::RED);
            colR2.setPixelColor(centerx, colours::RED);
            colR1.setPixelColor(centerx, colours::RED);
            colR1.setPixelColor(centerx + 2, colours::RED);

            break;
        }
    }
    if (currentL == 2)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx, colours::RED);
            col2.setPixelColor(centerx + 2, colours::RED);
            col2.setPixelColor(centerx + 4, colours::RED);
            col3.setPixelColor(centerx, colours::RED);

            break;
        case PIN_2:

            col3.setPixelColor(centerx, colours::RED);
            col3.setPixelColor(centerx + 2, colours::RED);
            col3.setPixelColor(centerx + 4, colours::RED);
            col4.setPixelColor(centerx, colours::RED);

            break;
        case PIN_3:

            col4.setPixelColor(centerx, colours::RED);
            col4.setPixelColor(centerx + 2, colours::RED);
            col4.setPixelColor(centerx + 4, colours::RED);
            colL1.setPixelColor(centerx, colours::RED);

            break;
        case PIN_4:

            colL1.setPixelColor(centerx, colours::RED);
            colL1.setPixelColor(centerx + 2, colours::RED);
            colL1.setPixelColor(centerx + 4, colours::RED);
            colL2.setPixelColor(centerx, colours::RED);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx, colours::RED);
            colL2.setPixelColor(centerx + 2, colours::RED);
            colL2.setPixelColor(centerx + 4, colours::RED);
            colL3.setPixelColor(centerx, colours::RED);
            break;
        case PIN_RIGHT1:

            col1.setPixelColor(centerx, colours::RED);
            col1.setPixelColor(centerx + 2, colours::RED);
            col1.setPixelColor(centerx + 4, colours::RED);
            col2.setPixelColor(centerx, colours::RED);

            break;
        case PIN_RIGHT2:

            colR1.setPixelColor(centerx, colours::RED);
            colR1.setPixelColor(centerx + 2, colours::RED);
            colR1.setPixelColor(centerx + 4, colours::RED);
            col1.setPixelColor(centerx, colours::RED);

            break;
        case PIN_RIGHT3:

            colR2.setPixelColor(centerx, colours::RED);
            colR2.setPixelColor(centerx + 2, colours::RED);
            colR2.setPixelColor(centerx + 4, colours::RED);
            colR1.setPixelColor(centerx, colours::RED);

            break;
        case PIN_RIGHT3 - 1:

            colR3.setPixelColor(centerx, colours::RED);
            colR3.setPixelColor(centerx + 2, colours::RED);
            colR3.setPixelColor(centerx + 4, colours::RED);
            colR2.setPixelColor(centerx, colours::RED);

            break;
        }
    }
    if (currentL == 3)
    {
        switch (centery1)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::RED);
            col1.setPixelColor(centerx + 2, colours::RED);
            col2.setPixelColor(centerx + 2, colours::RED);
            col3.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::RED);
            col2.setPixelColor(centerx + 2, colours::RED);
            col3.setPixelColor(centerx + 2, colours::RED);
            col4.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::RED);
            col3.setPixelColor(centerx + 2, colours::RED);
            col4.setPixelColor(centerx + 2, colours::RED);
            colL1.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::RED);
            col4.setPixelColor(centerx + 2, colours::RED);
            colL1.setPixelColor(centerx + 2, colours::RED);
            colL2.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_LEFT1:

            colL1.setPixelColor(centerx, colours::RED);
            colL1.setPixelColor(centerx + 2, colours::RED);
            colL2.setPixelColor(centerx + 2, colours::RED);
            colL3.setPixelColor(centerx + 2, colours::RED);
            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::RED);
            colR1.setPixelColor(centerx + 2, colours::RED);
            col1.setPixelColor(centerx + 2, colours::RED);
            col2.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::RED);
            colR2.setPixelColor(centerx + 2, colours::RED);
            colR1.setPixelColor(centerx + 2, colours::RED);
            col1.setPixelColor(centerx + 2, colours::RED);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::RED);
            colR3.setPixelColor(centerx + 2, colours::RED);
            colR2.setPixelColor(centerx + 2, colours::RED);
            colR1.setPixelColor(centerx + 2, colours::RED);

            break;
        }
    }
}

void lightZ(int centerx)
{
    if (currentZ == 0)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx + 2, colours::YELLOW);
            col2.setPixelColor(centerx + 4, colours::YELLOW);
            col3.setPixelColor(centerx, colours::YELLOW);
            col3.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_2:

            col3.setPixelColor(centerx + 2, colours::YELLOW);
            col3.setPixelColor(centerx + 4, colours::YELLOW);
            col4.setPixelColor(centerx, colours::YELLOW);
            col4.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_3:

            col4.setPixelColor(centerx + 2, colours::YELLOW);
            col4.setPixelColor(centerx + 4, colours::YELLOW);
            colL1.setPixelColor(centerx, colours::YELLOW);
            colL1.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_4:

            colL1.setPixelColor(centerx + 2, colours::YELLOW);
            colL1.setPixelColor(centerx + 4, colours::YELLOW);
            colL2.setPixelColor(centerx, colours::YELLOW);
            colL2.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx + 2, colours::YELLOW);
            colL2.setPixelColor(centerx + 4, colours::YELLOW);
            colL3.setPixelColor(centerx, colours::YELLOW);
            colL3.setPixelColor(centerx + 2, colours::YELLOW);
            break;
        case PIN_RIGHT1:

            col1.setPixelColor(centerx + 2, colours::YELLOW);
            col1.setPixelColor(centerx + 4, colours::YELLOW);
            col2.setPixelColor(centerx, colours::YELLOW);
            col2.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_RIGHT2:

            colR1.setPixelColor(centerx + 2, colours::YELLOW);
            colR1.setPixelColor(centerx + 4, colours::YELLOW);
            col1.setPixelColor(centerx, colours::YELLOW);
            col1.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_RIGHT3:

            colR2.setPixelColor(centerx + 2, colours::YELLOW);
            colR2.setPixelColor(centerx + 4, colours::YELLOW);
            colR1.setPixelColor(centerx, colours::YELLOW);
            colR1.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 2, colours::YELLOW);
            colR3.setPixelColor(centerx + 4, colours::YELLOW);
            colR2.setPixelColor(centerx, colours::YELLOW);
            colR2.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        }
    }
    if (currentZ == 1)
    {
        switch (centery1)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::YELLOW);
            col2.setPixelColor(centerx, colours::YELLOW);
            col2.setPixelColor(centerx + 2, colours::YELLOW);
            col3.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::YELLOW);
            col3.setPixelColor(centerx, colours::YELLOW);
            col3.setPixelColor(centerx + 2, colours::YELLOW);
            col4.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::YELLOW);
            col4.setPixelColor(centerx, colours::YELLOW);
            col4.setPixelColor(centerx + 2, colours::YELLOW);
            colL1.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::YELLOW);
            colL1.setPixelColor(centerx, colours::YELLOW);
            colL1.setPixelColor(centerx + 2, colours::YELLOW);
            colL2.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_LEFT1:

            colL1.setPixelColor(centerx, colours::YELLOW);
            colL2.setPixelColor(centerx, colours::YELLOW);
            colL2.setPixelColor(centerx + 2, colours::YELLOW);
            colL3.setPixelColor(centerx + 2, colours::YELLOW);
            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::YELLOW);
            col1.setPixelColor(centerx, colours::YELLOW);
            col1.setPixelColor(centerx + 2, colours::YELLOW);
            col2.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::YELLOW);
            colR1.setPixelColor(centerx, colours::YELLOW);
            colR1.setPixelColor(centerx + 2, colours::YELLOW);
            col1.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::YELLOW);
            colR2.setPixelColor(centerx, colours::YELLOW);
            colR2.setPixelColor(centerx + 2, colours::YELLOW);
            colR1.setPixelColor(centerx + 2, colours::YELLOW);

            break;
        }
    }
}
void lightT(int centerx)
{
    if (currentT == 0)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx, colours::PURPLE);
            col2.setPixelColor(centerx + 2, colours::PURPLE);
            col2.setPixelColor(centerx + 4, colours::PURPLE);
            col3.setPixelColor(centerx + 2, colours::PURPLE);

            break;
        case PIN_2:
            col3.setPixelColor(centerx, colours::PURPLE);
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            col3.setPixelColor(centerx + 4, colours::PURPLE);
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_3:
            col4.setPixelColor(centerx, colours::PURPLE);
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            col4.setPixelColor(centerx + 4, colours::PURPLE);
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx, colours::PURPLE);
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            colL1.setPixelColor(centerx + 4, colours::PURPLE);
            colL2.setPixelColor(centerx + 2, colours::PURPLE);

            break;
        case PIN_LEFT1:

            colL2.setPixelColor(centerx, colours::PURPLE);
            colL2.setPixelColor(centerx + 2, colours::PURPLE);
            colL2.setPixelColor(centerx + 4, colours::PURPLE);
            colL3.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx, colours::PURPLE);
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            col1.setPixelColor(centerx + 4, colours::PURPLE);
            col2.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx, colours::PURPLE);
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            colR1.setPixelColor(centerx + 4, colours::PURPLE);
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx, colours::PURPLE);
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            colR2.setPixelColor(centerx + 4, colours::PURPLE);
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx, colours::PURPLE);
            colR3.setPixelColor(centerx + 2, colours::PURPLE);
            colR3.setPixelColor(centerx + 4, colours::PURPLE);
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        }
    }
    if (currentT == 1)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx + 2, colours::PURPLE);
            col3.setPixelColor(centerx, colours::PURPLE);
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            col4.setPixelColor(centerx + 2, colours::PURPLE);

            break;
        case PIN_2:
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            col4.setPixelColor(centerx, colours::PURPLE);
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_3:
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            colL1.setPixelColor(centerx, colours::PURPLE);
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            colL2.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            colL2.setPixelColor(centerx, colours::PURPLE);
            colL2.setPixelColor(centerx + 2, colours::PURPLE);
            colL3.setPixelColor(centerx + 2, colours::PURPLE);

            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            col2.setPixelColor(centerx, colours::PURPLE);
            col2.setPixelColor(centerx + 2, colours::PURPLE);
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            col1.setPixelColor(centerx, colours::PURPLE);
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            col2.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            colR1.setPixelColor(centerx, colours::PURPLE);
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 2, colours::PURPLE);
            colR2.setPixelColor(centerx, colours::PURPLE);
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            break;
        }
    }
    if (currentT == 2)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx + 2, colours::PURPLE);
            col3.setPixelColor(centerx, colours::PURPLE);
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            col3.setPixelColor(centerx + 4, colours::PURPLE);

            break;
        case PIN_2:
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            col4.setPixelColor(centerx, colours::PURPLE);
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            col4.setPixelColor(centerx + 4, colours::PURPLE);
            break;
        case PIN_3:
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            colL1.setPixelColor(centerx, colours::PURPLE);
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            colL1.setPixelColor(centerx + 4, colours::PURPLE);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            colL2.setPixelColor(centerx, colours::PURPLE);
            colL2.setPixelColor(centerx + 2, colours::PURPLE);
            colL2.setPixelColor(centerx + 4, colours::PURPLE);

            break;
        case PIN_LEFT1:
            colL2.setPixelColor(centerx + 2, colours::PURPLE);
            colL3.setPixelColor(centerx, colours::PURPLE);
            colL3.setPixelColor(centerx + 2, colours::PURPLE);
            colL3.setPixelColor(centerx + 4, colours::PURPLE);

            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            col2.setPixelColor(centerx, colours::PURPLE);
            col2.setPixelColor(centerx + 2, colours::PURPLE);
            col2.setPixelColor(centerx + 4, colours::PURPLE);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            col1.setPixelColor(centerx, colours::PURPLE);
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            col1.setPixelColor(centerx + 4, colours::PURPLE);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            colR1.setPixelColor(centerx, colours::PURPLE);
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            colR1.setPixelColor(centerx + 4, colours::PURPLE);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx + 2, colours::PURPLE);
            colR2.setPixelColor(centerx, colours::PURPLE);
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            colR2.setPixelColor(centerx + 4, colours::PURPLE);
            break;
        }
    }
    if (currentT == 3)
    {
        switch (centery1)
        {
        case PIN:

            col2.setPixelColor(centerx, colours::PURPLE);
            col3.setPixelColor(centerx + 2, colours::PURPLE);
            col3.setPixelColor(centerx, colours::PURPLE);
            col4.setPixelColor(centerx, colours::PURPLE);

            break;
        case PIN_2:
            col3.setPixelColor(centerx, colours::PURPLE);
            col4.setPixelColor(centerx + 2, colours::PURPLE);
            col4.setPixelColor(centerx, colours::PURPLE);
            colL1.setPixelColor(centerx, colours::PURPLE);
            break;
        case PIN_3:
            col4.setPixelColor(centerx, colours::PURPLE);
            colL1.setPixelColor(centerx + 2, colours::PURPLE);
            colL1.setPixelColor(centerx, colours::PURPLE);
            colL2.setPixelColor(centerx, colours::PURPLE);
            break;
        case PIN_4:
            colL1.setPixelColor(centerx, colours::PURPLE);
            colL2.setPixelColor(centerx + 2, colours::PURPLE);
            colL2.setPixelColor(centerx, colours::PURPLE);
            colL3.setPixelColor(centerx, colours::PURPLE);

            break;
        case PIN_RIGHT1:
            col1.setPixelColor(centerx, colours::PURPLE);
            col2.setPixelColor(centerx + 2, colours::PURPLE);
            col2.setPixelColor(centerx, colours::PURPLE);
            col3.setPixelColor(centerx, colours::PURPLE);
            break;
        case PIN_RIGHT2:
            colR1.setPixelColor(centerx, colours::PURPLE);
            col1.setPixelColor(centerx + 2, colours::PURPLE);
            col1.setPixelColor(centerx, colours::PURPLE);
            col2.setPixelColor(centerx, colours::PURPLE);
            break;
        case PIN_RIGHT3:
            colR2.setPixelColor(centerx, colours::PURPLE);
            colR1.setPixelColor(centerx + 2, colours::PURPLE);
            colR1.setPixelColor(centerx, colours::PURPLE);
            col1.setPixelColor(centerx, colours::PURPLE);
            break;
        case PIN_RIGHT3 - 1:
            colR3.setPixelColor(centerx, colours::PURPLE);
            colR2.setPixelColor(centerx + 2, colours::PURPLE);
            colR2.setPixelColor(centerx, colours::PURPLE);
            colR1.setPixelColor(centerx, colours::PURPLE);
            break;
        }
    }
}

void lightI(int centerx)
{
    if (currentI == 0)
    {
        switch (centery1)
        {
        case PIN:

            col3.setPixelColor(centerx, colours::BLUE);
            col3.setPixelColor(centerx + 2, colours::BLUE);
            col3.setPixelColor(centerx + 4, colours::BLUE);
            col3.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_2:

            col4.setPixelColor(centerx, colours::BLUE);
            col4.setPixelColor(centerx + 2, colours::BLUE);
            col4.setPixelColor(centerx + 4, colours::BLUE);
            col4.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_3:

            colL1.setPixelColor(centerx, colours::BLUE);
            colL1.setPixelColor(centerx + 2, colours::BLUE);
            colL1.setPixelColor(centerx + 4, colours::BLUE);
            colL1.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_4:

            colL2.setPixelColor(centerx, colours::BLUE);
            colL2.setPixelColor(centerx + 2, colours::BLUE);
            colL2.setPixelColor(centerx + 4, colours::BLUE);
            colL2.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_LEFT1:

            colL3.setPixelColor(centerx, colours::BLUE);
            colL3.setPixelColor(centerx + 2, colours::BLUE);
            colL3.setPixelColor(centerx + 4, colours::BLUE);
            colL3.setPixelColor(centerx + 6, colours::BLUE);
            break;
        case PIN_RIGHT1:

            col2.setPixelColor(centerx, colours::BLUE);
            col2.setPixelColor(centerx + 2, colours::BLUE);
            col2.setPixelColor(centerx + 4, colours::BLUE);
            col2.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_RIGHT2:

            col1.setPixelColor(centerx, colours::BLUE);
            col1.setPixelColor(centerx + 2, colours::BLUE);
            col1.setPixelColor(centerx + 4, colours::BLUE);
            col1.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_RIGHT3:

            colR1.setPixelColor(centerx, colours::BLUE);
            colR1.setPixelColor(centerx + 2, colours::BLUE);
            colR1.setPixelColor(centerx + 4, colours::BLUE);
            colR1.setPixelColor(centerx + 6, colours::BLUE);

            break;
        case PIN_RIGHT3 - 1:
            colR2.setPixelColor(centerx, colours::BLUE);
            colR2.setPixelColor(centerx + 2, colours::BLUE);
            colR2.setPixelColor(centerx + 4, colours::BLUE);
            colR2.setPixelColor(centerx + 2, colours::BLUE);

            break;
        }
    }
    if (currentI == 1)
    {
        switch (centery1)
        {
        case PIN:

            col1.setPixelColor(centerx, colours::BLUE);
            col2.setPixelColor(centerx, colours::BLUE);
            col3.setPixelColor(centerx, colours::BLUE);
            col4.setPixelColor(centerx, colours::BLUE);

            break;
        case PIN_2:

            col2.setPixelColor(centerx, colours::BLUE);
            col3.setPixelColor(centerx, colours::BLUE);
            col4.setPixelColor(centerx, colours::BLUE);
            colL1.setPixelColor(centerx, colours::BLUE);

            break;
        case PIN_3:

            col3.setPixelColor(centerx, colours::BLUE);
            col4.setPixelColor(centerx, colours::BLUE);
            colL1.setPixelColor(centerx, colours::BLUE);
            colL2.setPixelColor(centerx, colours::BLUE);

            break;
        case PIN_4:

            col4.setPixelColor(centerx, colours::BLUE);
            colL1.setPixelColor(centerx, colours::BLUE);
            colL2.setPixelColor(centerx, colours::BLUE);
            colL3.setPixelColor(centerx, colours::BLUE);

            break;
        case PIN_RIGHT1:

            colR1.setPixelColor(centerx, colours::BLUE);
            col1.setPixelColor(centerx, colours::BLUE);
            col2.setPixelColor(centerx, colours::BLUE);
            col3.setPixelColor(centerx, colours::BLUE);

            break;
        case PIN_RIGHT2:

            colR2.setPixelColor(centerx, colours::BLUE);
            colR1.setPixelColor(centerx, colours::BLUE);
            col1.setPixelColor(centerx, colours::BLUE);
            col2.setPixelColor(centerx, colours::BLUE);

            break;
        case PIN_RIGHT3:

            colR3.setPixelColor(centerx, colours::BLUE);
            colR2.setPixelColor(centerx, colours::BLUE);
            colR1.setPixelColor(centerx, colours::BLUE);
            col1.setPixelColor(centerx, colours::BLUE);

            break;
        }
    }
}
// light for shape prediction
void lightOPre()
{
    colPL.setPixelColor(0, colours::GREEN);
    colPL.setPixelColor(2, colours::GREEN);
    colPR.setPixelColor(0, colours::GREEN);
    colPR.setPixelColor(2, colours::GREEN);
}
void lightTPre()
{
    colPR.setPixelColor(0, colours::PURPLE);
    colPR.setPixelColor(2, colours::PURPLE);
    colPR.setPixelColor(4, colours::PURPLE);
    colPL.setPixelColor(2, colours::PURPLE);
}
void lightZPre()
{
    colPL.setPixelColor(0, colours::YELLOW);
    colPL.setPixelColor(2, colours::YELLOW);
    colPR.setPixelColor(2, colours::YELLOW);
    colPR.setPixelColor(4, colours::YELLOW);
}
void lightLPre()
{
    colPR.setPixelColor(4, colours::RED);
    colPL.setPixelColor(0, colours::RED);
    colPL.setPixelColor(2, colours::RED);
    colPL.setPixelColor(4, colours::RED);
}
void lightIPre()
{
    colPL.setPixelColor(0, colours::BLUE);
    colPL.setPixelColor(2, colours::BLUE);
    colPL.setPixelColor(4, colours::BLUE);
    colPL.setPixelColor(6, colours::BLUE);
}

// turnoff for shape prediction
void turnoffOPre()
{
    colPL.setPixelColor(0, colours::BLACK);
    colPL.setPixelColor(2, colours::BLACK);
    colPR.setPixelColor(0, colours::BLACK);
    colPR.setPixelColor(2, colours::BLACK);
}
void turnoffTPre()
{
    colPR.setPixelColor(0, colours::BLACK);
    colPR.setPixelColor(2, colours::BLACK);
    colPR.setPixelColor(4, colours::BLACK);
    colPL.setPixelColor(2, colours::BLACK);
}
void turnoffZPre()
{
    colPL.setPixelColor(0, colours::BLACK);
    colPL.setPixelColor(2, colours::BLACK);
    colPR.setPixelColor(2, colours::BLACK);
    colPR.setPixelColor(4, colours::BLACK);
}
void turnoffLPre()
{
    colPR.setPixelColor(4, colours::BLACK);
    colPL.setPixelColor(0, colours::BLACK);
    colPL.setPixelColor(2, colours::BLACK);
    colPL.setPixelColor(4, colours::BLACK);
}
void turnoffIPre()
{
    colPL.setPixelColor(0, colours::BLACK);
    colPL.setPixelColor(2, colours::BLACK);
    colPL.setPixelColor(4, colours::BLACK);
    colPL.setPixelColor(6, colours::BLACK);
}

void displayScore(int score)
{
    int ones = 0;
    int tens = 0;
    if (score <= 9)
    {
        ones = score;
    }
    else if (score > 9)
    {
        ones = score % 10;
        tens = score / 10;
    }

    for (int j = 0; j < 7; j++)
    {
        digitalWrite(ones_score_pin[j], numbers[ones][j]);
        digitalWrite(tens_score_pin[j], numbers[tens][j]);
    }
}

void displayHighscore(int score)
{
    int ones = 0;
    int tens = 0;
    if (score <= 9)
    {
        ones = score;
    }
    else if (score > 9)
    {
        ones = score % 10;
        tens = score / 10;
    }

    for (int j = 0; j < 7; j++)
    {
        digitalWrite(ones_highscore_pin[j], numbers[ones][j]);
        digitalWrite(tens_highscore_pin[j], numbers[tens][j]);
    }
}

void audioControl()
{
    char wavFile[33];
    switch (audio_state)
    {
    case add_point:
        strcpy_P(wavFile, wav_table[0]);
        audio.play(wavFile, 1);
        break; // Play point sound on pins 5,2
    case game_over:
        strcpy_P(wavFile, wav_table[1]);
        audio.play(wavFile, 1);
        break; // Play gameover sound on pins 5,2
    case background:
        strcpy_P(wavFile, wav_table[2]);
        audio.play(wavFile, 0);
        break; // Play background music on pins 6,7
    case menu:
        strcpy_P(wavFile, wav_table[3]);
        audio.play(wavFile, 0);
        break; // Play background music on pins 6,7
    case increase:
        audio.volume(1);
        break; // Increase volume by 1
    case decrease:
        audio.volume(0);
        break; // Decrease volume by 1
    case mute_bg:
        audio.setVolume(0, 0);
        break; // mute background on output 0
    case unmute_bg:
        audio.setVolume(3, 0);
        break; // Unmute background on output 0
    case stop_sound:
        audio.stopPlayback(1);
        break; // Stop playback on output 0
    case stop_all:
        audio.stopPlayback();
        break; // Stop all playback
    default:
        break;
    }
    audio_state = default_null;
}