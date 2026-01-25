#ifndef SMC_DRIVERS_ST7789V_H
#define SMC_DRIVERS_ST7789V_H

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#include <cstdint>

class Touchscreen {
	public:
		int init(void);
		void hline(uint16_t x, uint16_t y, uint16_t w, uint16_t c);
		void vline(uint16_t x, uint16_t y, uint16_t h, uint16_t c);
		void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
		void image_1bit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* image, uint16_t c);
		void image_2bit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* image, uint16_t c1, uint16_t c2, uint16_t c3);
		bool read_touch(uint16_t *x, uint16_t *y);

	private:
		void addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
		void rotate(int rotation);
		inline void push_pixel(uint16_t color);
		inline void write_command(uint8_t b);
		inline void write_data(uint8_t b);

};

#endif 
