/***************************************************************************

    Ausbauf?higer Mikrocomputer mit dem U 880
    electronica Band 227/228

****************************************************************************/

/*

    TODO:

    - keyboard repeat is too fast
    - tone generator
    - cassette (Z80SIO)
    - eprom programmer
    - power off

*/

#include "emu.h"
#include "includes/huebler.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "devices/cassette.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/z80ctc.h"
#include "devices/messram.h"

/* Keyboard */

static TIMER_DEVICE_CALLBACK( keyboard_tick )
{
	amu880_state *state = timer.machine->driver_data<amu880_state>();

	state->key_y++;

	if (state->key_y == 16)
	{
		state->key_y = 0;
	}
}

/* Read/Write Handlers */

static READ8_HANDLER( keyboard_r )
{
	/*

        A0..A3  Y
        A4      !(X0&X3)
        A5      !(X2&X3)
        A6      shift
        A7      ctrl
        A8      key pressed
        A9      AB0

    */

	amu880_state *state = space->machine->driver_data<amu880_state>();

	static const char *const keynames[] = { "Y0", "Y1", "Y2", "Y3", "Y4", "Y5", "Y6", "Y7", "Y8", "Y9", "Y10", "Y11", "Y12", "Y13", "Y14", "Y15" };

	UINT8 data = input_port_read(space->machine, keynames[state->key_y]);
	UINT8 special = input_port_read(space->machine, "SPECIAL");
	UINT16 address;

	int ctrl = BIT(special, 0);
	int shift = BIT(special, 2) & BIT(special, 1);
	int a8 = (data & 0x0f) == 0x0f;
	int ab0 = BIT(offset, 0);

	if (!a8)
	{
		int a4 = BIT(data, 1) & BIT(data, 3);
		int a5 = BIT(data, 2) & BIT(data, 3);

		state->keylatch = (!a5 << 5) | (!a4 << 4) | state->key_y;
	}

	address = (ab0 << 9) | (a8 << 8) | (ctrl << 7) | (shift << 6) | state->keylatch;

	return state->keyboard_rom[address];
}

/* Memory Maps */

