// attiny85: 8K flash, 512B eeprom, 512B sram
// fuses: lfuse E2, hfuse DF, ext FF

#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#define LED_COUNT 20

#define SPIPORT PORTB
#define SPIDDR DDRB
#define SPICLK _BV(2)
#define SPIDATA _BV(1)

static uint8_t r[LED_COUNT], g[LED_COUNT], b[LED_COUNT];
static uint8_t brightness;

/*
 * Output one byte. Set data, set clock, unset clock.
 * Slave samples on rising edge.
 * */
static void spi_out(uint8_t data) {
	for (uint8_t i = 0; i < 8; i++) {
		if (data & 0x80)
			SPIPORT |= SPIDATA;
		else
			SPIPORT &= ~SPIDATA;
		SPIPORT |= SPICLK;
		data <<= 1;
// 		_delay_us(1);
		SPIPORT &= ~SPICLK;
	}
}

static void spi_setup() {
	SPIDDR |= SPICLK;
	SPIDDR |= SPIDATA;
}

static void output_all() {
	for (uint8_t i = 0; i < LED_COUNT; i++) {
		spi_out(b[i]);
		spi_out(g[i]);
		spi_out(r[i]);
	}
	_delay_ms(1);
}

static void rainbow() {
	static uint8_t sstep=0, smode = 0, srr, sgg, sbb;
#define spets (255 - sstep)
	switch (smode) {
		case 0: srr = sstep; break; // 0 -> r
		case 1: sgg = sstep; break; // r -> rg
		case 2: srr = spets; break; // rg -> g
		case 3: sbb = sstep; break; // g -> gb
		case 4: srr = sstep; break; // gb -> rgb
		case 5: sgg = spets; break; // rgb -> rb
		case 6: srr = spets; break; // rb -> b
		case 7: sbb = spets; break; // b -> 0
	}

	uint8_t step, mode;
	uint8_t rr, gg, bb;
	mode=smode;step=sstep;rr=srr;gg=sgg;bb=sbb;
	if ((sstep += 32) == 0) if (++smode == 8) smode = 0;
	
	
#define pets (255 - step)
	for (uint8_t x = 0; x < LED_COUNT; x++) {
		switch (mode) {
			case 0: rr = step; break; // 0 -> r
			case 1: gg = step; break; // r -> rg
			case 2: rr = pets; break; // rg -> g
			case 3: bb = step; break; // g -> gb
			case 4: rr = step; break; // gb -> rgb
			case 5: gg = pets; break; // rgb -> rb
			case 6: rr = pets; break; // rb -> b
			case 7: bb = pets; break; // b -> 0
		}
		r[x] = rr;
		g[x] = gg;
		b[x] = bb;
		if ((step += 32) == 0) if (++mode == 8) mode = 0;
	}
	_delay_ms(25);
}

static void rollpoints() {
	static uint8_t cmode, cc, pos;
#define omg(x,v,k) x[(v)%LED_COUNT] = k
#define zing2(x, q) omg(x, q, c>>4); omg(x, q+1, c>>2); omg(x, q+2, c); omg(x, q+3, c>>2); omg(x, q+4, c>>4)
#define zing(x, q) omg(x, q+1, c>>2); omg(x, q+2, c); omg(x, q+3, c>>4)
#define hax(x) zing(x, pos); zing(x, pos + LED_COUNT / 2)
	
		cc += 4;
		uint8_t c = cc <= 127 ? cc : 255 - cc;
		if (cc == 0) cmode++;
		
		pos+=1;
		pos %= LED_COUNT;

		uint8_t x;
		const int fadespd = 3;
		for (x = 0; x < LED_COUNT; x++) {
			//r[x]=g[x]=b[x]=0;
			r[x] = r[x] <= fadespd ? 0 : r[x] - fadespd;
			g[x] = g[x] <= fadespd ? 0 : g[x] - fadespd;
			b[x] = b[x] <= fadespd ? 0 : b[x] - fadespd;
		}

		switch (cmode % 7) { // r, g, b, rg, gb, rb, rgb // r, rb
			case 0: // rgb
				hax(r);//r[p] = c; r[pp] = c;
			case 1: // gb
				hax(g);//g[p] = c; g[pp] = c;
			case 2: // b
				hax(b);//b[p] = c; b[pp] = c;
				break;
			case 3: // rg
				hax(r);//r[p] = c; r[pp] = c;
			case 4: // g
				hax(g);//g[p] = c; g[pp] = c;
				break;
			case 5: // rb
				hax(b);//b[p] = c; b[pp] = c;
			case 6: // r
				hax(r);//r[p] = c; r[pp] = c;
				break;
		}
		_delay_ms(50);
}

static void whites(uint8_t w) {
	memset(r, w, LED_COUNT);
	memset(g, w, LED_COUNT);
	memset(b, w, LED_COUNT);
}

static void dim(uint8_t shift) {
	while (shift--) {
		for (int i = 0; i < LED_COUNT; i++) {
			// avr can do just one at a time
			r[i] >>= 1;
			g[i] >>= 1;
			b[i] >>= 1;
		}
	}
}

static inline void button_setup() {
	DDRB |= _BV(3);
	DDRB |= _BV(4);
	// pull ups because buttons
	PORTB |= _BV(3);
	PORTB |= _BV(4);
}

static inline uint8_t buttons() {
	return (~PINB) & (_BV(3)|_BV(4));
}

int main() {
	spi_setup();
	button_setup();
	int time = 0;
	uint8_t prevbtn = buttons();
	uint8_t mode = 0;
	for (;;) {
		output_all();

		switch (mode) {
		case 0:
		case 1:
			time++;
			if (time < 200)
				rollpoints();
			else if (time < 200+400)
				rainbow();
			else
				time = 0;
			if (mode == 1) {
				dim(3);
			}
			break;
		case 2:
			whites(255);
			break;
		case 3:
			whites(50);
			break;
		case 4:
			whites(1);
			break;
		case 5:
			whites(0);
			break;
		}

		uint8_t curbtn = buttons();
		uint8_t diffbtn = curbtn ^ prevbtn;
		if (diffbtn && curbtn) {
			// "debounce"
			_delay_ms(50);
			curbtn = buttons();
			if (curbtn) {
				mode = mode == 5 ? 0 : mode + 1;
			}
		}
		prevbtn = curbtn;
	}
}
