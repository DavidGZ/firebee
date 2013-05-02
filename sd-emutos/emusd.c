/*
 * emusd.c
 *
 * The driver actually resides within BaS_gcc. All we need to do within the AUTO-folder program is to find the driver
 * entry point and put its address into the XHDI cookie
 *
 *  Created on: 01.05.2013
 *      Author: mfro
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <osbind.h>

#define XHDIMAGIC 0x27011992L

typedef uint32_t (*cookie_fun)(uint16_t opcode,...);

static cookie_fun old_vector = NULL;

static long cookieptr (void)
{
	return * (uint32_t *) 0x5a0L;
}

static int getcookie(uint32_t cookie, uint32_t *p_value)
{
	long *cookiejar = (long *) Supexec (cookieptr);

	if (!cookiejar) return 0;

	do
	{
		if (cookiejar[0] == cookie)
		{
			if (p_value) *p_value = cookiejar[1];
			return 1;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while (cookiejar[-2]);

	return 0;
}

static void setcookie(uint32_t cookie, uint32_t value)
{
	long *cookiejar = (long *) Supexec(cookieptr);

	do
	{
		if (cookiejar[0] == cookie)
		{
			cookiejar[1] = value;
			return;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while (cookiejar[-2]);
}

static cookie_fun get_fun_ptr(void)
{
	static cookie_fun XHDI = NULL;
	static int have_it = 0;

	if (!have_it)
	{
		uint32_t *magic_test;

		getcookie ('XHDI', (uint32_t *) &XHDI);
		have_it = 1;

		/* check magic */

		magic_test = (uint32_t *)XHDI;
		if (magic_test && (magic_test[-1] != XHDIMAGIC))
			XHDI = NULL;
	}

	return XHDI;
}

__extension__ cookie_fun bas_sd_vector(cookie_fun old_vector)
{
	register long retvalue __asm__("d0");

	__asm__ __volatile(
			"move.l	%1,-(sp)\n\t"
			"trap   #0\n\t"
			"addq.l	#4,sp\n\t"
			: "=r"(retvalue)
			: "g"(old_vector)
			:
	);
	retvalue;
}

int main(int argc, char *argv[])
{
	uint32_t value;
	cookie_fun bas_vector;

	if (getcookie('XHDI', &value))
	{
		if ((old_vector = get_fun_ptr()))
		{
			printf("old XHDI vector (%p) found and saved\r\n", old_vector);
		}
	}

	bas_vector = bas_sd_vector(old_vector);
	printf("got vector from BaS: %p\r\n", bas_vector);
	setcookie('XHDI', (uint32_t) bas_vector);

	printf("vector to BaS driver set\r\n");

	return 0;
}
