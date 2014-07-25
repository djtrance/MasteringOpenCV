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
	uint8_t * dst,
	const size_t width,
	const uint32_t color,
	char c
)
{
	if (c < 0x20 || c > 127)
		c = '?';

	const uint8_t* const f = fixed_font[c - 0x20];
	for (int i = 0 ; i < 5 ; i++, dst+=3)
	{
		uint8_t bits = f[i];
		for (int j = 0 ; j < 7 ; j++, bits >>= 1)
		{
			dst[j*width*3+0] = bits & 1 ? (color >> 16) : 0;
			dst[j*width*3+1] = bits & 1 ? (color >> 8) : 0;
			dst[j*width*3+2] = bits & 1 ? (color >> 0) : 0;
		}
	}
}


/** Write with a fixed-width 8px font */
void
ledscape_printf(
	uint8_t * dst,
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
	uint8_t * first = dst;

	//printf("%p => '%s'\n", dst, buf);
	for (unsigned i = 0 ; i < sizeof(buf) ; i++)
	{
		char c = buf[i];
		if (!c)
			break;
		if (c == '\n')
		{
			dst = first = first + 8 * width * 3;
			continue;
		}

		ledscape_draw_char(dst, width, color, c);
		dst += 6 * 3;
	}
}
