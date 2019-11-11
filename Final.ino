#include <FastLED.h>

#define LED_PIN     6
#define LED_PIN2    7
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

const uint8_t kMatrixWidth  = 12;
const uint8_t kMatrixHeight = 12;
const bool    kMatrixSerpentineLayout = true;


// This example combines two features of FastLED to produce a remarkable range of
// effects from a relatively small amount of code.  This example combines FastLED's 
// color palette lookup functions with FastLED's Perlin/simplex noise generator, and
// the combination is extremely powerful.
//
// You might want to look at the "ColorPalette" and "Noise" examples separately
// if this example code seems daunting.
//
// 
// The basic setup here is that for each frame, we generate a new array of 
// 'noise' data, and then map it onto the LED matrix through a color palette.
//
// Periodically, the color palette is changed, and new noise-generation parameters
// are chosen at the same time.  In this example, specific noise-generation
// values have been selected to match the given color palettes; some are faster, 
// or slower, or larger, or smaller than others, but there's no reason these 
// parameters can't be freely mixed-and-matched.
//
// In addition, this example includes some fast automatic 'data smoothing' at 
// lower noise speeds to help produce smoother animations in those cases.
//
// The FastLED built-in color palettes (Forest, Clouds, Lava, Ocean, Party) are
// used, as well as some 'hand-defined' ones, and some proceedurally generated
// palettes.


#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)

// The leds
CRGB leds[kMatrixWidth * kMatrixHeight];
CRGB leds2[kMatrixWidth * kMatrixHeight];

// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
uint16_t speed = 5; // speed is set dynamically once we've started up

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t scale = 30; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];

CRGBPalette16 currentPalette( LavaColors_p );
CRGBPalette16 currentPalette2( ForestColors_p );
uint8_t       colorLoop = 1;

 //Declare Variable for sensor data handling

const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

int inputPin = A0;


int readings2[numReadings];      // the readings from the analog input
int readIndex2 = 0;              // the index of the current reading
int total2 = 0;                  // the running total
int average2 = 0;                // the average

int inputPin2 = A2;


int Selector = 1;
int Selector2 = 2;

int Distance = 10;

void setup() {
  delay(3000);
  LEDS.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(leds,NUM_LEDS);
  LEDS.addLeds<LED_TYPE,LED_PIN2,COLOR_ORDER>(leds2,NUM_LEDS);
  LEDS.setBrightness(BRIGHTNESS);

  // Initialize our coordinates to some random values
  x = random16();
  y = random16();
  z = random16();


  // initialize serial communication with computer:
  Serial.begin(9600);
  // initialize all the readings to 0:
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
      }
      
  for (int i = 0; i < numReadings; i++) {
  readings2[i] = 0;
      }
 
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  
  } //End Setup
 

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if( speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  
  for(int i = 0; i < MAX_DIMENSION; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < MAX_DIMENSION; j++) {
      int joffset = scale * j;
      
      uint8_t data = inoise8(x + ioffset,y + joffset,z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }
      
      noise[i][j] = data;
    }
  }
  
  z += speed;
  
  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri =   noise[i][j];

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);
      leds[XY(i,j)] = color;

      CRGB color2 = ColorFromPalette( currentPalette2, index, bri);
      leds2[XY(i,j)] = color2;
    }
  }
  
  ihue+=1;
}



void loop() {
  // Periodically choose a new palette, speed, and scale
  //ChangePaletteAndSettingsPeriodically();



{ //Distance Sensor Input handling 
  // subtract the last reading:
  total = total - readings[readIndex];
  total2 = total2 - readings2[readIndex2];
  
  // read from the sensor:
  readings[readIndex] = analogRead(inputPin);
  readings2[readIndex2] = analogRead(inputPin2);
  
  // add the reading to the total:
  total = total + readings[readIndex];
  total2 = total2 + readings2[readIndex2];

  
  // advance to the next position in the array:
  readIndex = readIndex + 1;
  readIndex2 = readIndex2 + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  if (readIndex2 >= numReadings) {
    // ...wrap around to the beginning:
    readIndex2 = 0;
  }

  // calculate the average:
  average = total / numReadings;
  average2 = total2 / numReadings;

 int combinedSensor = (average+average2)/2;

  combinedSensor = constrain(combinedSensor, 60, 700);

  Distance = map(combinedSensor, 60, 700, 15, 3);
  
  // send it to the computer as ASCII digits
 //Serial.println(combinedSensor);
 // Serial.println(average2);
  delay(1);
}

int button0 = digitalRead(0);
int button1 = digitalRead(1);
int button2 = digitalRead(2);
int button3 = digitalRead(3);
int button4 = digitalRead(4);
int button5 = digitalRead(5);


if (button0 == LOW ) Selector = 0;
if (button1 == LOW ) Selector = 1;
if (button2 == LOW ) Selector = 2;

if (button3 == LOW ) Selector2 = 0;
if (button4 == LOW ) Selector2 = 2;
if (button5 == LOW ) Selector2 = 1;

Serial.println(button0);
Serial.println(button1);
Serial.println(button2);

ChangePaletteToData();

  // generate noise data
  fillnoise8();
  
  // convert the noise data to colors in the LED array
  // using the current palette
  mapNoiseToLEDsUsingPalette();

  LEDS.show();
  // delay(10);
}//End Loop



// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.

// 1 = 5 sec per palette
// 2 = 10 sec per palette
// etc
#define HOLD_PALETTES_X_TIMES_AS_LONG 1


void ChangePaletteToData()
{
  uint8_t secondHand = ((millis() / 1000) / HOLD_PALETTES_X_TIMES_AS_LONG) % 60;
  static uint8_t lastSecond = 99;
  
  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    if( Selector == 0)  { currentPalette = LavaColors_p;           speed =  Distance; scale = 120; colorLoop = 0; }
    if( Selector == 1)  { currentPalette = CloudColors_p;           speed =  Distance; scale = 80; colorLoop = 0; }
    if( Selector == 2)  { currentPalette = LavaColors_p;            speed =  Distance; scale = 50; colorLoop = 0; }
    if( Selector == 3)  { currentPalette = OceanColors_p;           speed = Distance; scale = 90; colorLoop = 0; }

    if( Selector2 == 0)  { currentPalette2 = ForestColors_p;          speed =  Distance; scale = 120; colorLoop = 0; }
    if( Selector2 == 1)  { currentPalette2 = CloudColors_p;           speed =  Distance; scale = 30; colorLoop = 0; }
    if( Selector2 == 2)  { currentPalette2 = LavaColors_p;            speed =  Distance; scale = 50; colorLoop = 0; }
    if( Selector2 == 3)  { currentPalette2 = OceanColors_p;           speed = Distance; scale = 90; colorLoop = 0; }
  }
}

void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    green,  green,  black,  black,
    purple, purple, black,  black,
    green,  green,  black,  black,
    purple, purple, black,  black );
}


//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }
  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  return i;
}
