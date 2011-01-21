/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/p2000t.h"
#include "sound/speaker.h"

#define P2000M_101F_CASDAT	0x01
#define P2000M_101F_CASCMD	0x02
#define P2000M_101F_CASREW	0x04
#define P2000M_101F_CASFOR	0x08
#define P2000M_101F_KEYINT	0x40
#define P2000M_101F_PRNOUT	0x80

#define	P2000M_202F_PINPUT	0x01
#define P2000M_202F_PREADY	0x02
#define	P2000M_202F_STRAPN	0x04
#define P2000M_202F_CASENB	0x08
#define P2000M_202F_CASPOS	0x10
#define P2000M_202F_CASEND	0x20
#define P2000M_202F_CASCLK	0x40
#define P2000M_202F_CASDAT	0x80

#define P2000M_303F_VIDEO	0x01

#define P2000M_707F_DISA	0x01

/*
    Keyboard port 0x0x

    If the keyboard interrupt is enabled, all keyboard matrix rows are
    connected and reading from either of these ports will give the
    keyboard status (FF=no key pressed)

    If the keyboard interrupt is disabled, reading one of these ports
    will read the corresponding keyboard matrix row
*/
READ8_HANDLER (	p2000t_port_000f_r )
{
	p2000t_state *state = space->machine->driver_data<p2000t_state>();
	static const char *const keynames[] = {
		"KEY0", "KEY1", "KEY2", "KEY3", "KEY4",
		"KEY5", "KEY6", "KEY7", "KEY8", "KEY9"
	};

	if (state->port_101f & P2000M_101F_KEYINT)
	{
		return (input_port_read(space->machine, "KEY0") & input_port_read(space->machine, "KEY1") &
		input_port_read(space->machine, "KEY2") & input_port_read(space->machine, "KEY3") &
		input_port_read(space->machine, "KEY4") & input_port_read(space->machine, "KEY5") &
		input_port_read(space->machine, "KEY6") & input_port_read(space->machine, "KEY7") &
		input_port_read(space->machine, "KEY8") & input_port_read(space->machine, "KEY9"));
	}
	else if (offset < 10)
	{
		return (input_port_read(space->machine, keynames[offset]));
	}
	return (0xff);
}


/*
    Input port 0x2x

    bit 0 - Printer input
    bit 1 - Printer ready
    bit 2 - Strap N (daisy/matrix)
    bit 3 - Cassette write enabled
    bit 4 - Cassette in position
    bit 5 - Begin/end of tape
    bit 6 - Cassette read clock
    bit 7 - Cassette read data
*/
READ8_HANDLER (	p2000t_port_202f_r )
{
	return (0xff);
}


/*
    Output Port 0x1x

    bit 0 - Cassette write data
    bit 1 - Cassette write command
    bit 2 - Cassette rewind
    bit 3 - Cassette forward
    bit 4 - Unused
    bit 5 - Unused
    bit 6 - Keyboard interrupt enable
    bit 7 - Printer output
*/
WRITE8_HANDLER ( p2000t_port_101f_w )
{
	p2000t_state *state = space->machine->driver_data<p2000t_state>();
	state->port_101f = data;
}

/*
    Scroll Register 0x3x (P2000T only)

    bit 0 - /
    bit 1 - |
    bit 2 - | Index of the first character
    bit 3 - | to be displayed
    bit 4 - |
    bit 5 - |
    bit 6 - \
    bit 7 - Video disable (0 = enabled)
*/
WRITE8_HANDLER ( p2000t_port_303f_w )
{
	p2000t_state *state = space->machine->driver_data<p2000t_state>();
	state->port_303f = data;
}

/*
    Beeper 0x5x

    bit 0 - Beeper
    bit 1 - Unused
    bit 2 - Unused
    bit 3 - Unused
    bit 4 - Unused
    bit 5 - Unused
    bit 6 - Unused
    bit 7 - Unused
*/
WRITE8_HANDLER ( p2000t_port_505f_w )
{
	device_t *speaker = space->machine->device("speaker");
	speaker_level_w(speaker, BIT(data,0));
}

/*
    DISAS 0x7x (P2000M only)

    bit 0 - Unused
    bit 1 - DISAS enable
    bit 2 - Unused
    bit 3 - Unused
    bit 4 - Unused
    bit 5 - Unused
    bit 6 - Unused
    bit 7 - Unused

    When the DISAS is active, the CPU has the highest priority and
    video refresh is disabled when the CPU accesses video memory

*/
WRITE8_HANDLER ( p2000t_port_707f_w )
{
	p2000t_state *state = space->machine->driver_data<p2000t_state>();
	state->port_707f = data;
}

WRITE8_HANDLER ( p2000t_port_888b_w ) {}
WRITE8_HANDLER ( p2000t_port_8c90_w ) {}
WRITE8_HANDLER ( p2000t_port_9494_w ) {}

