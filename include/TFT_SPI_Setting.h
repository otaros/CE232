#define USER_SETUP_INFO "ESP32 S3 ILI9341"

#define ILI9341_DRIVER

#define TFT_CS 
#define TFT_MOSI 
#define TFT_SCLK
#define TFT_MISO

#define TFT_DC
#define TFT_RST

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY 40000000 // Maximum for ILI9341

#define SPI_READ_FREQUENCY 6000000 // 6 MHz is the maximum SPI read speed for the ST7789V

#define SPI_TOUCH_FREQUENCY 2500000