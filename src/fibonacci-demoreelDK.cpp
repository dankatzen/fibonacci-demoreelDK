#include <Arduino.h>

/*
   Fibonacci Demo Reel: https://github.com/jasoncoon/fibonacci-demoreel
   Copyright (C) 2020 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FastLED.h>  // https://github.com/FastLED/FastLED
#include <Button.h>   // https://github.com/madleech/Button

FASTLED_USING_NAMESPACE

#include "GradientPalettes.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define DATA_PIN      4
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      64

#define MILLI_AMPS         2000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  120  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.




CRGB leds[NUM_LEDS];

const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
int8_t brightnessIndex = 0;
uint8_t brightness = brightnessMap[brightnessIndex];

const uint8_t PIN_BUTTON_PATTERN = 3;
const uint8_t PIN_BUTTON_BRIGHTNESS = 2;

Button buttonBrightness(2);
Button buttonPattern(3);

uint8_t power = 1;

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
uint8_t secondsPerPalette = 10;

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
uint8_t cooling = 49;

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
uint8_t sparking = 60;

uint8_t speed = 30;



///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];

uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

int8_t currentPatternIndex = 0; // Index number of which pattern is current
uint8_t autoplay = 1;

uint8_t autoplayDuration = 10;
unsigned long autoPlayTimeout = 0;

uint8_t showClock = 0;
uint8_t clockBackgroundFade = 240;

uint8_t currentPaletteIndex = 0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

CRGB solidColor = CRGB::Blue;

// scale the brightness of all pixels down
void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value);
  }
}

typedef struct {
  CRGBPalette16 palette;
  String name;
} PaletteAndName;
typedef PaletteAndName PaletteAndNameList[];

const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};

const uint8_t paletteCount = ARRAY_SIZE(palettes);

typedef void (*Pattern)();
typedef Pattern PatternList[];

#include "Twinkles.h"
#include "TwinkleFOX.h"
#include "Map.h"

//FORWARD DECs

// Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino

void addGlitter( uint8_t chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}


// based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino
void heatMap(CRGBPalette16 palette, bool up)
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( uint16_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( uint16_t j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    colorindex = scale8(heat[j], 190);

    CRGB color = ColorFromPalette(palette, colorindex);

    if (up) {
      leds[j] = color;
    }
    else {
      leds[(NUM_LEDS - 1) - j] = color;
    }
  }
}


void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void rainbowSolid()
{
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  // leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(speed, 0, NUM_LEDS);
  static int prevpos = 0;
  CRGB color = ColorFromPalette(palettes[currentPaletteIndex], gHue, 255);
  if ( pos < prevpos ) {
    fill_solid( leds + pos, (prevpos - pos) + 1, color);
  } else {
    fill_solid( leds + prevpos, (pos - prevpos) + 1, color);
  }
  prevpos = pos;
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t beat = beatsin8( speed, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle()
{
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

 static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for ( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

void fire()
{
  heatMap(HeatColors_p, true);
}

void water()
{
  heatMap(IceColors_p, false);
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.



void fillWithPride(bool useFibonacciOrder) {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;

    if (useFibonacciOrder) pixelnumber = fibonacciToPhysical[i];
    
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

void pride() {
  fillWithPride(false);
}

void prideFibonacci() {
  fillWithPride(true);
}


// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void fillWithColorWaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette, bool useFibonacciOrder)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;

    if (useFibonacciOrder) pixelnumber = fibonacciToPhysical[i];
    
    pixelnumber = (numleds - 1) - pixelnumber;

    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette
// across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}


void colorWaves() {
  fillWithColorWaves(leds, NUM_LEDS, gCurrentPalette, false);
}

void colorWavesFibonacci() {
  fillWithColorWaves(leds, NUM_LEDS, gCurrentPalette, true);
}


// increase or decrease the current brightness, and wrap around at the ends
void adjustBrightness(bool up)
{
  if (up)
    brightnessIndex++;
  else
    brightnessIndex--;

  // wrap around at the ends
  if (brightnessIndex < 0)
    brightnessIndex = brightnessCount - 1;
  if (brightnessIndex >= brightnessCount)
    brightnessIndex = 0;

  brightness = brightnessMap[brightnessIndex];
  FastLED.setBrightness(brightness);

  Serial.print("brightness: ");
  Serial.println(brightness);
}




//#include "Noise.h"

// List of patterns to cycle through.  Each is defined as a separate function below.

PatternList patterns = {
//  pride,
  prideFibonacci,
//  colorWaves,
  colorWavesFibonacci,

  // matrix patterns
  anglePalette,
  radiusPalette,
  xPalette,
  yPalette,
  xyPalette,

  angleGradientPalette,
  radiusGradientPalette,
  xGradientPalette,
  yGradientPalette,
  xyGradientPalette,

//  // noise patterns
//  fireNoise,
//  fireNoise2,
//  lavaNoise,
//  rainbowNoise,
//  rainbowStripeNoise,
//  partyNoise,
//  forestNoise,
//  cloudNoise,
//  oceanNoise,
//  blackAndWhiteNoise,
//  blackAndBlueNoise,

  // twinkle patterns
  rainbowTwinkles,
  snowTwinkles,
  cloudTwinkles,
  incandescentTwinkles,

  // TwinkleFOX patterns
  retroC9Twinkles,
  redWhiteTwinkles,
  blueWhiteTwinkles,
  redGreenWhiteTwinkles,
  fairyLightTwinkles,
  snow2Twinkles,
  hollyTwinkles,
  iceTwinkles,
  partyTwinkles,
  forestTwinkles,
  lavaTwinkles,
  fireTwinkles,
  cloud2Twinkles,
  oceanTwinkles,
  rainbow,
  rainbowWithGlitter,
  rainbowSolid,
  confetti,
  sinelon,
  bpm,
  juggle,
  fire,
  water,

//  strandTest,
//
//  showSolidColor
};

const uint8_t patternCount = ARRAY_SIZE(patterns);

// increase or decrease the current pattern number, and wrap around at the ends
void adjustPattern(bool up)
{
  if (up)
    currentPatternIndex++;
  else
    currentPatternIndex--;

  // wrap around at the ends
  if (currentPatternIndex < 0)
    currentPatternIndex = patternCount - 1;
  if (currentPatternIndex >= patternCount)
    currentPatternIndex = 0;

  Serial.print("pattern: ");
  Serial.println(currentPatternIndex);
}


void handleInput() {
  if (buttonPattern.released())
  {
    Serial.println("Pattern button released");
    if (autoplay) {
      autoplay = 0;
      Serial.println("Autoplay: off");
    }
    adjustPattern(true);
  }

  if (buttonBrightness.released()) {
    Serial.println("Brightness button released");
    adjustBrightness(true);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  FastLED.setBrightness(brightness);

  autoPlayTimeout = millis() + (autoplayDuration * 1000);

  buttonBrightness.begin();
  buttonPattern.begin();
}

void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  handleInput();

  if (power == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(1000 / FRAMES_PER_SECOND);
    return;
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS( secondsPerPalette ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(40) {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 8);
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if (autoplay && (millis() > autoPlayTimeout)) {
    adjustPattern(true);
    autoPlayTimeout = millis() + (autoplayDuration * 1000);
  }

  // Call the current pattern function once, updating the 'leds' array
  patterns[currentPatternIndex]();

  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}


void strandTest()
{
  uint8_t i = speed;

  if (i >= NUM_LEDS) i = NUM_LEDS - 1;

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  leds[i] = solidColor;
}

void showSolidColor()
{
  fill_solid(leds, NUM_LEDS, solidColor);
}


void radialPaletteShift()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( gCurrentPalette, gHue + sin8(i*16), brightness);
    leds[fibonacciToPhysical[i]] = ColorFromPalette(gCurrentPalette, i + gHue, 255, LINEARBLEND);
  }
}

void radialPaletteShiftOutward()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[fibonacciToPhysical[i]] = ColorFromPalette(gCurrentPalette, i - gHue, 255, LINEARBLEND);
  }
}

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

uint8_t beatsaw8( accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255,
                  uint32_t timebase = 0, uint8_t phase_offset = 0)
{
  uint8_t beat = beat8( beats_per_minute, timebase);
  uint8_t beatsaw = beat + phase_offset;
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8( beatsaw, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}



