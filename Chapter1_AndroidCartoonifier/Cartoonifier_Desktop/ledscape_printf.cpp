/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>

extern const uint8_t fixed_font[][5];

void
ledscape_draw_char(
	uint32_t * px,
	const size_t width,
	const uint32_t color,
	char c
)
{
	if (c < 0x20 || c > 127)
		c = '?';

	const uint8_t* const f = fixed_font[c - 0x20];
	for (int i = 0 ; i < 5 ; i++, px++)
	{
		uint8_t bits = f[i];
		for (int j = 0 ; j < 7 ; j++, bits >>= 1)
			px[j*width] = bits & 1 ? color : 0;
	}
}


/** Write with a fixed-width 8px font */
void
ledscape_printf(
	uint32_t * px,
	const size_t width,
	const uint32_t color,
	const char * fmt,
	...
)
{
	char buf[128];
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	(void) len;
	uint32_t * start = px;

	//printf("%p => '%s'\n", px, buf);
	for (unsigned i = 0 ; i < sizeof(buf) ; i++)
	{
		char c = buf[i];
		if (!c)
			break;
		if (c == '\n')
		{
			px = start = start + 8 * width;
			continue;
		}

		ledscape_draw_char(px, width, color, c);
		px += 6;
	}
}