static ADDRESS_MAP_START( amu880_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xe7ff) AM_RAM
	AM_RANGE(0xe800, 0xefff) AM_RAM AM_BASE_MEMBER(amu880_state, video_ram)
	AM_RANGE(0xf000, 0xfbff) AM_ROM
	AM_RANGE(0xfc00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( amu880_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//  AM_RANGE(0x00, 0x00) AM_MIRROR(0x03) AM_WRITE(power_off_w)
//  AM_RANGE(0x04, 0x04) AM_MIRROR(0x02) AM_WRITE(tone_off_w)
//  AM_RANGE(0x05, 0x05) AM_MIRROR(0x02) AM_WRITE(tone_on_w)
	AM_RANGE(0x08, 0x09) AM_MIRROR(0x02) AM_READ(keyboard_r)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE(Z80PIO2_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE(Z80PIO1_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x14, 0x17) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0x18, 0x1b) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_ba_cd_r, z80sio_ba_cd_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( amu880 )
	PORT_START("Y0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('e')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('h')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("Y1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('r')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('j')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CLR") PORT_CODE(KEYCODE_END)

	PORT_START("Y2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('t')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('k')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START("Y3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('l')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START("Y4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('u')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("Y5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('i')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("Y6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('o')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // <-|

	PORT_START("Y7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('p')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('z')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)

	PORT_START("Y8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('x')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("Y9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('c')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("Y10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('@') PORT_CHAR('\\')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('v')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("Y11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('a')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('b')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("Y12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('_') PORT_CHAR('|')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('s')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('n')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("Y13")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('d')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('m')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("Y14")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('q')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('f')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("Y15")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('w')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('g')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
INPUT_PORTS_END

/* Video */

static VIDEO_START( amu880 )
{
	amu880_state *state = machine->driver_data<amu880_state>();

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");
}

static VIDEO_UPDATE( amu880 )
{
	amu880_state *state = screen->machine->driver_data<amu880_state>();

	int y, sx, x, line;

	for (y = 0; y < 240; y++)
	{
		line = y % 10;

		for (sx = 0; sx < 64; sx++)
		{
			UINT16 videoram_addr = ((y / 10) * 64) + sx;
			UINT8 videoram_data = state->video_ram[videoram_addr & 0x7ff];

			UINT16 charrom_addr = ((videoram_data & 0x7f) << 3) | line;
			UINT8 data = state->char_rom[charrom_addr & 0x3ff];

			for (x = 0; x < 6; x++)
			{
				int color = ((line > 7) ? 0 : BIT(data, 7)) ^ BIT(videoram_data, 7);

				*BITMAP_ADDR16(bitmap, y, (sx * 6) + x) = color;

				data <<= 1;
			}
		}
	}

    return 0;
}

/* Z80-CTC Interface */

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
	/* cassette transmit/receive clock */
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              	/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),	/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),	/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)	/* ZC/TO2 callback */
};

/* Z80-PIO Interface */

static Z80PIO_INTERFACE( pio1_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

static Z80PIO_INTERFACE( pio2_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80-SIO Interface */

static void z80daisy_interrupt(running_device *device, int state)
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, state);
}

static const z80sio_interface sio_intf =
{
	z80daisy_interrupt,	/* interrupt handler */
	NULL,				/* DTR changed handler */
	NULL,				/* RTS changed handler */
	NULL,				/* BREAK changed handler */
	NULL,				/* transmit handler */
	NULL				/* receive handler */
};

/* Z80 Daisy Chain */

static const z80_daisy_config amu880_daisy_chain[] =
{
	{ Z80CTC_TAG },
	{ Z80SIO_TAG },
	{ Z80PIO1_TAG },
	{ Z80PIO2_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( amu880 )
{
	amu880_state *state = machine->driver_data<amu880_state>();

	/* find devices */
	state->cassette = machine->device(CASSETTE_TAG);

	/* find memory regions */
	state->keyboard_rom = memory_region(machine, "keyboard");

	/* register for state saving */
	state_save_register_global(machine, state->key_y);
	state_save_register_global(machine, state->keylatch);
}

/* Machine Driver */

static const cassette_config amu880_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

/* F4 Character Displayer */
static const gfx_layout amu880_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( amu880 )
	GFXDECODE_ENTRY( "chargen", 0x0000, amu880_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_DRIVER_START( amu880 )
	MDRV_DRIVER_DATA(amu880_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_10MHz/4) /* U880D */
	MDRV_CPU_PROGRAM_MAP(amu880_mem)
	MDRV_CPU_IO_MAP(amu880_io)
	MDRV_CPU_CONFIG(amu880_daisy_chain)

	MDRV_MACHINE_START(amu880)

	MDRV_TIMER_ADD_PERIODIC("keyboard", keyboard_tick, HZ(1500))

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(9000000, 576, 0*6, 64*6, 320, 0*10, 24*10)

	MDRV_GFXDECODE(amu880)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(amu880)
	MDRV_VIDEO_UPDATE(amu880)

	/* devices */
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_10MHz/4, ctc_intf)
	MDRV_Z80PIO_ADD(Z80PIO1_TAG, XTAL_10MHz/4, pio1_intf)
	MDRV_Z80PIO_ADD(Z80PIO2_TAG, XTAL_10MHz/4, pio2_intf)
	MDRV_Z80SIO_ADD(Z80SIO_TAG, XTAL_10MHz/4, sio_intf)

	MDRV_CASSETTE_ADD(CASSETTE_TAG, amu880_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( amu880 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS( "v21" )
	ROM_SYSTEM_BIOS( 0, "v21", "H.MON v2.1" )
	ROMX_LOAD( "mon21a.bin", 0xf000, 0x0400, NO_DUMP, ROM_BIOS(1) )
	ROMX_LOAD( "mon21b.bin", 0xf400, 0x0400, NO_DUMP, ROM_BIOS(1) )
	ROMX_LOAD( "mon21c.bin", 0xf800, 0x0400, NO_DUMP, ROM_BIOS(1) )
	ROMX_LOAD( "mon21.bin", 0xf000, 0x0bdf, BAD_DUMP CRC(ba905563) SHA1(1fa0aeab5428731756bdfa74efa3c664898bf083), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v30", "H.MON v3.0" )
	ROMX_LOAD( "mon30.bin", 0xf000, 0x1000, CRC(033f8112) SHA1(0c6ae7b9d310dec093652db6e8ae84f8ebfdcd29), ROM_BIOS(2) )

	ROM_REGION( 0x4800, "hbasic", 0 )
	ROM_LOAD( "mon30p_hbasic33p.bin", 0x0000, 0x4800, CRC(c927e7be) SHA1(2d1f3ff4d882c40438a1281872c6037b2f07fdf2) )

	ROM_REGION( 0x400, "chargen", 0 )
	ROM_LOAD( "hemcfont.bin", 0x0000, 0x0400, CRC(1074d103) SHA1(e558279cff5744acef4eccf30759a9508b7f8750) )

	ROM_REGION( 0x400, "keyboard", 0 )
	ROM_LOAD( "keyboard.bin", 0x0000, 0x0400, BAD_DUMP CRC(daa06361) SHA1(b0299bd8d1686e05dbeeaed54f6c41ae543be20c) ) // typed in from manual
ROM_END

/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                     FULLNAME                                        FLAGS */
COMP( 1983, amu880,	0,		0,		amu880,	amu880,	0,		"Militaerverlag der DDR",	"Ausbaufaehiger Mikrocomputer mit dem U 880",	GAME_NO_SOUND )