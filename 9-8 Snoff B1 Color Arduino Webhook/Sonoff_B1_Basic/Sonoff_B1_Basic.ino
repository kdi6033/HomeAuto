#include <my92xx.h> //https://github.com/xoseperez/my92xx

#define MY92XX_MODEL    MY92XX_MODEL_MY9231     // The MY9291 is a 4-channel driver, usually for RGBW lights
#define MY92XX_CHIPS    2                       // No daisy-chain
#define MY92XX_DI_PIN   12                      // DI GPIO
#define MY92XX_DCKI_PIN 14                      // DCKI GPIO
#define UNIVERSE 1                              // First DMX Universe to listen for
#define UNIVERSE_COUNT 1                        // Total number of Universes to listen for, starting at UNIVERSE

my92xx _my92xx = my92xx(MY92XX_MODEL, MY92XX_CHIPS, MY92XX_DI_PIN, MY92XX_DCKI_PIN, MY92XX_COMMAND_DEFAULT);

// r g b w  0~255까지 입력
int r = 100;
int g = 0;
int b = 0;
int w = 100;

void updateLights()
{
      _my92xx.setChannel(4, r);
      _my92xx.setChannel(3, g);
      _my92xx.setChannel(5, b); 
      _my92xx.setChannel(0, w); 
      _my92xx.setChannel(1, w); 
      _my92xx.setState(true);
      _my92xx.update();
}

void setup() {

}

void loop() {
  updateLights();
  //delay(10); // 이시간을 길게하면 동작하지 않는다.
}
