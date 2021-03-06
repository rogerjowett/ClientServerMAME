/***********************************************************************************************************
Barcrest MPU4 Extension driver by J.Wallace, and Anonymous.

--- Board Setup ---
For the Barcrest MPU4 Video system, the GAME CARD (cartridge) contains the MPU4 video bios in the usual ROM
space (occupying 16k), an interface card to connect an additional Video board, and a 6850 serial IO to
communicate with said board.

The VIDEO BOARD is driven by a 10mhz 68000 processor, and contains a 6840PTM, 6850 serial IO
(the other end of the communications), an SAA1099 for stereo sound and SCN2674 gfx chip.

The VIDEO CARTRIDGE plugs into the video board, and contains the program ROMs for the video based game.
Like the MPU4 game card, in some cases an extra OKI sound chip is added to the video board's game card,
as well as extra RAM.
There is a protection chip similar to and replacing the MPU4 Characteriser.

No video card schematics ever left the PCB factory, but some decent scans of the board have been made,
now also available for review.

Additional: 68k HALT line is tied to the reset circuit of the MPU4.

--- Preliminary MPU4 Video Interface Card Memorymap  ---

   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+--------------------------------------------------------------------------
 0000-07FF |R/W| D D D D D D D D | 2k RAM
-----------+---+-----------------+--------------------------------------------------------------------------
 0800      |R/W|                 | ACIA 6850
-----------+---+-----------------+--------------------------------------------------------------------------
 0900-     |R/W| D D D D D D D D | MC6840 PTM IC2


  Clock1 <--------------------------------------
     |                                          |
     V                                          |
  Output1 ---> Clock2                           |
                                                |
               Output2 --+-> Clock3             |
                         |                      |
                         |   Output3 ---> 'to audio amp' ??
                         |
                         +--------> CA1 IC3 (

IRQ line connected to CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 0A00-0A03 |R/W| D D D D D D D D | PIA6821 IC3 port A Lamp Drives 1,2,3,4,6,7,8,9 (sic)(IC14)
           |   |                 |
           |   |                 |          CA1 <= output2 from PTM6840 (IC2)
           |   |                 |          CA2 => alpha data
           |   |                 |
           |   |                 |          port B Lamp Drives 10,11,12,13,14,15,16,17 (sic)(IC13)
           |   |                 |
           |   |                 |          CB2 => alpha reset (clock on Dutch systems)
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0B00-0B03 |R/W| D D D D D D D D | PIA6821 IC4 port A = data for 7seg leds (pins 10 - 17, via IC32)
           |   |                 |
           |   |                 |             CA1 INPUT, 50 Hz input (used to generate IRQ)
           |   |                 |             CA2 OUTPUT, connected to pin2 74LS138 CE for multiplexer
           |   |                 |                        (B on LED strobe multiplexer)
           |   |                 |             IRQA connected to IRQ of CPU
           |   |                 |             port B
           |   |                 |                    PB7 = INPUT, serial port Receive data (Rx)
           |   |                 |                    PB6 = INPUT, reel A sensor
           |   |                 |                    PB5 = INPUT, reel B sensor
           |   |                 |                    PB4 = INPUT, reel C sensor
           |   |                 |                    PB3 = INPUT, reel D sensor
           |   |                 |                    PB2 = INPUT, Connected to CA1 (50Hz signal)
           |   |                 |                    PB1 = INPUT, undercurrent sense
           |   |                 |                    PB0 = INPUT, overcurrent  sense
           |   |                 |
           |   |                 |             CB1 INPUT,  used to generate IRQ on edge of serial input line
           |   |                 |             CB2 OUTPUT, enable signal for reel optics
           |   |                 |             IRQB connected to IRQ of CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0C00-0C03 |R/W| D D D D D D D D | PIA6821 IC5 port A
           |   |                 |
           |   |                 |                    PA0-PA7, INPUT AUX1 connector
           |   |                 |
           |   |                 |             CA2  OUTPUT, serial port Transmit line
           |   |                 |             CA1  not connected
           |   |                 |             IRQA connected to IRQ of CPU
           |   |                 |
           |   |                 |             port B
           |   |                 |
           |   |                 |                    PB0-PB7 INPUT, AUX2 connector
           |   |                 |
           |   |                 |             CB1  INPUT,  connected to PB7 (Aux2 connector pin 4)
           |   |                 |
           |   |                 |             CB2  OUTPUT, AY8913 chip select line
           |   |                 |             IRQB connected to IRQ of CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0D00-0D03 |R/W| D D D D D D D D | PIA6821 IC6
           |   |                 |
           |   |                 |  port A
           |   |                 |
           |   |                 |        PA0 - PA7 (INPUT/OUTPUT) data port AY8913 sound chip
           |   |                 |
           |   |                 |        CA1 INPUT,  not connected
           |   |                 |        CA2 OUTPUT, BC1 pin AY8913 sound chip
           |   |                 |        IRQA , connected to IRQ CPU
           |   |                 |
           |   |                 |  port B
           |   |                 |
           |   |                 |        PB0-PB3 OUTPUT, reel A
           |   |                 |        PB4-PB7 OUTPUT, reel B
           |   |                 |
           |   |                 |        CB1 INPUT,  not connected
           |   |                 |        CB2 OUTPUT, B01R pin AY8913 sound chip
           |   |                 |        IRQB , connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0E00-0E03 |R/W| D D D D D D D D | PIA6821 IC7
           |   |                 |
           |   |                 |  port A
           |   |                 |
           |   |                 |        PA0-PA3 OUTPUT, reel C
           |   |                 |        PA4-PA7 OUTPUT, reel D
           |   |                 |        CA1     INPUT,  not connected
           |   |                 |        CA2     OUTPUT, A on LED strobe multiplexer
           |   |                 |        IRQA , connected to IRQ CPU
           |   |                 |
           |   |                 |  port B
           |   |                 |
           |   |                 |        PB0-PB6 OUTPUT mech meter 1-7 or reel E + F
           |   |                 |        PB7     Voltage drop sensor
           |   |                 |        CB1     INPUT, not connected
           |   |                 |        CB2     OUTPUT,mech meter 8
           |   |                 |        IRQB , connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0F00-0F03 |R/W| D D D D D D D D | PIA6821 IC8
           |   |                 |
           |   |                 | port A
           |   |                 |
           |   |                 |        PA0-PA7 INPUT  multiplexed inputs data
           |   |                 |
           |   |                 |        CA1     INPUT, not connected
           |   |                 |        CA2    OUTPUT, C on LED strobe multiplexer
           |   |                 |        IRQA           connected to IRQ CPU
           |   |                 |
           |   |                 | port B
           |   |                 |
           |   |                 |        PB0-PB7 OUTPUT  triacs outputs connector PL6
           |   |                 |        used for slides / hoppers
           |   |                 |
           |   |                 |        CB1     INPUT, not connected
           |   |                 |        CB2    OUTPUT, pin1 alpha display PL7 (clock signal)
           |   |                 |        IRQB           connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 4000-40FF |R/W| D D D D D D D D | RAM ?
-----------+---+-----------------+--------------------------------------------------------------------------
 BE00-BFFF |R/W| D D D D D D D D | RAM
-----------+---+-----------------+--------------------------------------------------------------------------
 C000-FFFF | R | D D D D D D D D | ROM
-----------+---+-----------------+--------------------------------------------------------------------------
Everything here is preliminary...  the boards are quite fussy with regards their self tests
and the timing may have to be perfect for them to function correctly.  (as the comms are
timer driven, the video is capable of various raster effects etc.)

TODO:
      - Correctly implement characteriser protection for each game.
      - Mating Game animation and screen still slower than it should be, AVDC timing/IRQs?
      - Get the BwB games running
        * They have a slightly different 68k memory map. The 6850 is at e00000 and the 6840 is at e01000
        They appear to hang on the handshake with the MPU4 board
      - Find out what causes the games to hang/reset in service mode
        Probably down to AVDC interrupt timing, there seem to be a number of race conditions re: masks
        that need sorting out with proper blank handling, etc.
      - Deal 'Em lockouts vary on certain cabinets (normally connected to AUX2, but not there?)
      - Deal 'Em has bad tiles (apostrophe, logo, bottom corner), black should actually be transparent
        to give black on green.
***********************************************************************************************************/

#include "cpu/m68000/m68000.h"
#include "machine/6850acia.h"
#include "sound/saa1099.h"
#include "video/mc6845.h"
#include "video/resnet.h"
#include "machine/nvram.h"

#ifdef MAME_DEBUG
#define MPU4VIDVERBOSE 1
#else
#define MPU4VIDVERBOSE 0
#endif

#define LOGSTUFF(x) do { if (MPU4VIDVERBOSE) logerror x; } while (0)
#define LOG2674(x) do { if (MPU4VIDVERBOSE) logerror x; } while (0)


#define VIDEO_MASTER_CLOCK			XTAL_10MHz


static READ16_HANDLER( characteriser16_r );
static WRITE16_HANDLER( characteriser16_w );

static READ16_HANDLER( bwb_characteriser16_r );
static WRITE16_HANDLER( bwb_characteriser16_w );

/*************************************
 *
 *  Interrupt system
 *
 *************************************/

/* The interrupt system consists of a 74148 priority encoder
   with the following interrupt priorites.  A lower number
   indicates a lower priority:

    7 - Game Card
    6 - Game Card
    5 - Game Card
    4 - Game Card
    3 - 2674 AVDC
    2 - 6850 ACIA
    1 - 6840 PTM
    0 - Unused
*/


static void update_mpu68_interrupts(running_machine &machine)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	cputag_set_input_line(machine, "video", 1, state->m_m6840_irq_state ? ASSERT_LINE : CLEAR_LINE);
	cputag_set_input_line(machine, "video", 2, state->m_m6850_irq_state ? CLEAR_LINE : ASSERT_LINE);
	cputag_set_input_line(machine, "video", 3, state->m_scn2674_irq_state ? ASSERT_LINE : CLEAR_LINE);
	//logerror("%x,%x,%x\n",state->m_m6840_irq_state,state->m_m6850_irq_state,state->m_scn2674_irq_state);
}

/* Communications with 6809 board */

static READ_LINE_DEVICE_HANDLER( m6809_acia_rx_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	return state->m_m68k_m6809_line;
}

static WRITE_LINE_DEVICE_HANDLER( m6809_acia_tx_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m6809_m68k_line = state;
}

static READ_LINE_DEVICE_HANDLER( m6809_acia_cts_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	return state->m_m6809_acia_cts;
}

static WRITE_LINE_DEVICE_HANDLER( m6809_acia_rts_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m6809_acia_rts = state;
}

static READ_LINE_DEVICE_HANDLER( m6809_acia_dcd_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	return state->m_m6809_acia_dcd;
}

static WRITE_LINE_DEVICE_HANDLER( m6809_acia_irq )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m68k_acia_cts = !state;
	cputag_set_input_line(device->machine(), "maincpu", M6809_IRQ_LINE, state ? CLEAR_LINE : ASSERT_LINE);
}

static ACIA6850_INTERFACE( m6809_acia_if )
{
	0,
	0,
	DEVCB_LINE(m6809_acia_rx_r),
	DEVCB_LINE(m6809_acia_tx_w),
	DEVCB_LINE(m6809_acia_cts_r),
	DEVCB_LINE(m6809_acia_rts_w),
	DEVCB_LINE(m6809_acia_dcd_r),
	DEVCB_LINE(m6809_acia_irq)
};

static READ_LINE_DEVICE_HANDLER( m68k_acia_rx_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	return state->m_m6809_m68k_line;
}

static WRITE_LINE_DEVICE_HANDLER( m68k_acia_tx_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m68k_m6809_line = state;
}

static READ_LINE_DEVICE_HANDLER( m68k_acia_cts_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	return state->m_m68k_acia_cts;
}

static WRITE_LINE_DEVICE_HANDLER( m68k_acia_rts_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m6809_acia_dcd = state;
}

static READ_LINE_DEVICE_HANDLER( m68k_acia_dcd_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	return state->m_m6809_acia_rts;
}

static WRITE_LINE_DEVICE_HANDLER( m68k_acia_irq )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m6809_acia_cts = !state;
	drvstate->m_m6850_irq_state = state;
	update_mpu68_interrupts(device->machine());
}

static ACIA6850_INTERFACE( m68k_acia_if )
{
	0,
	0,
	DEVCB_LINE(m68k_acia_rx_r),
	DEVCB_LINE(m68k_acia_tx_w),
	DEVCB_LINE(m68k_acia_cts_r),
	DEVCB_LINE(m68k_acia_rts_w),
	DEVCB_LINE(m68k_acia_dcd_r),
	DEVCB_LINE(m68k_acia_irq)
};


static WRITE_LINE_DEVICE_HANDLER( cpu1_ptm_irq )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_m6840_irq_state = state;
	update_mpu68_interrupts(device->machine());
}


static WRITE8_DEVICE_HANDLER( vid_o1_callback )
{
	downcast<ptm6840_device *>(device)->set_c2(data); /* this output is the clock for timer2 */

	if (data)
	{
		acia6850_device *acia_0 = device->machine().device<acia6850_device>("acia6850_0");
		acia6850_device *acia_1 = device->machine().device<acia6850_device>("acia6850_1");
		acia_0->tx_clock_in();
		acia_0->rx_clock_in();
		acia_1->tx_clock_in();
		acia_1->rx_clock_in();
	}
}


static WRITE8_DEVICE_HANDLER( vid_o2_callback )
{
	downcast<ptm6840_device *>(device)->set_c3(data); /* this output is the clock for timer3 */
}


static WRITE8_DEVICE_HANDLER( vid_o3_callback )
{
	downcast<ptm6840_device *>(device)->set_c1(data); /* this output is the clock for timer1 */
}


static const ptm6840_interface ptm_vid_intf =
{
	VIDEO_MASTER_CLOCK / 10, /* 68k E clock */
	{ 0, 0, 0 },
	{ DEVCB_HANDLER(vid_o1_callback),
	  DEVCB_HANDLER(vid_o2_callback),
	  DEVCB_HANDLER(vid_o3_callback) },
	DEVCB_LINE(cpu1_ptm_irq)
};


/*************************************
 *
 *  SCN2674 AVDC
 *
 *************************************/

/* the chip is actually more complex than this.. character aren't limited to 8 rows high... but I
   don't *think* the MPU4 stuff needs otherwise.. yet .. */

static const gfx_layout mpu4_vid_char_8x8_layout =
{
	8,8,
	0x1000, /* 0x1000 tiles (128k of GFX RAM, 0x20 bytes per tile) */
	4,
	{ 8,0,24,16 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	8*32
};


/* double height */
static const gfx_layout mpu4_vid_char_8x16_layout =
{
	8,16,
	0x1000, /* 0x1000 tiles (128k of GFX RAM, 0x20 bytes per tile) */
	4,
	{ 8,0,24,16 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 0*32, 1*32, 1*32, 2*32, 2*32, 3*32, 3*32, 4*32, 4*32, 5*32, 5*32, 6*32, 6*32, 7*32, 7*32},
	8*32
};


/* double width */
static const gfx_layout mpu4_vid_char_16x8_layout =
{
	16,8,
	0x1000, /* 0x1000 tiles (128k of GFX RAM, 0x20 bytes per tile) */
	4,
	{ 8,0,24,16 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	8*32
};


/* double height & width */
static const gfx_layout mpu4_vid_char_16x16_layout =
{
	16,16,
	0x1000,  /* 0x1000 tiles (128k of GFX RAM, 0x20 bytes per tile) */
	4,
	{ 8,0,24,16 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	{ 0*32, 0*32, 1*32, 1*32, 2*32, 2*32, 3*32, 3*32, 4*32, 4*32, 5*32, 5*32, 6*32, 6*32, 7*32, 7*32},
	8*32
};




static SCREEN_UPDATE(mpu4_vid)
{
	mpu4_state *state = screen->machine().driver_data<mpu4_state>();
	int x, y/*, count = 0*/;

	bitmap_fill(bitmap,cliprect,0);

	/* this is in main ram.. i think it must transfer it out of here??? */
	/* count = 0x0018b6/2; - crmaze count = 0x004950/2; - turnover */
	/* we're in row table mode...thats why */
	for(y = 0; y < state->m_IR4_scn2674_rows_per_screen; y++)
	{
		int screen2_base = (state->m_scn2674_screen2_h << 8) | state->m_scn2674_screen2_l;
		UINT16 rowbase = (state->m_vid_mainram[1+screen2_base+(y*2)]<<8)|state->m_vid_mainram[screen2_base+(y*2)];
		int dbl_size=0;
		int gfxregion = 0;

		if (state->m_IR0_scn2674_double_ht_wd)
		{
			dbl_size = (rowbase & 0xc000)>>14;  /* ONLY if double size is enabled.. otherwise it can address more chars given more RAM */
		}

		if (dbl_size&2)
		{
			gfxregion = 1;
		}
		for(x = 0; x < state->m_IR5_scn2674_character_per_row; x++)
		{
			UINT16 tiledat;
			UINT16 attr;

			tiledat = state->m_vid_mainram[(rowbase+x)&0x7fff];
			attr = tiledat >>12;

			if (attr)
				drawgfx_opaque(bitmap,cliprect,screen->machine().gfx[gfxregion],tiledat,0,0,0,(x*8)+(4*8),(y*8)+4);

		}
		if (dbl_size&2)
		{
			//state->m_linecounter -=8;//Since every row is 8 scanlines, a double row must halve this, so we take 8 lines off the counter
			y++;/* skip a row? */
		}

	}

	return 0;
}


static READ16_HANDLER( mpu4_vid_vidram_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	return state->m_vid_vidram[offset];
}


static WRITE16_HANDLER( mpu4_vid_vidram_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	COMBINE_DATA(&state->m_vid_vidram[offset]);
	offset <<= 1;
	gfx_element_mark_dirty(space->machine().gfx[state->m_gfx_index+0], offset/0x20);
	gfx_element_mark_dirty(space->machine().gfx[state->m_gfx_index+1], offset/0x20);
	gfx_element_mark_dirty(space->machine().gfx[state->m_gfx_index+2], offset/0x20);
	gfx_element_mark_dirty(space->machine().gfx[state->m_gfx_index+3], offset/0x20);
}


/*
SCN2674 - Advanced Video Display Controller (AVDC)  (Video Chip)

15 Initialization Registers (8-bit each)
*/


static void scn2674_write_init_regs(mpu4_state *state, UINT8 data)
{
	LOGSTUFF(("scn2674_write_init_regs %02x %02x\n",state->m_scn2674_IR_pointer,data));

//  state->m_scn2674_IR[state->m_scn2674_IR_pointer]=data;


	switch ( state->m_scn2674_IR_pointer) /* display some debug info, set mame specific variables */
	{
		case 0:
			state->m_IR0_scn2674_double_ht_wd = (data & 0x80)>>7;
			state->m_IR0_scn2674_scanline_per_char_row = ((data & 0x78)>>3) + 1;
			state->m_IR0_scn2674_sync_select = (data&0x04)>>2;
			state->m_IR0_scn2674_buffer_mode_select = (data&0x03);

			LOGSTUFF(("IR0 - Double Ht Wd %02x\n",state->m_IR0_scn2674_double_ht_wd));//affects IR14 as well
			LOGSTUFF(("IR0 - Scanlines per Character Row %02x\n",state->m_IR0_scn2674_scanline_per_char_row));//value+1 = scanlines

			if (state->m_IR0_scn2674_scanline_per_char_row != 8)
			{
				popmessage("Row size change, contact MAMEDEV");
			}
			LOGSTUFF(("IR0 - Sync Select %02x\n",state->m_IR0_scn2674_sync_select));//1 = csync
			LOGSTUFF(("IR0 - Buffer Mode Select %02x\n",state->m_IR0_scn2674_buffer_mode_select)); //0 independent 1 transparent 2 shared 3 row
			break;

		case 1:
			state->m_IR1_scn2674_interlace_enable = (data&0x80)>>7;
			state->m_IR1_scn2674_equalizing_constant = (data&0x7f)+1;

			LOGSTUFF(("IR1 - Interlace Enable %02x\n",state->m_IR1_scn2674_interlace_enable));
			LOGSTUFF(("IR1 - Equalizing Constant %02i CCLKs\n",state->m_IR1_scn2674_equalizing_constant));
			break;

		case 2:
			state->m_IR2_scn2674_row_table = (data&0x80)>>7;
			state->m_IR2_scn2674_horz_sync_width = (((data&0x78)>>3)*2) + 2;
			state->m_IR2_scn2674_horz_back_porch = ((data&0x07)*4) - 1;

			LOGSTUFF(("IR2 - Row Table %02x\n",state->m_IR2_scn2674_row_table));
			LOGSTUFF(("IR2 - Horizontal Sync Width %02i CCLKs\n",state->m_IR2_scn2674_horz_sync_width));
			LOGSTUFF(("IR2 - Horizontal Back Porch %02i CCLKs\n",state->m_IR2_scn2674_horz_back_porch));
			break;

		case 3:
			state->m_IR3_scn2674_vert_front_porch =  (((data&0xe0)>>5) * 4)+4 ;//returning actual value
			state->m_IR3_scn2674_vert_back_porch = ((data&0x1f) * 2) + 4;

			LOGSTUFF(("IR3 - Vertical Front Porch %02i Lines\n",state->m_IR3_scn2674_vert_front_porch));
			LOGSTUFF(("IR3 - Vertical Back Porch %02i Lines\n",state->m_IR3_scn2674_vert_back_porch));
			break;

		case 4:
			state->m_IR4_scn2674_rows_per_screen = (data&0x7f) + 1;
			state->m_IR4_scn2674_character_blink_rate_divisor = ((data & 0x80)>>7 ? 128:64);

			LOGSTUFF(("IR4 - Rows Per Screen %02i\n",state->m_IR4_scn2674_rows_per_screen));
			LOGSTUFF(("IR4 - Character Blink Rate = 1/%02i\n",state->m_IR4_scn2674_character_blink_rate_divisor));
			break;

		case 5:
		   /* IR5 - Active Characters Per Row
             cccc cccc
             c = Characters Per Row */
			state->m_IR5_scn2674_character_per_row = data + 1;
			LOGSTUFF(("IR5 - Active Characters Per Row %02i\n",state->m_IR5_scn2674_character_per_row));
			break;

		case 6:
			state->m_IR6_scn2674_cursor_last_scanline = (data & 0x0f);
			state->m_IR6_scn2674_cursor_first_scanline = (data & 0xf0)>>4;
			LOGSTUFF(("IR6 - First Line of Cursor %02x\n",state->m_IR6_scn2674_cursor_first_scanline));
			LOGSTUFF(("IR6 - Last Line of Cursor %02x\n",state->m_IR6_scn2674_cursor_last_scanline));
			break;

		case 7:
			state->m_IR7_scn2674_cursor_underline_position = (data & 0x0f);
			state->m_IR7_scn2674_cursor_rate_divisor = ((data & 0x10)>>4 ? 64:32);
			state->m_IR7_scn2674_cursor_blink = (data & 0x20)>>5;

			state->m_IR7_scn2674_vsync_width = vsync_table[(data & 0xC0)>>6];

			LOGSTUFF(("IR7 - Underline Position %02x\n",state->m_IR7_scn2674_cursor_underline_position));
			LOGSTUFF(("IR7 - Cursor rate 1/%02i\n",state->m_IR7_scn2674_cursor_rate_divisor));
			LOGSTUFF(("IR7 - Cursor blink %02x\n",state->m_IR7_scn2674_cursor_blink));
			LOGSTUFF(("IR7 - Vsync Width  %02i Lines\n",state->m_IR7_scn2674_vsync_width));
			break;

		case 8:
			state->m_IR8_scn2674_display_buffer_first_address_LSB = data;
			LOGSTUFF(("IR8 - Display Buffer First Address LSB %02x\n",state->m_IR8_scn2674_display_buffer_first_address_LSB));
			break;

		case 9:
			state->m_IR9_scn2674_display_buffer_first_address_MSB = data & 0x0f;
			state->m_IR9_scn2674_display_buffer_last_address = (data & 0xf0)>>4;
			LOGSTUFF(("IR9 - Display Buffer First Address MSB %02x\n",state->m_IR9_scn2674_display_buffer_first_address_MSB));
			LOGSTUFF(("IR9 - Display Buffer Last Address %02x\n",state->m_IR9_scn2674_display_buffer_last_address));
			break;

		case 10:
			state->m_IR10_scn2674_display_pointer_address_lower = data;
			LOGSTUFF(("IR10 - Display Pointer Address Lower %02x\n",state->m_IR10_scn2674_display_pointer_address_lower));
			break;

		case 11:
			state->m_IR11_scn2674_display_pointer_address_upper= data&0x3f;
			state->m_IR11_scn2674_reset_scanline_counter_on_scrollup= (data&0x40 >> 6);
			state->m_IR11_scn2674_reset_scanline_counter_on_scrolldown= (data&0x80 >> 7);

			LOGSTUFF(("IR11 - Display Pointer Address Lower %02x\n",state->m_IR11_scn2674_display_pointer_address_upper));
			LOGSTUFF(("IR11 - Reset Scanline Counter on Scroll Up %02x\n",state->m_IR11_scn2674_reset_scanline_counter_on_scrollup));
			LOGSTUFF(("IR11 - Reset Scanline Counter on Scroll Down %02x\n",state->m_IR11_scn2674_reset_scanline_counter_on_scrolldown));
			break;

		case 12:
			state->m_IR12_scn2674_scroll_start = (data & 0x80)>>7;
			state->m_IR12_scn2674_split_register_1 = (data & 0x7f);
			LOGSTUFF(("IR12 - Scroll Start %02x\n",state->m_IR12_scn2674_scroll_start));
			LOGSTUFF(("IR12 - Split Register 1 %02x\n",state->m_IR12_scn2674_split_register_1));
			break;

		case 13:
			state->m_IR13_scn2674_scroll_end = (data & 0x80)>>7;
			state->m_IR13_scn2674_split_register_2 = (data & 0x7f);
			LOGSTUFF(("IR13 - Scroll End %02x\n",state->m_IR13_scn2674_scroll_end));
			LOGSTUFF(("IR13 - Split Register 2 %02x\n",state->m_IR13_scn2674_split_register_2));
			break;

		case 14:
			state->m_IR14_scn2674_scroll_lines = (data & 0x0f);
			if (!state->m_IR0_scn2674_double_ht_wd)
			{
				state->m_IR14_scn2674_double_2 = (data & 0x30)>>4;
				LOGSTUFF(("IR14 - Double 2 %02x\n",state->m_IR14_scn2674_double_2));
			}
			//0 normal, 1, double width, 2, double width and double tops 3, double width and double bottoms
			//1 affects SSR1, 2 affects SSR2
			//If Double Height enabled in IR0, Screen start 1 upper (bits 7 and 6)replace Double 1, and Double 2 is unused
			state->m_IR14_scn2674_double_1 = (data & 0xc0)>>6;
			LOGSTUFF(("IR14 - Double 1 %02x\n",state->m_IR14_scn2674_double_1));

			LOGSTUFF(("IR14 - Scroll Lines %02i\n",state->m_IR14_scn2674_scroll_lines));
			break;

		case 15: /* not valid! */
			break;

	}

	state->m_scn2674_horz_front_porch = 2*(state->m_IR1_scn2674_equalizing_constant) + 3*(state->m_IR2_scn2674_horz_sync_width)-(state->m_IR5_scn2674_character_per_row) - state->m_IR2_scn2674_horz_back_porch;
	LOGSTUFF(("IRXX - Horizontal Front Porch %02x CCLKs\n",state->m_scn2674_horz_front_porch));

	state->m_scn2674_IR_pointer++;
	if (state->m_scn2674_IR_pointer>14)state->m_scn2674_IR_pointer=14;
}

static void scn2674_write_command(running_machine &machine, UINT8 data)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	UINT8 operand;
	int i;

	LOGSTUFF(("scn2674_write_command %02x\n",data));

	if (data==0x00)
	{
		/* master reset, configures registers */
		LOGSTUFF(("master reset\n"));
		state->m_scn2674_IR_pointer=0;
		state->m_scn2674_irq_register = 0x00;
		state->m_scn2674_status_register = 0x20;//RDFLG activated
		state->m_linecounter =0;
		state->m_rowcounter =0;
		state->m_scn2674_irq_mask = 0x00;
		state->m_scn2674_gfx_enabled = 0;
		state->m_scn2674_display_enabled = 0;
		state->m_scn2674_cursor_enabled = 0;
		state->m_IR2_scn2674_row_table = 0;
	}

	if ((data&0xf0)==0x10)
	{
		/* set IR pointer */
		operand = data & 0x0f;
		LOGSTUFF(("set IR pointer %02x\n",operand));

		state->m_scn2674_IR_pointer=operand;

	}

	/* ANY COMBINATION OF THESE ARE POSSIBLE */

	if ((data&0xe3)==0x22)
	{
		/* Disable GFX */
		LOGSTUFF(("disable GFX %02x\n",data));
		state->m_scn2674_gfx_enabled = 0;
	}

	if ((data&0xe3)==0x23)
	{
		/* Enable GFX */
		LOGSTUFF(("enable GFX %02x\n",data));
		state->m_scn2674_gfx_enabled = 1;
	}

	if ((data&0xe9)==0x28)
	{
		/* Display off */
		operand = data & 0x04;

		state->m_scn2674_display_enabled = 0;

		if (operand)
			LOGSTUFF(("display OFF - float DADD bus %02x\n",data));
		else
			LOGSTUFF(("display OFF - no float DADD bus %02x\n",data));
	}

	if ((data&0xe9)==0x29)
	{
		/* Display on */
		operand = data & 0x04;

		//state->m_scn2674_display_enabled = 1;

		if (operand)
		{
			state->m_scn2674_display_enabled_field = 1;
			LOGSTUFF(("display ON - next field %02x\n",data));
		}
		else
		{
			state->m_scn2674_display_enabled_scanline = 1;
			LOGSTUFF(("display ON - next scanline %02x\n",data));
		}
	}

	if ((data&0xf1)==0x30)
	{
		/* Cursor Off */
		LOGSTUFF(("cursor off %02x\n",data));
		state->m_scn2674_cursor_enabled = 0;
	}

	if ((data&0xf1)==0x31)
	{
		/* Cursor On */
		LOGSTUFF(("cursor on %02x\n",data));
		state->m_scn2674_cursor_enabled = 1;
	}

	/* END */

	if ((data&0xe0)==0x40)
	{
		/* Reset Interrupt / Status bit */
		operand = data & 0x1f;
		LOGSTUFF(("reset interrupt / status bit %02x\n",operand));

		state->m_scn2674_irq_register &= ~(data & 0x1f);//((data & 0x1f)^0x1f);
		state->m_scn2674_status_register &= ~(data & 0x1f);//((data & 0x1f)^0x1f);

		LOGSTUFF(("IRQ Status after reset\n"));
		LOGSTUFF(("Split 2   IRQ: %d Active\n",(state->m_scn2674_irq_register>>0)&1));
		LOGSTUFF(("Ready     IRQ: %d Active\n",(state->m_scn2674_irq_register>>1)&1));
		LOGSTUFF(("Split 1   IRQ: %d Active\n",(state->m_scn2674_irq_register>>2)&1));
		LOGSTUFF(("Line Zero IRQ: %d Active\n",(state->m_scn2674_irq_register>>3)&1));
		LOGSTUFF(("V-Blank   IRQ: %d Active\n",(state->m_scn2674_irq_register>>4)&1));

		state->m_scn2674_irq_state = 0;

		for (i = 0; i < 5; i++)
		{
			if ((state->m_scn2674_irq_register>>i&1)&(state->m_scn2674_irq_mask>>i&1))
			{
				state->m_scn2674_irq_state = 1;
			}
		}
		update_mpu68_interrupts(machine);

	}
	if ((data&0xe0)==0x80)
	{
		/* Disable Interrupt mask*/
		operand = data & 0x1f;
		state->m_scn2674_irq_mask &= ~(operand);
		LOGSTUFF(("IRQ Mask after disable %x\n",operand));
		LOGSTUFF(("Split 2   IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>0)&1));
		LOGSTUFF(("Ready     IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>1)&1));
		LOGSTUFF(("Split 1   IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>2)&1));
		LOGSTUFF(("Line Zero IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>3)&1));
		LOGSTUFF(("V-Blank   IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>4)&1));

	}

	if ((data&0xe0)==0x60)
	{
		/* Enable Interrupt mask*/
		operand = data & 0x1f;
		state->m_scn2674_irq_mask |= (data & 0x1f);  /* enables .. doesn't disable? */

		LOGSTUFF(("IRQ Mask after enable %x\n",operand));
		LOGSTUFF(("Split 2   IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>0)&1));
		LOGSTUFF(("Ready     IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>1)&1));
		LOGSTUFF(("Split 1   IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>2)&1));
		LOGSTUFF(("Line Zero IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>3)&1));
		LOGSTUFF(("V-Blank   IRQ: %d Unmasked\n",(state->m_scn2674_irq_mask>>4)&1));

	}

	/* Delayed Commands */
	/* These set 0x20 in status register when done */

	if (data == 0xa4)
	{
		/* read at pointer address */
		LOGSTUFF(("DELAYED read at pointer address %02x\n",data));
	}

	if (data == 0xa2)
	{
		/* write at pointer address */
		LOGSTUFF(("DELAYED write at pointer address %02x\n",data));
	}

	if (data == 0xa9)
	{
		/* increase cursor address */
		LOGSTUFF(("DELAYED increase cursor address %02x\n",data));
	}

	if (data == 0xac)
	{
		/* read at cursor address */
		LOGSTUFF(("DELAYED read at cursor address %02x\n",data));
	}

	if (data == 0xaa)
	{
		/* write at cursor address */
		LOGSTUFF(("DELAYED write at cursor address %02x\n",data));
	}

	if (data == 0xad)
	{
		/* read at cursor address + increment */
		LOGSTUFF(("DELAYED read at cursor address+increment %02x\n",data));
	}

	if (data == 0xab)
	{
		/* write at cursor address + increment */
		LOGSTUFF(("DELAYED write at cursor address+increment %02x\n",data));
	}

	if (data == 0xbb)
	{
		/* write from cursor address to pointer address */
		LOGSTUFF(("DELAYED write from cursor address to pointer address %02x\n",data));
	}

	if (data == 0xbd)
	{
		/* read from cursor address to pointer address */
		LOGSTUFF(("DELAYED read from cursor address to pointer address %02x\n",data));
	}
}


static READ16_HANDLER( mpu4_vid_scn2674_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	/*
    Offset:  Purpose
     0       Interrupt Register
     1       Status Register
     2       Screen Start 1 Lower Register
     3       Screen Start 1 Upper Register
     4       Cursor Address Lower Register
     5       Cursor Address Upper Register
     6       Screen Start 2 Lower Register
     7       Screen Start 2 Upper Register
    */

	switch (offset)
	{

		/*  Status / Irq Register

            --RV ZSRs

         6+7 -- = ALWAYS 0
          5  R  = RDFLG (Status Register Only)
          4  V  = Vblank
          3  Z  = Line Zero
          2  S  = Split 1
          1  R  = Ready
          0  s  = Split 2
        */

		case 0:
			LOGSTUFF(("Read Irq Register %02x %06x\n",state->m_scn2674_irq_register,cpu_get_pc(&space->device())));
			return state->m_scn2674_irq_register;

		case 1:
			LOGSTUFF(("Read Status Register %02X %06x\n",state->m_scn2674_status_register,cpu_get_pc(&space->device())));
			return state->m_scn2674_status_register;

		case 2: LOGSTUFF(("Read Screen1_l Register %06x\n",cpu_get_pc(&space->device())));return state->m_scn2674_screen1_l;
		case 3: LOGSTUFF(("Read Screen1_h Register %06x\n",cpu_get_pc(&space->device())));return state->m_scn2674_screen1_h;
		case 4: LOGSTUFF(("Read Cursor_l Register %06x\n",cpu_get_pc(&space->device())));return state->m_scn2674_cursor_l;
		case 5: LOGSTUFF(("Read Cursor_h Register %06x\n",cpu_get_pc(&space->device())));return state->m_scn2674_cursor_h;
		case 6:	LOGSTUFF(("Read Screen2_l Register %06x\n",cpu_get_pc(&space->device())));return state->m_scn2674_screen2_l;
		case 7: LOGSTUFF(("Read Screen2_h Register %06x\n",cpu_get_pc(&space->device())));return state->m_scn2674_screen2_h;
	}

	return 0xffff;
}


static WRITE16_HANDLER( mpu4_vid_scn2674_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	/*
    Offset:  Purpose
     0       Initialization Registers
     1       Command Register
     2       Screen Start 1 Lower Register
     3       Screen Start 1 Upper Register
     4       Cursor Address Lower Register
     5       Cursor Address Upper Register
     6       Screen Start 2 Lower Register
     7       Screen Start 2 Upper Register
    */

	data &=0x00ff; /* it's an 8-bit chip on a 16-bit board, feel the cheapness. */

	switch (offset)
	{
		case 0:
			scn2674_write_init_regs(state, data);
			break;

		case 1:
			scn2674_write_command(space->machine(), data);
			break;

		case 2: state->m_scn2674_screen1_l = data; break;
		case 3:
			state->m_scn2674_screen1_h = (data&0x3f);//uppermost two bytes not part of register
			state->m_scn2674_dbl1=(data & 0xc0)>>6;
			if (state->m_IR0_scn2674_double_ht_wd)
			{
				state->m_IR14_scn2674_double_1 = state->m_scn2674_dbl1;
				LOGSTUFF(("IR14 - Double 1 overridden %02x\n",state->m_IR14_scn2674_double_1));
			}
			break;

		case 4: state->m_scn2674_cursor_l  = data; break;
		case 5: state->m_scn2674_cursor_h  = data; break;
		case 6:	state->m_scn2674_screen2_l = data; break;
		case 7:
			state->m_scn2674_screen2_h = (data&0x3f);
			state->m_scn2674_spl1 = (data & 0x40);
			state->m_scn2674_spl2 = (data & 0x80);
			break;

		break;
	}
}


static VIDEO_START( mpu4_vid )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	/* if anything uses tile sizes other than 8x8 we can't really do it this way.. we'll have to draw tiles by hand.
      maybe we will anyway, but for now we don't need to */

	state->m_vid_vidram = auto_alloc_array(machine, UINT16, 0x20000/2);

	memset(state->m_vid_vidram,0,0x20000);

	/* find first empty slot to decode gfx */
	for (state->m_gfx_index = 0; state->m_gfx_index < MAX_GFX_ELEMENTS; state->m_gfx_index++)
		if (machine.gfx[state->m_gfx_index] == 0)
			break;

	assert(state->m_gfx_index != MAX_GFX_ELEMENTS);

	/* create the char set (gfx will then be updated dynamically from RAM) */
	machine.gfx[state->m_gfx_index+0] = gfx_element_alloc(machine, &mpu4_vid_char_8x8_layout, (UINT8 *)state->m_vid_vidram, machine.total_colors() / 16, 0);
	machine.gfx[state->m_gfx_index+1] = gfx_element_alloc(machine, &mpu4_vid_char_8x16_layout, (UINT8 *)state->m_vid_vidram, machine.total_colors() / 16, 0);
	machine.gfx[state->m_gfx_index+2] = gfx_element_alloc(machine, &mpu4_vid_char_16x8_layout, (UINT8 *)state->m_vid_vidram, machine.total_colors() / 16, 0);
	machine.gfx[state->m_gfx_index+3] = gfx_element_alloc(machine, &mpu4_vid_char_16x16_layout, (UINT8 *)state->m_vid_vidram, machine.total_colors() / 16, 0);

	state->m_scn2674_IR_pointer = 0;
}




/****************************
 *  EF9369 color palette IC
 *  (16 colors from 4096)
 ****************************/

/* Non-multiplexed mode */

static WRITE16_HANDLER( ef9369_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	struct ef9369_t &pal = state->m_pal;
	data &= 0x00ff;

	/* Address register */
	if (offset & 1)
	{
		pal.addr = data & 0x1f;
	}
	/* Data register */
	else
	{
		UINT32 entry = pal.addr >> 1;

		if ((pal.addr & 1) == 0)
		{
			pal.clut[entry] &= ~0x00ff;
			pal.clut[entry] |= data;
		}
		else
		{
			UINT16 col;

			pal.clut[entry] &= ~0x1f00;
			pal.clut[entry] |= (data & 0x1f) << 8;

			/* Remove the marking bit */
			col = pal.clut[entry] & 0xfff;

			/* Update the MAME palette */
			palette_set_color_rgb(space->machine(), entry, pal4bit(col >> 8), pal4bit(col >> 4), pal4bit(col >> 0));
		}

			/* Address register auto-increment */
		if (++pal.addr == 32)
			pal.addr = 0;
	}
}


static READ16_HANDLER( ef9369_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	struct ef9369_t &pal = state->m_pal;
	if ((offset & 1) == 0)
	{
		UINT16 col = pal.clut[pal.addr >> 1];

		if ((pal.addr & 1) == 0)
			return col & 0xff;
		else
			return col >> 8;
	}
	else
	{
		/* Address register is write only */
		return 0xffff;
	}
}

/******************************************
 *
 *  Brooktree Bt471 RAMDAC
 *  Implementation stolen from JPM
 *  Impact, may not be 100% (that has a 477)
 ******************************************/

/*
 *  0 0 0    Address register (RAM write mode)
 *  0 0 1    Color palette RAMs
 *  0 1 0    Pixel read mask register
 *  0 1 1    Address register (RAM read mode)
 *  1 0 0    Address register (overlay write mode)
 *  1 1 1    Address register (overlay read mode)
 *  1 0 1    Overlay register
 */

WRITE16_HANDLER( bt471_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	struct bt471_t &bt471 = state->m_bt471;
	UINT8 val = data & 0xff;
		{
			popmessage("Bt477: Unhandled write access (offset:%x, data:%x)", offset, val);
		}

	switch (offset)
	{
		case 0x0:
		{
			bt471.address = val;
			bt471.addr_cnt = 0;
			break;
		}
		case 0x1:
		{
			UINT8 *addr_cnt = &bt471.addr_cnt;
			rgb_t *color = &bt471.color;

			color[*addr_cnt] = val;

			if (++*addr_cnt == 3)
			{
				palette_set_color(space->machine(), bt471.address, MAKE_RGB(color[0], color[1], color[2]));
				*addr_cnt = 0;

				/* Address register increments */
				bt471.address++;
			}
			break;
		}
		case 0x2:
		{
			bt471.pixmask = val;
			break;
		}

//      default:
		{
			popmessage("Bt477: Unhandled write access (offset:%x, data:%x)", offset, val);
		}
	}
}

READ16_HANDLER( bt471_r )
{
	popmessage("Bt477: Unhandled read access (offset:%x)", offset);
	return 0;
}


/*************************************
 *
 *  Trackball interface
 *
 *************************************/

static READ8_DEVICE_HANDLER( pia_ic5_porta_track_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	/* The SWP trackball interface connects a standard trackball to the AUX1 port on the MPU4
    mainboard. As per usual, they've taken the cheap route here, reading and processing the
    raw quadrature signal from the encoder wheels for a 4 bit interface, rather than use any
    additional hardware to simplify matters. What makes matters worse is that there is a 45 degree rotation to take into account.
    For our purposes, two fake ports give the X and Y positions, which are then worked back into the signal levels.
    We invert the X and Y data at source due to the use of Schmitt triggers in the interface, which
    clean up the pulses and flip the active phase.*/

	LOG(("%s: IC5 PIA Read of Port A (AUX1)\n",device->machine().describe_context()));


	UINT8 data = input_port_read(device->machine(), "AUX1");

	INT8 dx = input_port_read(device->machine(), "TRACKX");
	INT8 dy = input_port_read(device->machine(), "TRACKY");

	state->m_cur[0] = dy + dx;
	state->m_cur[1] = dy - dx;

	UINT8 xa, xb, ya, yb;

	/* generate pulses for the input port (A and B are 1 unit out of phase for direction sensing)*/
	xa = ((state->m_cur[0] + 1) & 3) <= 1;
	xb = (state->m_cur[0] & 3) <= 1;
	ya = ((state->m_cur[1] + 1) & 3) <= 1;
	yb = (state->m_cur[1] & 3) <= 1;

	data |= (xa << 4); // XA
	data |= (ya << 5); // YA
	data |= (xb << 6); // XB
	data |= (yb << 7); // YB

	return data;
}

static const pia6821_interface pia_ic5t_intf =
{
	DEVCB_HANDLER(pia_ic5_porta_track_r),		/* port A in */
	DEVCB_HANDLER(pia_ic5_portb_r),	/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_NULL,		/* port A out */
	DEVCB_NULL,		/* port B out */
	DEVCB_LINE(pia_ic5_ca2_w),		/* line CA2 out */
	DEVCB_LINE(pia_ic5_cb2_w),		/* port CB2 out */
	DEVCB_LINE(cpu0_irq),			/* IRQA */
	DEVCB_LINE(cpu0_irq)			/* IRQB */
};

//Sampled sound timer
/*
For the video card, it appears that the standard MPU4 program card is interfaced to
the video board. Since this uses the E clock, our formula changes to

MSM6376 clock frequency:-
freq = (1000000/((t3L+1)(t3H+1)))*[(t3H(T3L+1)+1)/(2(t1+1))]
where [] means rounded up integer,
t3L is the LSB of Clock 3,
t3H is the MSB of Clock 3,
and t1 is the initial value in clock 1.
*/

//O3 -> G1  O1 -> c2 o2 -> c1
static WRITE8_DEVICE_HANDLER( ic3ss_vid_o1_callback )
{
	downcast<ptm6840_device *>(device)->set_c2(data);
}


static WRITE8_DEVICE_HANDLER( ic3ss_vid_o2_callback )//Generates 'beep' tone
{
	downcast<ptm6840_device *>(device)->set_c1(data); /* this output is the clock for timer1 */
}


static WRITE8_DEVICE_HANDLER( ic3ss_vid_o3_callback )
{
	downcast<ptm6840_device *>(device)->set_g1(data); /* this output is the clock for timer1 */
}

static WRITE8_HANDLER( ic3ss_vid_w )
{
	//This would seem to be right, needs checking against a PCB
	device_t *ic3ssv = space->machine().device("ptm_ic3ss_vid");
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	downcast<ptm6840_device *>(ic3ssv)->write(offset,data);//can speed things up if this is disabled
	device_t *msm6376 = space->machine().device("msm6376");

	if (offset == 3)
	{
		state->m_t1 = data;
	}
	if (offset == 6)
	{
		state->m_t3h = data;
	}
	if (offset == 7)
	{
		state->m_t3l = data;
	}

	float num = ((VIDEO_MASTER_CLOCK / 10)/((state->m_t3l + 1)*(state->m_t3h + 1)));
	float denom1 = ((state->m_t3h *(state->m_t3l + 1)+ 1)/(2*(state->m_t1 + 1)));

	int denom2 = denom1 +0.5;//need to round up, this gives same precision as chip
	int freq=num*denom2;

	if (freq)
	{
		okim6376_set_frequency(msm6376, freq);
	}
}

static const ptm6840_interface ptm_ic3ss_vid_intf =
{
	VIDEO_MASTER_CLOCK / 10, /* 68k E clock */
	{ 0, 0, 0 },
	{ DEVCB_HANDLER(ic3ss_vid_o1_callback),
	  DEVCB_HANDLER(ic3ss_vid_o2_callback),
	  DEVCB_HANDLER(ic3ss_vid_o3_callback) },
	DEVCB_NULL
};

/*************************************
 *
 *  Input defintions
 *
 *************************************/

static INPUT_PORTS_START( crmaze )
	PORT_START("ORANGE1")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("ORANGE2")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Door Switch?") PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Right Yellow")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Right Red")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Left Red")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Left Yellow")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Getout Yellow")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Getout Red")/* Labelled Escape on cabinet */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)//resets game if pressed - sometimes hoppers run here, but crmaze has tubes
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_SPECIAL)//XA
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SPECIAL)//YA
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SPECIAL)//XB
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_SPECIAL)//YB

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")

	PORT_START("TRACKX")//FAKE
    PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_INVERT
	PORT_START("TRACKY")//FAKE
    PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_INVERT
INPUT_PORTS_END

static INPUT_PORTS_START( mating )
	PORT_START("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W) PORT_TOGGLE
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Right Yellow")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Right Red") // selects the answer
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Left Yellow")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Left Red")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("100p Service?")

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_SPECIAL)//XA
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SPECIAL)//YA
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SPECIAL)//XB
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_SPECIAL)//YB

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")

	PORT_START("TRACKX")//FAKE
    PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_INVERT
	PORT_START("TRACKY")//FAKE
    PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_INVERT

INPUT_PORTS_END


static INPUT_PORTS_START( dealem )
	PORT_START("ORANGE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_COIN5) PORT_NAME("20p Token")PORT_IMPULSE(5)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x04)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x05)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x06)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x07)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x08)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x09)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Gamble")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_START2) PORT_NAME("Pontoon")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x01)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x09)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x03)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x04)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x05)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x06)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x07)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x08)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Rear Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x00)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Hi-Lo") PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x02)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("Twist")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lo")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Hi")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Stick")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Collect")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Deal")

	PORT_START("DIL1")
	PORT_DIPNAME( 0x0f, 0x00, "Cabinet Set Up Mode" ) PORT_DIPLOCATION("DIL1:01,02,03,04")
	PORT_DIPSETTING(    0x00, "Stop The Clock" )
	PORT_DIPSETTING(    0x01, "Hit the Top" )
	PORT_DIPSETTING(    0x02, "Way In" )
	PORT_DIPSETTING(    0x03, "Smash and Grab" )
	PORT_DIPSETTING(    0x04, "Ready Steady Go-1" )
	PORT_DIPSETTING(    0x05, "Ready Steady Go-2" )
	PORT_DIPSETTING(    0x06, "Top Gears-1" )
	PORT_DIPSETTING(    0x07, "Top Gears-2" )
	PORT_DIPSETTING(    0x08, "Nifty Fifty" )
	PORT_DIPSETTING(    0x09, "Super Tubes" )
	PORT_DIPNAME( 0x70, 0x00, "Target Payout Percentage" ) PORT_DIPLOCATION("DIL1:05,06,07")
	PORT_DIPSETTING(    0x00, "72%" )
	PORT_DIPSETTING(    0x10, "74%" )
	PORT_DIPSETTING(    0x20, "76%" )
	PORT_DIPSETTING(    0x30, "78%" )
	PORT_DIPSETTING(    0x40, "80%" )
	PORT_DIPSETTING(    0x50, "82%" )
	PORT_DIPSETTING(    0x60, "84%" )
	PORT_DIPSETTING(    0x70, "86%" )
	PORT_DIPNAME( 0x80, 0x00, "Display Switch Settings on Monitor" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "Payout Limit" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, "200p (All Cash)")
	PORT_DIPSETTING(    0x01, "200p (Cash)+400p (Token)")
	PORT_DIPNAME( 0x02, 0x00, "10p Payout Priority" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "Clear Credits and bank at power on?" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x08, 0x00, "50p Payout Solenoid fitted?" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x10, 0x00, "100p Payout Solenoid fitted?" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin alarms active?" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x40, 0x00, "Price of Play" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, "10p 1 Game" )
	PORT_DIPSETTING(    0x40, "10p 2 Games" )
	PORT_DIPNAME( 0x80, 0x00, "Coin Entry" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, "Multi" )
	PORT_DIPSETTING(    0x80, DEF_STR(Single))

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5) PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x00)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5) PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x01)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5) PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x02)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5) PORT_CONDITION("DIL1",0x0f,PORTCOND_EQUALS,0x03)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static INPUT_PORTS_START( skiltrek )
	PORT_START("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Pass")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("C")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("B")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("A")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Continue")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("C")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("B")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON8) PORT_NAME("A")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "1 Pound for change" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "Attract mode inhibit" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin alarm inhibit" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "Single coin entry" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static INPUT_PORTS_START( turnover )
	PORT_START("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Pass")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("C")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("B")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("A")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Continue")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("C")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("B")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON8) PORT_NAME("A")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_START1)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "1 Pound for change" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "Attract mode inhibit" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin alarm inhibit" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "Single coin entry" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static INPUT_PORTS_START( adders )
	PORT_START("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("C")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("B")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("A")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Pass")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("C")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("B")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("A")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "1 Pound for change" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "Attract mode inhibit" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin alarm inhibit" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "Single coin entry" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static void video_reset(device_t *device)
{
	device->machine().device("6840ptm_68k")->reset();
	device->machine().device("acia6850_1")->reset();
}

/* machine start (called only once) */
static MACHINE_START( mpu4_vid )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	mpu4_config_common(machine);

	state->m_mod_number=4; //No AY chip
	/* setup communications */
	state->m_link7a_connected = 1;

	/* setup 8 mechanical meters */
	MechMtr_config(machine,8);

	/* setup the standard oki MSC1937 display */
	ROC10937_init(0, MSC1937, 0);

	/* Hook the reset line */
	m68k_set_reset_callback(machine.device("video"), video_reset);
}

static MACHINE_RESET( mpu4_vid )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	ROC10937_reset(0);

	mpu4_stepper_reset(state);

	state->m_lamp_strobe    = 0;
	state->m_lamp_strobe2   = 0;
	state->m_led_strobe     = 0;

	state->m_IC23GC    = 0;
	state->m_IC23GB    = 0;
	state->m_IC23GA    = 0;
	state->m_IC23G1    = 1;
	state->m_IC23G2A   = 0;
	state->m_IC23G2B   = 0;

	state->m_prot_col  = 0;
	state->m_chr_counter    = 0;
	state->m_chr_value		= 0;
}

static ADDRESS_MAP_START( mpu4_68k_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x7fffff) AM_ROM
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE_MEMBER(mpu4_state, m_vid_mainram)// AM_MIRROR(0x10000)
//  AM_RANGE(0x810000, 0x81ffff) AM_RAM /* ? */
	AM_RANGE(0x900000, 0x900001) AM_DEVWRITE8("saa", saa1099_data_w, 0x00ff)
	AM_RANGE(0x900002, 0x900003) AM_DEVWRITE8("saa", saa1099_control_w, 0x00ff)
	AM_RANGE(0xa00000, 0xa00003) AM_READWRITE(ef9369_r, ef9369_w)
/*  AM_RANGE(0xa00004, 0xa0000f) AM_READWRITE(mpu4_vid_unmap_r, mpu4_vid_unmap_w) */
	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)//where is BLANK?
	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)
	AM_RANGE(0xff8000, 0xff8001) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, status_read, control_write, 0xff)
	AM_RANGE(0xff8002, 0xff8003) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, data_read, data_write, 0xff)
	AM_RANGE(0xff9000, 0xff900f) AM_DEVREADWRITE8_MODERN("6840ptm_68k", ptm6840_device, read, write, 0xff)
	AM_RANGE(0xffd000, 0xffd00f) AM_READWRITE(characteriser16_r, characteriser16_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpu4oki_68k_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x5fffff) AM_ROM //AM_WRITENOP
	AM_RANGE(0x600000, 0x63ffff) AM_RAM	/* The Mating Game has an extra 256kB RAM on the program card */
//  AM_RANGE(0x640000, 0x7fffff) AM_NOP /* Possible bug, reads and writes here */
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE_MEMBER(mpu4_state, m_vid_mainram)
	AM_RANGE(0x900000, 0x900001) AM_DEVWRITE8("saa", saa1099_data_w, 0x00ff)
	AM_RANGE(0x900002, 0x900003) AM_DEVWRITE8("saa", saa1099_control_w, 0x00ff)
	AM_RANGE(0xa00000, 0xa00003) AM_READWRITE(ef9369_r, ef9369_w)
	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)
	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)
	AM_RANGE(0xff8000, 0xff8001) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, status_read, control_write, 0xff)
	AM_RANGE(0xff8002, 0xff8003) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, data_read, data_write, 0xff)
	AM_RANGE(0xff9000, 0xff900f) AM_DEVREADWRITE8_MODERN("6840ptm_68k", ptm6840_device, read, write, 0xff)
	AM_RANGE(0xffa040, 0xffa04f) AM_DEVREAD8_MODERN("ptm_ic3ss_vid", ptm6840_device, read,0xff)  // 6840PTM on sampled sound board
	AM_RANGE(0xffa040, 0xffa04f) AM_WRITE8(ic3ss_vid_w,0x00ff)  // 6840PTM on sampled sound board
	AM_RANGE(0xffa060, 0xffa067) AM_DEVREADWRITE8_MODERN("pia_ic4ss", pia6821_device, read, write,0x00ff)    // PIA6821 on sampled sound board
	AM_RANGE(0xffd000, 0xffd00f) AM_READWRITE(characteriser16_r, characteriser16_w)
//  AM_RANGE(0xfff000, 0xffffff) AM_NOP /* Possible bug, reads and writes here */
ADDRESS_MAP_END

/* TODO: Fix up MPU4 map*/
static ADDRESS_MAP_START( mpu4_6809_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_SHARE("nvram")
	AM_RANGE(0x0800, 0x0800) AM_DEVREADWRITE_MODERN("acia6850_0", acia6850_device, status_read, control_write)
	AM_RANGE(0x0801, 0x0801) AM_DEVREADWRITE_MODERN("acia6850_0", acia6850_device, data_read, data_write)
	AM_RANGE(0x0880, 0x0881) AM_NOP //Read/write here
	AM_RANGE(0x0900, 0x0907) AM_DEVREADWRITE_MODERN("ptm_ic2", ptm6840_device, read, write)
	AM_RANGE(0x0a00, 0x0a03) AM_DEVREADWRITE_MODERN("pia_ic3", pia6821_device, read, write)
	AM_RANGE(0x0b00, 0x0b03) AM_DEVREADWRITE_MODERN("pia_ic4", pia6821_device, read, write)
	AM_RANGE(0x0c00, 0x0c03) AM_DEVREADWRITE_MODERN("pia_ic5", pia6821_device, read, write)
	AM_RANGE(0x0d00, 0x0d03) AM_DEVREADWRITE_MODERN("pia_ic6", pia6821_device, read, write)
	AM_RANGE(0x0e00, 0x0e03) AM_DEVREADWRITE_MODERN("pia_ic7", pia6821_device, read, write)
	AM_RANGE(0x0f00, 0x0f03) AM_DEVREADWRITE_MODERN("pia_ic8", pia6821_device, read, write)
	AM_RANGE(0x4000, 0x7fff) AM_RAM
	AM_RANGE(0xbe00, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM	AM_REGION("maincpu",0)  /* 64k EPROM on board, only this region read */
ADDRESS_MAP_END

static ADDRESS_MAP_START( vp_68k_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x7fffff) AM_ROM
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE_MEMBER(mpu4_state, m_vid_mainram)
	AM_RANGE(0x810000, 0x81ffff) AM_RAM /* ? */
	AM_RANGE(0x900000, 0x900001) AM_DEVWRITE8("saa", saa1099_data_w, 0x00ff)
	AM_RANGE(0x900002, 0x900003) AM_DEVWRITE8("saa", saa1099_control_w, 0x00ff)
	AM_RANGE(0xa00000, 0xa00003) AM_READWRITE(ef9369_r, ef9369_w)
/*  AM_RANGE(0xa00004, 0xa0000f) AM_READWRITE(mpu4_vid_unmap_r, mpu4_vid_unmap_w) */
	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)
	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)
/*  AM_RANGE(0xe05000, 0xe05001) AM_READWRITE(adpcm_r, adpcm_w) */
	AM_RANGE(0xff8000, 0xff8001) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, status_read, control_write, 0xff)
	AM_RANGE(0xff8002, 0xff8003) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, data_read, data_write, 0xff)
	AM_RANGE(0xff9000, 0xff900f) AM_DEVREADWRITE8_MODERN("6840ptm_68k", ptm6840_device, read, write, 0xff)
/*  AM_RANGE(0xffd000, 0xffd00f) AM_READWRITE(characteriser16_r, characteriser16_w) Word-based version of old CHR??? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bwbvid_6809_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_SHARE("nvram")
	AM_RANGE(0x0800, 0x0800) AM_DEVREADWRITE_MODERN("acia6850_0", acia6850_device, status_read, control_write)
	AM_RANGE(0x0801, 0x0801) AM_DEVREADWRITE_MODERN("acia6850_0", acia6850_device, data_read, data_write)
	AM_RANGE(0x0880, 0x0881) //AM_NOP //Read/write here
	AM_RANGE(0x0900, 0x0907) AM_DEVREADWRITE_MODERN("ptm_ic2", ptm6840_device, read, write)
	AM_RANGE(0x0a00, 0x0a03) AM_DEVREADWRITE_MODERN("pia_ic3", pia6821_device, read, write)
	AM_RANGE(0x0b00, 0x0b03) AM_DEVREADWRITE_MODERN("pia_ic4", pia6821_device, read, write)
	AM_RANGE(0x0c00, 0x0c03) AM_DEVREADWRITE_MODERN("pia_ic5", pia6821_device, read, write)
	AM_RANGE(0x0d00, 0x0d03) AM_DEVREADWRITE_MODERN("pia_ic6", pia6821_device, read, write)
	AM_RANGE(0x0e00, 0x0e03) AM_DEVREADWRITE_MODERN("pia_ic7", pia6821_device, read, write)
	AM_RANGE(0x0f00, 0x0f03) AM_DEVREADWRITE_MODERN("pia_ic8", pia6821_device, read, write)
	AM_RANGE(0x4000, 0x7fff) AM_RAM
	AM_RANGE(0xbe00, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM	AM_REGION("maincpu",0)  /* 64k EPROM on board, only this region read */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bwbvid_68k_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x7fffff) AM_ROM
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE_MEMBER(mpu4_state, m_vid_mainram)
	AM_RANGE(0x810000, 0x81ffff) AM_RAM /* ? */
	AM_RANGE(0x900000, 0x900001) AM_DEVWRITE8("saa", saa1099_data_w, 0x00ff)
	AM_RANGE(0x900002, 0x900003) AM_DEVWRITE8("saa", saa1099_control_w, 0x00ff)
	AM_RANGE(0xa00000, 0xa00003) AM_READWRITE(ef9369_r, ef9369_w)
//  AM_RANGE(0xa00000, 0xa0000f) AM_READWRITE(bt471_r,bt471_w) //Some games use this
/*  AM_RANGE(0xa00004, 0xa0000f) AM_READWRITE(mpu4_vid_unmap_r, mpu4_vid_unmap_w) */
	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)
	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)
	AM_RANGE(0xe00000, 0xe00001) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, status_read, control_write, 0xff)
	AM_RANGE(0xe00002, 0xe00003) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, data_read, data_write, 0xff)
	AM_RANGE(0xe01000, 0xe0100f) AM_DEVREADWRITE8_MODERN("6840ptm_68k", ptm6840_device, read, write, 0xff)
	//AM_RANGE(0xa00004, 0xa0000f) AM_READWRITE(bwb_characteriser16_r, bwb_characteriser16_w)//AM_READWRITE(adpcm_r, adpcm_w)  CHR ?
ADDRESS_MAP_END

static ADDRESS_MAP_START( bwbvid5_68k_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x7fffff) AM_ROM
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE_MEMBER(mpu4_state, m_vid_mainram)
	AM_RANGE(0x810000, 0x81ffff) AM_RAM /* ? */
	AM_RANGE(0x900000, 0x900001) AM_DEVWRITE8("saa", saa1099_data_w, 0x00ff)
	AM_RANGE(0x900002, 0x900003) AM_DEVWRITE8("saa", saa1099_control_w, 0x00ff)
	AM_RANGE(0xa00000, 0xa00003) AM_READWRITE(ef9369_r, ef9369_w)
	//AM_RANGE(0xa00000, 0xa00003) AM_READWRITE(bt471_r,bt471_w) Some games use this
/*  AM_RANGE(0xa00004, 0xa0000f) AM_READWRITE(mpu4_vid_unmap_r, mpu4_vid_unmap_w) */
	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)
	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)
	AM_RANGE(0xe00000, 0xe00001) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, status_read, control_write, 0xff)
	AM_RANGE(0xe00002, 0xe00003) AM_DEVREADWRITE8_MODERN("acia6850_1", acia6850_device, data_read, data_write, 0xff)
	AM_RANGE(0xe01000, 0xe0100f) AM_DEVREADWRITE8_MODERN("6840ptm_68k", ptm6840_device, read, write, 0x00ff)
	AM_RANGE(0xe02000, 0xe02007) AM_DEVREADWRITE8_MODERN("pia_ic4ss", pia6821_device, read, write, 0xff00)
	AM_RANGE(0xe03000, 0xe0300f) AM_DEVREAD8_MODERN("ptm_ic3ss_vid", ptm6840_device, read,0xff00)  // 6840PTM on sampled sound board
	AM_RANGE(0xe03000, 0xe0300f) AM_WRITE8(ic3ss_vid_w,0xff00)  // 6840PTM on sampled sound board
	AM_RANGE(0xe04000, 0xe0400f) AM_READWRITE(bwb_characteriser16_r, bwb_characteriser16_w)//AM_READWRITE(adpcm_r, adpcm_w)  CHR ?
ADDRESS_MAP_END

/* Deal 'Em */
/* Deal 'Em was designed as an enhanced gamecard, to fit into an existing MPU4 cabinet
It's an unoffical addon, and does all its work through the existing 6809 CPU.
Although given unofficial status, Barcrest's patent on the MPU4 Video hardware (GB1596363) describes
the Deal 'Em board design, rather than the one they ultimately used, suggesting some sort of licensing deal. */

static const gfx_layout dealemcharlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 3*4, 2*4, 1*4, 0*4, 7*4, 6*4, 5*4, 4*4  },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static GFXDECODE_START( dealem )
	GFXDECODE_ENTRY( "gfx1", 0x0000, dealemcharlayout, 0, 32 )
GFXDECODE_END



/***************************************************************************

  Convert the color PROMs into a more useable format.

  The palette PROM is connected to the RGB output this way:

  Red:      1K      Bit 0
            470R
            220R

  Green:    1K      Bit 3
            470R
            220R

  Blue:     470R
            220R    Bit 7

  Everything is also tied to a 1K pulldown resistor
***************************************************************************/


static PALETTE_INIT( dealem )
{
	int i, len;
	static const int resistances_rg[3] = { 1000, 470, 220 };
	static const int resistances_b [2] = { 470, 220 };
	double weights_r[3], weights_g[3], weights_b[2];

	compute_resistor_weights(0,	255,	-1.0,
			3,	resistances_rg,	weights_r,	1000,	0,
			3,	resistances_rg,	weights_g,	1000,	0,
			2,	resistances_b,	weights_b,	1000,	0);

	len = machine.region("proms")->bytes();
	for (i = 0; i < len; i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = combine_3_weights(weights_r, bit0, bit1, bit2);
		/* green component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		g = combine_3_weights(weights_g, bit0, bit1, bit2);
		/* blue component */
		bit0 = BIT(*color_prom,6);
		bit1 = BIT(*color_prom,7);
		b = combine_2_weights(weights_b, bit0, bit1);

		palette_set_color(machine,i,MAKE_RGB(r,g,b));
		color_prom++;
	}
}


static SCREEN_UPDATE(dealem)
{
	mpu4_state *state = screen->machine().driver_data<mpu4_state>();
	int x,y;
	int count = 0;

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 40; x++)
		{
			int tile = state->m_dealem_videoram[count + 0x1000] | (state->m_dealem_videoram[count] << 8);
			count++;
			drawgfx_opaque(bitmap,cliprect,screen->machine().gfx[0],tile,0,0,0,x * 8,y * 8);
		}
	}

	return 0;
}


static WRITE_LINE_DEVICE_HANDLER( dealem_vsync_changed )
{
	cputag_set_input_line(device->machine(), "maincpu", INPUT_LINE_NMI, state);
}


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static const mc6845_interface hd6845_intf =
{
	"screen",							/* screen we are acting on */
	8,									/* number of pixels per video memory address */
	NULL,								/* before pixel update callback */
	NULL,								/* row update callback */
	NULL,								/* after pixel update callback */
	DEVCB_NULL,							/* callback for display state changes */
	DEVCB_NULL,							/* callback for cursor state changes */
	DEVCB_NULL,							/* HSYNC callback */
	DEVCB_LINE(dealem_vsync_changed),	/* VSYNC callback */
	NULL								/* update address callback */
};


static ADDRESS_MAP_START( dealem_memmap, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_SHARE("nvram")

	AM_RANGE(0x0800, 0x0800) AM_DEVWRITE_MODERN("crtc", mc6845_device, address_w)
	AM_RANGE(0x0801, 0x0801) AM_DEVREADWRITE_MODERN("crtc", mc6845_device, register_r, register_w)

/*  AM_RANGE(0x08e0, 0x08e7) AM_READWRITE(68681_duart_r,68681_duart_w) */ //Runs hoppers

	AM_RANGE(0x0900, 0x0907) AM_DEVREADWRITE_MODERN("ptm_ic2", ptm6840_device, read, write)/* PTM6840 IC2 */

	AM_RANGE(0x0a00, 0x0a03) AM_DEVREADWRITE_MODERN("pia_ic3", pia6821_device, read, write)		/* PIA6821 IC3 */
	AM_RANGE(0x0b00, 0x0b03) AM_DEVREADWRITE_MODERN("pia_ic4", pia6821_device, read, write)		/* PIA6821 IC4 */
	AM_RANGE(0x0c00, 0x0c03) AM_DEVREADWRITE_MODERN("pia_ic5", pia6821_device, read, write)		/* PIA6821 IC5 */
	AM_RANGE(0x0d00, 0x0d03) AM_DEVREADWRITE_MODERN("pia_ic6", pia6821_device, read, write)		/* PIA6821 IC6 */
	AM_RANGE(0x0e00, 0x0e03) AM_DEVREADWRITE_MODERN("pia_ic7", pia6821_device, read, write)		/* PIA6821 IC7 */
	AM_RANGE(0x0f00, 0x0f03) AM_DEVREADWRITE_MODERN("pia_ic8", pia6821_device, read, write)		/* PIA6821 IC8 */

	AM_RANGE(0x1000, 0x2fff) AM_RAM AM_BASE_MEMBER(mpu4_state, m_dealem_videoram)
	AM_RANGE(0x8000, 0xffff) AM_ROM	AM_WRITENOP/* 64k  paged ROM (4 pages) */
ADDRESS_MAP_END


static void scn2674_line(running_machine &machine)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();

	if (state->m_linecounter==0)//front porch
	{
		// these will be used to track which row / line we're on eventually
		// and used by the renderer to render the correct data

		state->m_scn2674_status_register |= 0x10;
		if (state->m_scn2674_irq_mask&0x10)
		{
			LOG2674(("vblank irq\n"));
			state->m_scn2674_irq_state = 1;
			state->m_scn2674_irq_register |= 0x10;
			update_mpu68_interrupts(machine);
		}

	}

	if (state->m_linecounter==4)/* Ready - this triggers for the first scanline of the screen */
	{
		state->m_scn2674_status_register |= 0x02;
		if (state->m_scn2674_irq_mask&0x02)
		{
			LOG2674(("SCN2674 Ready\n"));
			state->m_scn2674_irq_state = 1;
			state->m_scn2674_irq_register |= 0x02;
			update_mpu68_interrupts(machine);
		}
	}

	// should be triggered at the start of each ROW (line zero for that row)
	if ((state->m_linecounter+4)%8 == 0)
	{
		state->m_scn2674_status_register |= 0x08;
		if (state->m_scn2674_irq_mask&0x08)
		{
			LOG2674(("SCN2674 Line Zero\n"));
			state->m_scn2674_irq_state = 1;
			state->m_scn2674_irq_register |= 0x08;
			update_mpu68_interrupts(machine);
		}
		state->m_rowcounter++;
	}
		if (state->m_linecounter%7 == 0)
		{
			state->m_rowcounter++;
		}


	// this is ROWS not scanlines!!
	if (state->m_linecounter+4 == state->m_IR12_scn2674_split_register_1*8)
	/* Split Screen 1 */
	{
		if (state->m_scn2674_spl1)
		{
			popmessage("Split screen 1 address shift required, contact MAMEDEV");
		}
		state->m_scn2674_status_register |= 0x04;
		if (state->m_scn2674_irq_mask&0x04)
		{
			state->m_scn2674_irq_register |= 0x04;
			LOG2674(("SCN2674 Split Screen 1\n"));
			state->m_scn2674_irq_state = 1;
			update_mpu68_interrupts(machine);
			machine.primary_screen->update_partial(machine.primary_screen->vpos());
		}
	}

	// this is in ROWS not scanlines!!!
	if (state->m_linecounter+4 == state->m_IR13_scn2674_split_register_2*8)
	/* Split Screen 2 */
	{
		if (state->m_scn2674_spl2)
		{
			popmessage("Split screen 2 address shift required, contact MAMEDEV");
		}
		state->m_scn2674_status_register |= 0x01;
		if (state->m_scn2674_irq_mask&0x01)
		{
			LOG2674(("SCN2674 Split Screen 2 irq\n"));
			state->m_scn2674_irq_state = 1;
			state->m_scn2674_irq_register |= 0x01;
			update_mpu68_interrupts(machine);
			machine.primary_screen->update_partial(machine.primary_screen->vpos());
		}
	}
}


static TIMER_DEVICE_CALLBACK( scanline_timer_callback )
{
	mpu4_state *state = timer.machine().driver_data<mpu4_state>();

	if ((state->m_scn2674_display_enabled_scanline)&&(!state->m_scn2674_display_enabled))
	{
		state->m_scn2674_display_enabled = 1;
		//state->m_linecounter = 0;
	}

	if (state->m_scn2674_display_enabled)
	{
		state->m_linecounter = ((state->m_linecounter+1)%313);
	}

	scn2674_line(timer.machine());
	timer.machine().scheduler().synchronize();

}


static MACHINE_CONFIG_START( mpu4_vid, mpu4_state )
	MCFG_CPU_ADD("maincpu", M6809, MPU4_MASTER_CLOCK/4 )
	MCFG_CPU_PROGRAM_MAP(mpu4_6809_map)
	MCFG_TIMER_ADD_PERIODIC("50hz",gen_50hz, attotime::from_hz(100))

	MCFG_NVRAM_ADD_0FILL("nvram")				/* confirm */

	/* 6840 PTM */
	MCFG_PTM6840_ADD("ptm_ic2", ptm_ic2_intf)

	MCFG_PIA6821_ADD("pia_ic3", pia_ic3_intf)
	MCFG_PIA6821_ADD("pia_ic4", pia_ic4_intf)
	MCFG_PIA6821_ADD("pia_ic5", pia_ic5_intf)
	MCFG_PIA6821_ADD("pia_ic6", pia_ic6_intf)
	MCFG_PIA6821_ADD("pia_ic7", pia_ic7_intf)
	MCFG_PIA6821_ADD("pia_ic8", pia_ic8_intf)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE((63*8)+(17*8), (37*8)+17) // note this directly affects the scanline counters used below, and thus the timing of everything
	MCFG_SCREEN_VISIBLE_AREA(4*8, (63*8)+(4*8)-1, 4, (37*8)+4-1)

	MCFG_SCREEN_REFRESH_RATE(50)
//  MCFG_SCREEN_RAW_PARAMS(4000000*2, 80*8, 4, (63*8)+4, (37*8)+10+4+3, 4, (37*8)+4)//42*8, 0*8, 37*8) 3
	MCFG_SCREEN_UPDATE(mpu4_vid)

	MCFG_CPU_ADD("video", M68000, VIDEO_MASTER_CLOCK )
	MCFG_CPU_PROGRAM_MAP(mpu4_68k_map)

//  MCFG_QUANTUM_TIME(attotime::from_hz(960))

	MCFG_MACHINE_START(mpu4_vid)
	MCFG_MACHINE_RESET(mpu4_vid)
	MCFG_VIDEO_START (mpu4_vid)

	MCFG_PALETTE_LENGTH(16)

	MCFG_PTM6840_ADD("6840ptm_68k", ptm_vid_intf)
	/* Present on all video cards */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_SOUND_ADD("saa", SAA1099, 8000000)
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(1, "rspeaker", 0.5)

	MCFG_ACIA6850_ADD("acia6850_0", m6809_acia_if)
	MCFG_ACIA6850_ADD("acia6850_1", m68k_acia_if)

	// for the video timing
	MCFG_TIMER_ADD_SCANLINE("scan_timer", scanline_timer_callback, "screen", 0, 1)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( crmaze, mpu4_vid )
	MCFG_PIA6821_MODIFY("pia_ic5", pia_ic5t_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( mating, crmaze )
	MCFG_CPU_MODIFY("video")
	MCFG_CPU_PROGRAM_MAP(mpu4oki_68k_map)
	MCFG_PTM6840_ADD("ptm_ic3ss_vid", ptm_ic3ss_intf)
	MCFG_PIA6821_ADD("pia_ic4ss", pia_ic4ss_intf)

	MCFG_SOUND_ADD("msm6376", OKIM6376, 128000) //?
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(1, "rspeaker", 0.5)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( vgpoker, mpu4_vid )
	MCFG_CPU_MODIFY("video")
	MCFG_CPU_PROGRAM_MAP(vp_68k_map)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( bwbvid, mpu4_state )
	MCFG_CPU_ADD("maincpu", M6809, MPU4_MASTER_CLOCK/4 )
	MCFG_CPU_PROGRAM_MAP(bwbvid_6809_map)
	MCFG_TIMER_ADD_PERIODIC("50hz",gen_50hz, attotime::from_hz(100))

	MCFG_NVRAM_ADD_0FILL("nvram")				/* confirm */

	/* 6840 PTM */
	MCFG_PTM6840_ADD("ptm_ic2", ptm_ic2_intf)

	MCFG_PIA6821_ADD("pia_ic3", pia_ic3_intf)
	MCFG_PIA6821_ADD("pia_ic4", pia_ic4_intf)
	MCFG_PIA6821_ADD("pia_ic5", pia_ic5_intf)
	MCFG_PIA6821_ADD("pia_ic6", pia_ic6_intf)
	MCFG_PIA6821_ADD("pia_ic7", pia_ic7_intf)
	MCFG_PIA6821_ADD("pia_ic8", pia_ic8_intf)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(64*8, 40*8) // note this directly affects the scanline counters used below, and thus the timing of everything
	MCFG_SCREEN_VISIBLE_AREA(0*8, 63*8-1, 0*8, 37*8-1)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_UPDATE(mpu4_vid)

	MCFG_CPU_ADD("video", M68000, VIDEO_MASTER_CLOCK )
	MCFG_CPU_PROGRAM_MAP(bwbvid_68k_map)

	MCFG_QUANTUM_TIME(attotime::from_hz(960))

	MCFG_MACHINE_START(mpu4_vid)
	MCFG_MACHINE_RESET(mpu4_vid)
	MCFG_VIDEO_START (mpu4_vid)

	MCFG_PALETTE_LENGTH(16)

	MCFG_PTM6840_ADD("6840ptm_68k", ptm_vid_intf)

	/* Present on all video cards */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_SOUND_ADD("saa", SAA1099, 8000000)
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(1, "rspeaker", 0.5)

	MCFG_ACIA6850_ADD("acia6850_0", m6809_acia_if)
	MCFG_ACIA6850_ADD("acia6850_1", m68k_acia_if)

	// for the video timing
	MCFG_TIMER_ADD_SCANLINE("scan_timer", scanline_timer_callback, "screen", 0, 1)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( bwbvid5, bwbvid )
	MCFG_CPU_MODIFY("video")
	MCFG_CPU_PROGRAM_MAP(bwbvid5_68k_map)

	MCFG_PTM6840_ADD("ptm_ic3ss_vid", ptm_ic3ss_intf)
	MCFG_PIA6821_ADD("pia_ic4ss", pia_ic4ss_intf)

	MCFG_SOUND_ADD("msm6376", OKIM6376, 128000) //?
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(1, "rspeaker", 0.5)

MACHINE_CONFIG_END

/* machine driver for Zenitone Deal 'Em board */
static MACHINE_CONFIG_START( dealem, mpu4_state )
	MCFG_MACHINE_START(mpu4mod2)							/* main mpu4 board initialisation */
	MCFG_MACHINE_RESET(mpu4_vid)
	MCFG_CPU_ADD("maincpu", M6809, MPU4_MASTER_CLOCK/4)
	MCFG_CPU_PROGRAM_MAP(dealem_memmap)

	MCFG_TIMER_ADD_PERIODIC("50hz",gen_50hz, attotime::from_hz(100))

	MCFG_PTM6840_ADD("ptm_ic2", ptm_ic2_intf)

	MCFG_PIA6821_ADD("pia_ic3", pia_ic3_intf)
	MCFG_PIA6821_ADD("pia_ic4", pia_ic4_intf)
	MCFG_PIA6821_ADD("pia_ic5", pia_ic5_intf)
	MCFG_PIA6821_ADD("pia_ic6", pia_ic6_intf)
	MCFG_PIA6821_ADD("pia_ic7", pia_ic7_intf)
	MCFG_PIA6821_ADD("pia_ic8", pia_ic8_intf)

	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ay8913",AY8913, MPU4_MASTER_CLOCK/4)
	MCFG_SOUND_CONFIG(ay8910_config)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_NVRAM_ADD_0FILL("nvram")

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE((54+1)*8, (32+1)*8)					/* Taken from 6845 init, registers 00 & 04. Normally programmed with (value-1) */
	MCFG_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 31*8-1)		/* Taken from 6845 init, registers 01 & 06 */
	MCFG_SCREEN_REFRESH_RATE(56)							/* Measured accurately from the flip-flop, but 6845 handles this */
	MCFG_SCREEN_UPDATE(dealem)

	MCFG_GFXDECODE(dealem)

	MCFG_PALETTE_LENGTH(32)
	MCFG_PALETTE_INIT(dealem)

	MCFG_MC6845_ADD("crtc", HD6845, MPU4_MASTER_CLOCK / 4 / 8, hd6845_intf)	/* HD68B45 */
MACHINE_CONFIG_END



/*
Characteriser (CHR)
 The question data on the quiz games gets passed through the characterizer, the tables tested at startup are just a
 very specific test with known responses to make sure the device functions properly.  Unless there is extra encryption
 applied to just the question ROMs then the assumptions made here are wrong, because the questions don't decode.

 Perhaps the address lines for the question ROMS are scrambled somehow to make things decode, but how?

 It seems more likely that the Characterizer (PAL) acts as a challenge / response system, but various writes cause
 'latching' behavior because if you study the sequence written at startup you can see that the same write value should
 generate different responses.

 Note:
 the 'challenge' part of the startup check is always the same
*/

static WRITE16_HANDLER( characteriser16_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	int x;
	int call=data;
	LOG_CHR_FULL(("%04x Characteriser write offset %02X data %02X", cpu_get_previouspc(&space->device()),offset,data));

	if (!state->m_current_chr_table)
		fatalerror("No Characteriser Table @ %04x\n", cpu_get_previouspc(&space->device()));

	for (x = state->m_prot_col; x < 64; x++)
	{
		if (call == 0)
		{
			state->m_prot_col = 0;
		}
		else
		{
			if	(state->m_current_chr_table[(x)].call == call)
			{
				state->m_prot_col = x;
				LOG_CHR(("Characteriser find column %02X\n",state->m_prot_col));
				break;
			}
		}
	}
}


static READ16_HANDLER( characteriser16_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	LOG_CHR_FULL(("%04x Characteriser read offset %02X,data %02X", cpu_get_previouspc(&space->device()),offset,state->m_current_chr_table[state->m_prot_col].response));
	LOG_CHR(("Characteriser read offset %02X \n",offset));
	LOG_CHR(("Characteriser read data %02X \n",state->m_current_chr_table[state->m_prot_col].response));

	if (!state->m_current_chr_table)
		fatalerror("No Characteriser Table @ %04x\n", cpu_get_previouspc(&space->device()));


	/* hack for 'invalid questions' error on time machine.. I guess it wants them to decode properly for startup check? */
	if (cpu_get_previouspc(&space->device())==0x283a)
	{
		return 0x00;
	}

	return state->m_current_chr_table[state->m_prot_col].response;
}


/*
BwB Characteriser (CHR)

The BwB method of protection is considerably different to the Barcrest one, with any
incorrect behaviour manifesting in ridiculously large payouts.

In fact, the software seems deliberately designed to mislead, but is (fortunately for
us) prone to similar weaknesses that allow a per game solution.

See MPU4.c for more info

As BwB games aren't yet booting on the 16-bit board, we're duplicating the 8-bit program
Precedent suggests this is not that dangerous an assumption to make.
*/


static WRITE16_HANDLER( bwb_characteriser16_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	int x;
	int call=data &0xff;
	LOG_CHR_FULL(("%04x Characteriser write offset %02X data %02X \n", cpu_get_previouspc(&space->device()),offset,data));
	if (!state->m_current_chr_table)
		fatalerror("No Characteriser Table @ %04x\n", cpu_get_previouspc(&space->device()));

	if (offset == 0)
	{
		if (!state->m_chr_state)
		{
			state->m_chr_state=1;
			state->m_chr_counter=0;
		}
		if (call == 0)
		{
			state->m_init_col ++;
		}
		else
		{
			state->m_init_col =0;
		}
	}
	state->m_chr_value = space->machine().rand();
	for (x = 0; x < 4; x++)
	{
		if	(state->m_current_chr_table[(x)].call == call)
		{
			if (x == 0) // reinit
			{
				state->m_bwb_return = 0;
			}
			state->m_chr_value = bwb_chr_table_common[(state->m_bwb_return)];
			state->m_bwb_return++;
			break;
		}
	}
}

static READ16_HANDLER( bwb_characteriser16_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();

	LOG_CHR(("Characteriser read offset %02X \n",offset));


	if (offset ==0)
	{
		switch (state->m_chr_counter)
		{
			case 6:
			case 13:
			case 20:
			case 27:
			case 34:
			{
				return state->m_bwb_chr_table1[(((state->m_chr_counter + 1) / 7) - 1)].response;
				break;
			}
			default:
			{
				if (state->m_chr_counter > 34)
				{
					state->m_chr_counter = 35;
					state->m_chr_state = 2;
				}
				state->m_chr_counter ++;
				return state->m_chr_value;
			}
		}
	}
	else
	{
		return state->m_chr_value;
	}
}

static const mpu4_chr_table adders_data[64] = {
	{0x00, 0x00}, {0x1A, 0x8C}, {0x04, 0x64}, {0x10, 0x84}, {0x18, 0x84}, {0x0F, 0xC4}, {0x13, 0x84}, {0x1B, 0x84},
	{0x03, 0x9C}, {0x07, 0xF4}, {0x17, 0x04}, {0x1D, 0xCC}, {0x36, 0x24}, {0x35, 0x84}, {0x2B, 0xC4}, {0x28, 0x94},
	{0x39, 0x54}, {0x21, 0x0C}, {0x22, 0x74}, {0x25, 0x0C}, {0x2C, 0x34}, {0x29, 0x04}, {0x31, 0x84}, {0x34, 0x84},
	{0x0A, 0xC4}, {0x1F, 0x84}, {0x06, 0x9C}, {0x0E, 0xE4}, {0x1C, 0x84}, {0x12, 0x84}, {0x1E, 0x84}, {0x0D, 0xD4},
	{0x14, 0x44}, {0x0A, 0x84}, {0x19, 0xC4}, {0x15, 0x84}, {0x06, 0x9C}, {0x0F, 0xE4}, {0x08, 0x84}, {0x1B, 0x84},
	{0x1E, 0x84}, {0x04, 0x8C}, {0x01, 0x60}, {0x0C, 0x84}, {0x18, 0x84}, {0x1A, 0x84}, {0x11, 0x84}, {0x0B, 0xC4},
	{0x03, 0x9C}, {0x17, 0xF4}, {0x10, 0x04}, {0x1D, 0xCC}, {0x0E, 0x24}, {0x07, 0x9C}, {0x12, 0xF4}, {0x09, 0x04},
	{0x0D, 0x94}, {0x1F, 0x14}, {0x16, 0x44}, {0x05, 0x8C}, {0x13, 0x34}, {0x1C, 0x04}, {0x02, 0x9C}, {0x00, 0x00}
};

static const mpu4_chr_table crmaze_data[64] = {
	{0x00, 0x00}, {0x1A, 0x34}, {0x04, 0x14}, {0x10, 0x0C}, {0x18, 0x54}, {0x0F, 0x04}, {0x13, 0x24}, {0x1B, 0x34},
	{0x03, 0x94}, {0x07, 0x94}, {0x17, 0x0C}, {0x1D, 0x5C}, {0x36, 0x6C}, {0x35, 0x44}, {0x2B, 0x24}, {0x28, 0x24},
	{0x39, 0x3C}, {0x21, 0x6C}, {0x22, 0xCC}, {0x25, 0x4C}, {0x2C, 0xC4}, {0x29, 0xA4}, {0x31, 0x24}, {0x34, 0x24},
	{0x0A, 0x34}, {0x1F, 0x84}, {0x06, 0xB4}, {0x0E, 0x1C}, {0x1C, 0x64}, {0x12, 0x24}, {0x1E, 0x34}, {0x0D, 0x04},
	{0x14, 0x24}, {0x0A, 0x34}, {0x19, 0x8C}, {0x15, 0xC4}, {0x06, 0xB4}, {0x0F, 0x1C}, {0x08, 0xE4}, {0x1B, 0x24},
	{0x1E, 0x34}, {0x04, 0x14}, {0x01, 0x10}, {0x0C, 0x84}, {0x18, 0x24}, {0x1A, 0x34}, {0x11, 0x04}, {0x0B, 0x24},
	{0x03, 0xB4}, {0x17, 0x04}, {0x10, 0x24}, {0x1D, 0x3C}, {0x0E, 0x74}, {0x07, 0x94}, {0x12, 0x0C}, {0x09, 0xC4},
	{0x0D, 0xA4}, {0x1F, 0x24}, {0x16, 0x24}, {0x05, 0x34}, {0x13, 0x04}, {0x1C, 0x34}, {0x02, 0x94}, {0x00, 0x00}
};

static const mpu4_chr_table crmazea_data[64] = {
	{0x00, 0x00}, {0x1A, 0x0C}, {0x04, 0x90}, {0x10, 0xE0}, {0x18, 0xA4}, {0x0F, 0xAC}, {0x13, 0x78}, {0x1B, 0x5C},
	{0x03, 0xDC}, {0x07, 0xD4}, {0x17, 0xA0}, {0x1D, 0xEC}, {0x36, 0x78}, {0x35, 0x54}, {0x2B, 0x48}, {0x28, 0x50},
	{0x39, 0xC8}, {0x21, 0xF8}, {0x22, 0xDC}, {0x25, 0x94}, {0x2C, 0xE0}, {0x29, 0x24}, {0x31, 0x0C}, {0x34, 0xD8},
	{0x0A, 0x5C}, {0x1F, 0xD4}, {0x06, 0x68}, {0x0E, 0x18}, {0x1C, 0x14}, {0x12, 0xC8}, {0x1E, 0x38}, {0x0D, 0x5C},
	{0x14, 0xDC}, {0x0A, 0x5C}, {0x19, 0xDC}, {0x15, 0xD4}, {0x06, 0x68}, {0x0F, 0x18}, {0x08, 0xD4}, {0x1B, 0x60},
	{0x1E, 0x0C}, {0x04, 0x90}, {0x01, 0xE8}, {0x0C, 0xF8}, {0x18, 0xD4}, {0x1A, 0x60}, {0x11, 0x44}, {0x0B, 0x4C},
	{0x03, 0xD8}, {0x17, 0xD4}, {0x10, 0xE8}, {0x1D, 0xF8}, {0x0E, 0x9C}, {0x07, 0xD4}, {0x12, 0xE8}, {0x09, 0x30},
	{0x0D, 0x48}, {0x1F, 0xD8}, {0x16, 0xDC}, {0x05, 0x94}, {0x13, 0xE8}, {0x1C, 0x38}, {0x02, 0xDC}, {0x00, 0x00}
};

static const mpu4_chr_table crmaze2_data[64] = {
	{0x00, 0x00}, {0x1A, 0x88}, {0x04, 0x54}, {0x10, 0x40}, {0x18, 0x88}, {0x0F, 0x54}, {0x13, 0x40}, {0x1B, 0x88},
	{0x03, 0x74}, {0x07, 0x28}, {0x17, 0x30}, {0x1D, 0x60}, {0x36, 0x80}, {0x35, 0x84}, {0x2B, 0xC4}, {0x28, 0xA4},
	{0x39, 0xC4}, {0x21, 0x8C}, {0x22, 0x74}, {0x25, 0x08}, {0x2C, 0x30}, {0x29, 0x00}, {0x31, 0x80}, {0x34, 0x84},
	{0x0A, 0xC4}, {0x1F, 0x84}, {0x06, 0xAC}, {0x0E, 0x5C}, {0x1C, 0x90}, {0x12, 0x44}, {0x1E, 0x88}, {0x0D, 0x74},
	{0x14, 0x00}, {0x0A, 0x80}, {0x19, 0xC4}, {0x15, 0x84}, {0x06, 0xAC}, {0x0F, 0x5C}, {0x08, 0xB0}, {0x1B, 0x24},
	{0x1E, 0x88}, {0x04, 0x54}, {0x01, 0x08}, {0x0C, 0x30}, {0x18, 0x00}, {0x1A, 0x88}, {0x11, 0x34}, {0x0B, 0x08},
	{0x03, 0x70}, {0x17, 0x00}, {0x10, 0x80}, {0x1D, 0xC4}, {0x0E, 0x84}, {0x07, 0xAC}, {0x12, 0x34}, {0x09, 0x00},
	{0x0D, 0xA0}, {0x1F, 0x84}, {0x16, 0x84}, {0x05, 0x8C}, {0x13, 0x34}, {0x1C, 0x00}, {0x02, 0xA8}, {0x00, 0x00}
};

static const mpu4_chr_table crmaze3_data[64] = {
	{0x00, 0x00}, {0x1A, 0x84}, {0x04, 0x94}, {0x10, 0x3C}, {0x18, 0xEC}, {0x0F, 0x5C}, {0x13, 0xEC}, {0x1B, 0x50},
	{0x03, 0x2C}, {0x07, 0x68}, {0x17, 0x60}, {0x1D, 0xAC}, {0x36, 0x74}, {0x35, 0x00}, {0x2B, 0xAC}, {0x28, 0x58},
	{0x39, 0xEC}, {0x21, 0x7C}, {0x22, 0xEC}, {0x25, 0x58}, {0x2C, 0xE0}, {0x29, 0x90}, {0x31, 0x18}, {0x34, 0xEC},
	{0x0A, 0x54}, {0x1F, 0x28}, {0x06, 0x68}, {0x0E, 0x44}, {0x1C, 0x84}, {0x12, 0xB4}, {0x1E, 0x10}, {0x0D, 0x20},
	{0x14, 0x84}, {0x0A, 0xBC}, {0x19, 0xE8}, {0x15, 0x70}, {0x06, 0x24}, {0x0F, 0x84}, {0x08, 0xB8}, {0x1B, 0xE0},
	{0x1E, 0x94}, {0x04, 0x14}, {0x01, 0x2C}, {0x0C, 0x64}, {0x18, 0x8C}, {0x1A, 0x50}, {0x11, 0x28}, {0x0B, 0x4C},
	{0x03, 0x6C}, {0x17, 0x60}, {0x10, 0xA0}, {0x1D, 0xBC}, {0x0E, 0xCC}, {0x07, 0x78}, {0x12, 0xE8}, {0x09, 0x50},
	{0x0D, 0x20}, {0x1F, 0xAC}, {0x16, 0x74}, {0x05, 0x04}, {0x13, 0xA4}, {0x1C, 0x94}, {0x02, 0x3C}, {0x00, 0x00}
};

static const mpu4_chr_table crmaze3a_data[64] = {
	{0x00, 0x00}, {0x1A, 0x0C}, {0x04, 0x60}, {0x10, 0x84}, {0x18, 0x34}, {0x0F, 0x08}, {0x13, 0xC0}, {0x1B, 0x14},
	{0x03, 0xA8}, {0x07, 0xF0}, {0x17, 0x10}, {0x1D, 0xA0}, {0x36, 0x1C}, {0x35, 0xE4}, {0x2B, 0x1C}, {0x28, 0xE4},
	{0x39, 0x34}, {0x21, 0xA8}, {0x22, 0xF8}, {0x25, 0x64}, {0x2C, 0x8C}, {0x29, 0xF0}, {0x31, 0x30}, {0x34, 0x08},
	{0x0A, 0xE8}, {0x1F, 0xF8}, {0x06, 0xE4}, {0x0E, 0x3C}, {0x1C, 0x44}, {0x12, 0x8C}, {0x1E, 0x58}, {0x0D, 0xC4},
	{0x14, 0x3C}, {0x0A, 0x6C}, {0x19, 0x68}, {0x15, 0xC0}, {0x06, 0x9C}, {0x0F, 0x64}, {0x08, 0x04}, {0x1B, 0x0C},
	{0x1E, 0x48}, {0x04, 0x60}, {0x01, 0xAC}, {0x0C, 0xF8}, {0x18, 0xE4}, {0x1A, 0x14}, {0x11, 0xA8}, {0x0B, 0x78},
	{0x03, 0xEC}, {0x17, 0xD0}, {0x10, 0xB0}, {0x1D, 0xB0}, {0x0E, 0x38}, {0x07, 0xE4}, {0x12, 0x9C}, {0x09, 0xE4},
	{0x0D, 0xBC}, {0x1F, 0xE4}, {0x16, 0x1C}, {0x05, 0x64}, {0x13, 0x8C}, {0x1C, 0x58}, {0x02, 0xEC}, {0x00, 0x00}
};

static const mpu4_chr_table mating_data[64] = {
	{0x00, 0x00}, {0x1A, 0x18}, {0x04, 0xC8}, {0x10, 0xA4}, {0x18, 0x0C}, {0x0F, 0x80}, {0x13, 0x0C}, {0x1B, 0x90},
	{0x03, 0x34}, {0x07, 0x30}, {0x17, 0x00}, {0x1D, 0x58}, {0x36, 0xC8}, {0x35, 0x84}, {0x2B, 0x4C}, {0x28, 0xA0},
	{0x39, 0x4C}, {0x21, 0xC0}, {0x22, 0x3C}, {0x25, 0xC8}, {0x2C, 0xA4}, {0x29, 0x4C}, {0x31, 0x80}, {0x34, 0x0C},
	{0x0A, 0x80}, {0x1F, 0x0C}, {0x06, 0xE0}, {0x0E, 0x1C}, {0x1C, 0x88}, {0x12, 0xA4}, {0x1E, 0x0C}, {0x0D, 0xA0},
	{0x14, 0x0C}, {0x0A, 0x80}, {0x19, 0x4C}, {0x15, 0xA0}, {0x06, 0x3C}, {0x0F, 0x98}, {0x08, 0xEC}, {0x1B, 0x84},
	{0x1E, 0x0C}, {0x04, 0xC0}, {0x01, 0x1C}, {0x0C, 0xA8}, {0x18, 0x84}, {0x1A, 0x0C}, {0x11, 0xA0}, {0x0B, 0x5C},
	{0x03, 0xE8}, {0x17, 0xA4}, {0x10, 0x0C}, {0x1D, 0xD0}, {0x0E, 0x04}, {0x07, 0x38}, {0x12, 0xA8}, {0x09, 0xC4},
	{0x0D, 0x2C}, {0x1F, 0x90}, {0x16, 0x44}, {0x05, 0x18}, {0x13, 0xE8}, {0x1C, 0x84}, {0x02, 0x3C}, {0x00, 0x00}
};

static const mpu4_chr_table skiltrek_data[64] = {
	{0x00, 0x00}, {0x1A, 0x1C}, {0x04, 0xCC}, {0x10, 0x64}, {0x18, 0x1C}, {0x0F, 0x4C}, {0x13, 0x64}, {0x1B, 0x1C},
	{0x03, 0xEC}, {0x07, 0xE4}, {0x17, 0x0C}, {0x1D, 0xD4}, {0x36, 0x84}, {0x35, 0x0C}, {0x2B, 0x44}, {0x28, 0x2C},
	{0x39, 0xD4}, {0x21, 0x14}, {0x22, 0x34}, {0x25, 0x14}, {0x2C, 0x24}, {0x29, 0x0C}, {0x31, 0x44}, {0x34, 0x0C},
	{0x0A, 0x44}, {0x1F, 0x1C}, {0x06, 0xEC}, {0x0E, 0x54}, {0x1C, 0x04}, {0x12, 0x0C}, {0x1E, 0x54}, {0x0D, 0x24},
	{0x14, 0x0C}, {0x0A, 0x44}, {0x19, 0x9C}, {0x15, 0xEC}, {0x06, 0xE4}, {0x0F, 0x1C}, {0x08, 0x6C}, {0x1B, 0x54},
	{0x1E, 0x04}, {0x04, 0x1C}, {0x01, 0xC8}, {0x0C, 0x64}, {0x18, 0x1C}, {0x1A, 0x4C}, {0x11, 0x64}, {0x0B, 0x1C},
	{0x03, 0xEC}, {0x17, 0x64}, {0x10, 0x0C}, {0x1D, 0xD4}, {0x0E, 0x04}, {0x07, 0x3C}, {0x12, 0x6C}, {0x09, 0x44},
	{0x0D, 0x2C}, {0x1F, 0x54}, {0x16, 0x84}, {0x05, 0x1C}, {0x13, 0xEC}, {0x1C, 0x44}, {0x02, 0x3C}, {0x00, 0x00}
};

static const mpu4_chr_table timemchn_data[64] = {
	{0x00, 0x00}, {0x1A, 0x2C}, {0x04, 0x94}, {0x10, 0x14}, {0x18, 0x04}, {0x0F, 0x0C}, {0x13, 0xC4}, {0x1B, 0x0C},
	{0x03, 0xD4}, {0x07, 0x64}, {0x17, 0x0C}, {0x1D, 0xB4}, {0x36, 0x04}, {0x35, 0x0C}, {0x2B, 0x84}, {0x28, 0x5C},
	{0x39, 0xDC}, {0x21, 0x9C}, {0x22, 0xDC}, {0x25, 0x9C}, {0x2C, 0xDC}, {0x29, 0xCC}, {0x31, 0x84}, {0x34, 0x0C},
	{0x0A, 0x84}, {0x1F, 0x0C}, {0x06, 0xD4}, {0x0E, 0x04}, {0x1C, 0x2C}, {0x12, 0xC4}, {0x1E, 0x0C}, {0x0D, 0xC4},
	{0x14, 0x0C}, {0x0A, 0x84}, {0x19, 0x1C}, {0x15, 0xDC}, {0x06, 0xDC}, {0x0F, 0x8C}, {0x08, 0xD4}, {0x1B, 0x44},
	{0x1E, 0x2C}, {0x04, 0x94}, {0x01, 0x20}, {0x0C, 0x0C}, {0x18, 0xA4}, {0x1A, 0x0C}, {0x11, 0xC4}, {0x0B, 0x0C},
	{0x03, 0xD4}, {0x17, 0x14}, {0x10, 0x14}, {0x1D, 0x54}, {0x0E, 0x04}, {0x07, 0x6C}, {0x12, 0xC4}, {0x09, 0x4C},
	{0x0D, 0xC4}, {0x1F, 0x0C}, {0x16, 0xC4}, {0x05, 0x2C}, {0x13, 0xC4}, {0x1C, 0x0C}, {0x02, 0xD4}, {0x00, 0x00}
};

static const mpu4_chr_table strikeit_data[64] = {
	{0x00, 0x00}, {0x1A, 0xC4}, {0x04, 0xC4}, {0x10, 0x44}, {0x18, 0xC4}, {0x0F, 0x44}, {0x13, 0x44}, {0x1B, 0xC4},
	{0x03, 0xCC}, {0x07, 0x3C}, {0x17, 0x5C}, {0x1D, 0x7C}, {0x36, 0x54}, {0x35, 0x24}, {0x2B, 0xC4}, {0x28, 0x4C},
	{0x39, 0xB4}, {0x21, 0x84}, {0x22, 0xCC}, {0x25, 0x34}, {0x2C, 0x04}, {0x29, 0x4C}, {0x31, 0x14}, {0x34, 0x24},
	{0x0A, 0xC4}, {0x1F, 0x44}, {0x06, 0xCC}, {0x0E, 0x14}, {0x1C, 0x04}, {0x12, 0x44}, {0x1E, 0xC4}, {0x0D, 0x4C},
	{0x14, 0x1C}, {0x0A, 0x54}, {0x19, 0x2C}, {0x15, 0x1C}, {0x06, 0x7C}, {0x0F, 0xD4}, {0x08, 0x0C}, {0x1B, 0x94},
	{0x1E, 0x04}, {0x04, 0xC4}, {0x01, 0xC0}, {0x0C, 0x4C}, {0x18, 0x94}, {0x1A, 0x04}, {0x11, 0x44}, {0x0B, 0x44},
	{0x03, 0xCC}, {0x17, 0x1C}, {0x10, 0x7C}, {0x1D, 0x7C}, {0x0E, 0xD4}, {0x07, 0x8C}, {0x12, 0x1C}, {0x09, 0x5C},
	{0x0D, 0x5C}, {0x1F, 0x5C}, {0x16, 0x7C}, {0x05, 0x74}, {0x13, 0x04}, {0x1C, 0xC4}, {0x02, 0xCC}, {0x00, 0x00}
};

static const mpu4_chr_table turnover_data[64] = {
	{0x00, 0x00}, {0x1A, 0x1C}, {0x04, 0x6C}, {0x10, 0xA4}, {0x18, 0x0C}, {0x0F, 0x24}, {0x13, 0x0C}, {0x1B, 0x34},
	{0x03, 0x94}, {0x07, 0x94}, {0x17, 0x44}, {0x1D, 0x5C}, {0x36, 0x6C}, {0x35, 0x24}, {0x2B, 0x1C}, {0x28, 0xAC},
	{0x39, 0x64}, {0x21, 0x1C}, {0x22, 0xEC}, {0x25, 0x64}, {0x2C, 0x0C}, {0x29, 0xA4}, {0x31, 0x0C}, {0x34, 0x24},
	{0x0A, 0x1C}, {0x1F, 0xAC}, {0x06, 0xE4}, {0x0E, 0x1C}, {0x1C, 0x2C}, {0x12, 0xA4}, {0x1E, 0x0C}, {0x0D, 0xA4},
	{0x14, 0x0C}, {0x0A, 0x24}, {0x19, 0x5C}, {0x15, 0xEC}, {0x06, 0xE4}, {0x0F, 0x1C}, {0x08, 0xAC}, {0x1B, 0x24},
	{0x1E, 0x1C}, {0x04, 0x6C}, {0x01, 0x60}, {0x0C, 0x0C}, {0x18, 0x34}, {0x1A, 0x04}, {0x11, 0x0C}, {0x0B, 0x24},
	{0x03, 0x9C}, {0x17, 0xEC}, {0x10, 0xA4}, {0x1D, 0x4C}, {0x0E, 0x24}, {0x07, 0x9C}, {0x12, 0xEC}, {0x09, 0x24},
	{0x0D, 0x0C}, {0x1F, 0x34}, {0x16, 0x04}, {0x05, 0x1C}, {0x13, 0xEC}, {0x1C, 0x24}, {0x02, 0x9C}, {0x00, 0x00}
};

static const mpu4_chr_table eyesdown_data[64] = {
	{0x00, 0x00}, {0x1A, 0x8C}, {0x04, 0x64}, {0x10, 0x0C}, {0x18, 0xC4}, {0x0F, 0x0C}, {0x13, 0x54}, {0x1B, 0x14},
	{0x03, 0x94}, {0x07, 0x94}, {0x17, 0x24}, {0x1D, 0xAC}, {0x36, 0x44}, {0x35, 0x0C}, {0x2B, 0x44}, {0x28, 0x1C},
	{0x39, 0x7C}, {0x21, 0x6C}, {0x22, 0x74}, {0x25, 0x84}, {0x2C, 0x3C}, {0x29, 0x4C}, {0x31, 0x44}, {0x34, 0x0C},
	{0x0A, 0x44}, {0x1F, 0x8C}, {0x06, 0x74}, {0x0E, 0x84}, {0x1C, 0x0C}, {0x12, 0x54}, {0x1E, 0x04}, {0x0D, 0x1C},
	{0x14, 0x7C}, {0x0A, 0xCC}, {0x19, 0x64}, {0x15, 0x0C}, {0x06, 0x74}, {0x0F, 0x84}, {0x08, 0x3C}, {0x1B, 0x5C},
	{0x1E, 0x4C}, {0x04, 0x64}, {0x01, 0x88}, {0x0C, 0x74}, {0x18, 0x04}, {0x1A, 0x8C}, {0x11, 0x54}, {0x0B, 0x04},
	{0x03, 0x9C}, {0x17, 0x7C}, {0x10, 0x5C}, {0x1D, 0x7C}, {0x0E, 0xCC}, {0x07, 0x74}, {0x12, 0x04}, {0x09, 0x1C},
	{0x0D, 0x5C}, {0x1F, 0x5C}, {0x16, 0x7C}, {0x05, 0x6C}, {0x13, 0x54}, {0x1C, 0x04}, {0x02, 0x9C}, {0x00, 0x00}
};

static const mpu4_chr_table quidgrid_data[64] = {
	{0x00, 0x00}, {0x1A, 0x64}, {0x04, 0x64}, {0x10, 0x24}, {0x18, 0x64}, {0x0F, 0x64}, {0x13, 0x24}, {0x1B, 0x64},
	{0x03, 0x74}, {0x07, 0x54}, {0x17, 0x84}, {0x1D, 0xA4}, {0x36, 0x24}, {0x35, 0x24}, {0x2B, 0x64}, {0x28, 0x24},
	{0x39, 0xE4}, {0x21, 0x64}, {0x22, 0x74}, {0x25, 0x44}, {0x2C, 0x34}, {0x29, 0x04}, {0x31, 0x24}, {0x34, 0x24},
	{0x0A, 0x64}, {0x1F, 0x64}, {0x06, 0x74}, {0x0E, 0x44}, {0x1C, 0x64}, {0x12, 0x24}, {0x1E, 0x64}, {0x0D, 0x24},
	{0x14, 0x24}, {0x0A, 0x64}, {0x19, 0xE4}, {0x15, 0x24}, {0x06, 0x74}, {0x0F, 0x44}, {0x08, 0x34}, {0x1B, 0x14},
	{0x1E, 0x04}, {0x04, 0x64}, {0x01, 0x60}, {0x0C, 0x24}, {0x18, 0x64}, {0x1A, 0x64}, {0x11, 0x24}, {0x0B, 0x64},
	{0x03, 0x74}, {0x17, 0x04}, {0x10, 0x24}, {0x1D, 0xE4}, {0x0E, 0x64}, {0x07, 0x74}, {0x12, 0x04}, {0x09, 0x34},
	{0x0D, 0x04}, {0x1F, 0x64}, {0x16, 0x24}, {0x05, 0x64}, {0x13, 0x24}, {0x1C, 0x64}, {0x02, 0x74}, {0x00, 0x00}
};


static const bwb_chr_table prizeinv_data1[5] = {
//This is all wrong, but without BwB Vid booting,
//I can't find the right values. These should be close though
	{0x67},{0x17},{0x0f},{0x24},{0x3c},
};

static const mpu4_chr_table prizeinv_data[8] = {
{0xEF, 0x02},{0x81, 0x00},{0xCE, 0x00},{0x00, 0x2e},
{0x06, 0x20},{0xC6, 0x0f},{0xF8, 0x24},{0x8E, 0x3c},
};

static DRIVER_INIT (adders)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = adders_data;
}

static DRIVER_INIT (crmaze)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = crmaze_data;
}

static DRIVER_INIT (crmazea)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = crmazea_data;
}

static DRIVER_INIT (crmaze2)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = crmaze2_data;
}

static DRIVER_INIT (crmaze2a)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
}

static DRIVER_INIT (crmaze3)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_reel_mux = FLUTTERBOX;
	state->m_current_chr_table = crmaze3_data;
}

static DRIVER_INIT (crmaze3a)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_reel_mux = FLUTTERBOX;
	state->m_current_chr_table = crmaze3a_data;
}

static DRIVER_INIT (mating)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games

	state->m_current_chr_table = mating_data;
}

static DRIVER_INIT (skiltrek)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = skiltrek_data;
}

static DRIVER_INIT (timemchn)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = timemchn_data;
}

static DRIVER_INIT (strikeit)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = strikeit_data;
}

static DRIVER_INIT (turnover)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = turnover_data;
}

static DRIVER_INIT (eyesdown)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = eyesdown_data;
}

static DRIVER_INIT (quidgrid)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = quidgrid_data;
}

static DRIVER_INIT (prizeinv)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0;//currently no hybrid games
	state->m_current_chr_table = prizeinv_data;
}

ROM_START( dealem )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "zenndlem.u6",	0x8000, 0x8000,  CRC(571e5c05) SHA1(89b4c331407a04eae34bb187b036791e0a671533) )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "zenndlem.u24",	0x0000, 0x10000, CRC(3a1950c4) SHA1(7138346d4e8b3cffbd9751b4d7ebd367b9ad8da9) )    /* text layer */

	ROM_REGION( 0x020, "proms", 0 )
	ROM_LOAD( "zenndlem.u22",	0x000, 0x020, CRC(29988304) SHA1(42f61b8f9e1ee96b65db3b70833eb2f6e7a6ae0a) )

	ROM_REGION( 0x200, "plds", 0 )
	ROM_LOAD( "zenndlem.u10",	0x000, 0x104, CRC(e3103c05) SHA1(91b7be75c5fb37025039ab54b484e46a033969b5) )
ROM_END

/*
   Barcrest released two different games called "The Crystal Maze".
   One is a non-video AWP, and uses only the MPU4 card, and the other SWP is the one we're interested in running
   Some of the dumps available seem to confuse the two, due to an early database not distinguishing
   between MPU4 and MPU4Video, as the latter had not been emulated at all at that stage. */

#define VID_BIOS \
	ROM_LOAD("vid.p1",  0x00000, 0x10000,  CRC(e996bc18) SHA1(49798165640627eb31024319353da04380787b10))

ROM_START( bctvidbs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS
ROM_END

ROM_START( prizeinv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("in220mpu4.p1",  0x00000, 0x04000,  CRC(75ff4b1f) SHA1(a3adaad9a91c30fe6ff42dc2003c34a199b28807) )

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "in2-20p1.1",  0x000000, 0x10000, CRC(f34d9001) SHA1(2bae06f4a5a5510b15b918261ecb0de9e34a6b53) )
	ROM_LOAD16_BYTE( "in2-20p1.2",  0x000001, 0x10000, CRC(1dc931b4) SHA1(c46626183edd52c7938c5edee2395aacb49e0730) )
	ROM_LOAD16_BYTE( "in2-20p1.3",  0x020000, 0x10000, CRC(107aa448) SHA1(7b3d4053aaae3b97136cddefbc9edd5e61713ff7) )
	ROM_LOAD16_BYTE( "in2-20p1.4",  0x020001, 0x10000, CRC(04933278) SHA1(97462aef782f7fe82b60f4bddcad0e6a6b50f3df) )
ROM_END

ROM_START( blox )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("blxv___2.0_0",  0x00000, 0x04000, CRC(b399b85e) SHA1(d36391fee4e3126754d6a0fa5f52fe05bc676930) )

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "blxv___2.0_1",  0x000000, 0x10000, CRC(68134eda) SHA1(b324ae1243164c7be7f5eced7ff93760e2176a4e) )
	ROM_LOAD16_BYTE( "blxv___2.0_2",  0x000001, 0x10000, CRC(6b1f8588) SHA1(e8a443f062555f1ed228e1bfed2031927bcd7015) )
	ROM_LOAD16_BYTE( "blxv___2.0_3",  0x020000, 0x10000, CRC(c62d704b) SHA1(bea0c9519063f1601f70372ccb49fb892fbd6e76) )
	ROM_LOAD16_BYTE( "blxv___2.0_4",  0x020001, 0x10000, CRC(e431471a) SHA1(cf90dc48be3bc5e3c5a8efea5818dbc15fa442e9) )
	ROM_LOAD16_BYTE( "blxv___2.0_5",  0x040001, 0x10000, CRC(98ac6bc7) SHA1(9575014ba21fa4330138a34f53e13d30d312bc8b) )
	ROM_LOAD16_BYTE( "blxv___2.0_6",  0x040001, 0x10000, CRC(a3d92b5b) SHA1(1e7042d5eae4a19a01a3ef7d806c434886dc9f4d) )
ROM_END

ROM_START( bloxd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("blxv_d_2.0_0",  0x00000, 0x04000, CRC(2e0891e1) SHA1(3e8ffd0d41227a8a1e311ca0c0bde7590e06dfbd) )

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "blxv___2.0_1",  0x000000, 0x10000, CRC(68134eda) SHA1(b324ae1243164c7be7f5eced7ff93760e2176a4e) )
	ROM_LOAD16_BYTE( "blxv___2.0_2",  0x000001, 0x10000, CRC(6b1f8588) SHA1(e8a443f062555f1ed228e1bfed2031927bcd7015) )
	ROM_LOAD16_BYTE( "blxv___2.0_3",  0x020000, 0x10000, CRC(c62d704b) SHA1(bea0c9519063f1601f70372ccb49fb892fbd6e76) )
	ROM_LOAD16_BYTE( "blxv___2.0_4",  0x020001, 0x10000, CRC(e431471a) SHA1(cf90dc48be3bc5e3c5a8efea5818dbc15fa442e9) )
	ROM_LOAD16_BYTE( "blxv___2.0_5",  0x040001, 0x10000, CRC(98ac6bc7) SHA1(9575014ba21fa4330138a34f53e13d30d312bc8b) )
	ROM_LOAD16_BYTE( "blxv___2.0_6",  0x040001, 0x10000, CRC(a3d92b5b) SHA1(1e7042d5eae4a19a01a3ef7d806c434886dc9f4d) )
ROM_END

ROM_START( bwbtetrs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("tetris22.p0",  0x00000, 0x04000, CRC(b711c7ae) SHA1(767b17ddf9021fdf79ff6c52f04a5d8ea60cf30e) )

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "tetris22.p1",  0x000000, 0x10000, CRC(e81dd182) SHA1(28b460224abf6fe24b637542ccd1c84040674555) )
	ROM_LOAD16_BYTE( "tetris22.p2",  0x000001, 0x10000, CRC(68aa4f15) SHA1(4e4511a64391fc64e5f5b7ccb46a78fd2e1d94d6) )
	ROM_LOAD16_BYTE( "tetris22.p3",  0x020000, 0x10000, CRC(b38b4763) SHA1(d28e77fdd6869cb5b5ec40ed1f300a2a947e0482) )
	ROM_LOAD16_BYTE( "tetris22.p4",  0x020001, 0x10000, CRC(1649f604) SHA1(ca4ac303391a0969d41c8f988b8e81cfcee1a21c) )
	ROM_LOAD16_BYTE( "tetris22.p5",  0x040001, 0x10000, CRC(02859676) SHA1(5293c767021a6b5253eecab0b0568aa082ea7084) )
	ROM_LOAD16_BYTE( "tetris22.p6",  0x040001, 0x10000, CRC(40d24c82) SHA1(7ac3cf148af84ad93eaf11ce3420abbe45d986e2) )
ROM_END

/* Vegas Poker Prototype dumped by HIGHWAYMAN */
ROM_START( vgpoker )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("comms-v2.0.bin",  0x00000, 0x10000,  CRC(1717581f) SHA1(40f8cae39a2ab0c89d2bbfd8a37725aaae229c96))

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE("video-1.bin",  0x000000, 0x010000,  CRC(7ba2396c) SHA1(eb24b802b984315fc2eba4f15c208e2c1925c1c8))
	ROM_LOAD16_BYTE("video-2.bin",  0x000001, 0x010000,  CRC(4f9e830b) SHA1(f17bebb289c3620bf4c88b2b358a9dab87ac214f))
	ROM_LOAD16_BYTE("video-3.bin",  0x020000, 0x010000,  CRC(607e0baa) SHA1(9f64a46ef3b9a854e939b5e7f0d1e6e925735922))
	ROM_LOAD16_BYTE("video-4.bin",  0x020001, 0x010000,  CRC(2019f5d3) SHA1(d183b3b92d03be9f9d57b5df1a621cbfe955ed93))
	ROM_LOAD16_BYTE("video-5.bin",  0x040000, 0x010000,  CRC(c029202e) SHA1(b08bb2678c2ff62a58ef67d5440c326d0fadc34e))
	ROM_LOAD16_BYTE("video-6.bin",  0x040001, 0x010000,  CRC(3287ae4e) SHA1(3b05a036de3ca7ec644bfbf04934e44e631d1e28))
	ROM_LOAD16_BYTE("video-7.bin",  0x060000, 0x010000,  CRC(231cf163) SHA1(02b28ef0e1661a82d0fba2ecc5474c79651fa9e7))
	ROM_LOAD16_BYTE("video-8.bin",  0x060001, 0x010000,  CRC(076efdc8) SHA1(bef0a1d8f0e7486ee5dc7407ce5c96854cefa5cf))
ROM_END


ROM_START( renoreel )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("rr_27s__.a60",  0x00000, 0x10000,  CRC(44c9ff47) SHA1(93a3155144b233c113aa3b49bd4eb5969e400a68))

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "rr______.a_1",  0x000000, 0x80000,  CRC(ff27d0ba) SHA1(85cce36495f00a05c1806ecde37274212680e466) )
	ROM_LOAD16_BYTE( "rr______.a_2",  0x000001, 0x80000,  CRC(519b9ae1) SHA1(8ccfe8de0f2c85923df81af8cba6f20af43d2fe2) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "renosnda.bin",  0x000000, 0x080000,  CRC(a72a5e1b) SHA1(a0d5338a400345a55484848a7612119405f617b1) )
	ROM_LOAD( "renosndb.bin",  0x080000, 0x080000,  CRC(46e9a32f) SHA1(d45835a82368992597e44b3c5b9d00d8b901e733) )

ROM_END

ROM_START( redhtpkr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("rp_20s__.3_0",  0x00000, 0x10000,  CRC(b7d02d22) SHA1(f9da1c6dde064bc39d0c48a165dac7acde933397))

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE("rp______.3_1",  0x000000, 0x010000,  CRC(b987406d) SHA1(8c4d386570c0c6298b1cabf50295021b3b0cf625))
	ROM_LOAD16_BYTE("rp______.3_2",  0x000001, 0x010000,  CRC(73e3c12e) SHA1(19e3ed7255fa0c3bfa14b6a4b705c0c3e1a237b6))
	ROM_LOAD16_BYTE("rp______.3_3",  0x020000, 0x010000,  CRC(05a30183) SHA1(302f4926073bf7335da7f0b1e6399b64ea9bbae4))
	ROM_LOAD16_BYTE("rp______.3_4",  0x020001, 0x010000,  CRC(6b122765) SHA1(72cd0fda322790bed8cdc7697306ec01efc43789))
	ROM_LOAD16_BYTE("rp______.3_5",  0x040000, 0x010000,  CRC(d9fd05d0) SHA1(330ef58c012b5d5fd018bea54b3ae315b3e45cfd))
	ROM_LOAD16_BYTE("rp______.3_6",  0x040001, 0x010000,  CRC(eeea91ff) SHA1(cc7870a68f62d4dd70c13713a432a61a091821ef))

ROM_END

ROM_START( crmaze )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "crys.p1", 0x000000, 0x80000,  CRC(40fbde50) SHA1(91bd21c0aaffb9c9b89a114affbc485b12ae9bb4) )
	ROM_LOAD16_BYTE( "cry.p2",  0x000001, 0x80000,  CRC(fa7d006f) SHA1(ecc03b4d7a4089feccc53ad05313c35b33e061d7) )
	ROM_LOAD16_BYTE( "cry.p3",  0x100000, 0x80000,  CRC(e8cf8203) SHA1(e9f42e5c18b97807f51284ad2416346578ed73c4) )
	ROM_LOAD16_BYTE( "cry.p4",  0x100001, 0x80000,  CRC(7b036151) SHA1(7b0040c296059b1e1798ddedf0ecb4582d67ee70) )
	ROM_LOAD16_BYTE( "cry.p5",  0x200000, 0x80000,  CRC(48f17b20) SHA1(711c46fcfd86ded8ff7da883188d70560d20e42f) )
	ROM_LOAD16_BYTE( "cry.p6",  0x200001, 0x80000,  CRC(2b3d9a97) SHA1(7468fffd90d840d245a70475b42308f1e48c5017) )
	ROM_LOAD16_BYTE( "cry.p7",  0x300000, 0x80000,  CRC(20f73433) SHA1(593b40ac17591ac312ad41b4d3a5772626137bba) )
	ROM_LOAD16_BYTE( "cry.p8",  0x300001, 0x80000,  CRC(835da1f2) SHA1(f93e075916d370466832871410591570ad7b9f3b) )
	ROM_LOAD16_BYTE( "cry.p9",  0x400000, 0x80000,  CRC(c0e442ee) SHA1(a3877b200538642fe2bc96cfe8b33f04d8a82a98) )
	ROM_LOAD16_BYTE( "cry.p10", 0x400001, 0x80000,  CRC(500172fa) SHA1(d83a37612daa79ba8425fdb28f39b8324b5736b6) )
ROM_END

ROM_START( crmazed )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "cryd.p1", 0x000000, 0x80000,  CRC(b245f661) SHA1(85a8baca8797fea74bc36eea077be56bde8e54a9) )
	ROM_LOAD16_BYTE( "cry.p2",  0x000001, 0x80000,  CRC(fa7d006f) SHA1(ecc03b4d7a4089feccc53ad05313c35b33e061d7) )
	ROM_LOAD16_BYTE( "cry.p3",  0x100000, 0x80000,  CRC(e8cf8203) SHA1(e9f42e5c18b97807f51284ad2416346578ed73c4) )
	ROM_LOAD16_BYTE( "cry.p4",  0x100001, 0x80000,  CRC(7b036151) SHA1(7b0040c296059b1e1798ddedf0ecb4582d67ee70) )
	ROM_LOAD16_BYTE( "cry.p5",  0x200000, 0x80000,  CRC(48f17b20) SHA1(711c46fcfd86ded8ff7da883188d70560d20e42f) )
	ROM_LOAD16_BYTE( "cry.p6",  0x200001, 0x80000,  CRC(2b3d9a97) SHA1(7468fffd90d840d245a70475b42308f1e48c5017) )
	ROM_LOAD16_BYTE( "cry.p7",  0x300000, 0x80000,  CRC(20f73433) SHA1(593b40ac17591ac312ad41b4d3a5772626137bba) )
	ROM_LOAD16_BYTE( "cry.p8",  0x300001, 0x80000,  CRC(835da1f2) SHA1(f93e075916d370466832871410591570ad7b9f3b) )
	ROM_LOAD16_BYTE( "cry.p9",  0x400000, 0x80000,  CRC(c0e442ee) SHA1(a3877b200538642fe2bc96cfe8b33f04d8a82a98) )
	ROM_LOAD16_BYTE( "cry.p10", 0x400001, 0x80000,  CRC(500172fa) SHA1(d83a37612daa79ba8425fdb28f39b8324b5736b6) )
ROM_END

ROM_START( crmazea )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "am1z.p1",  0x000000, 0x80000,  CRC(d2fdda6d) SHA1(96c27dedc3cf1dd478e6169200844d418a276f14) )
	ROM_LOAD16_BYTE( "am1z.p2",  0x000001, 0x80000,  CRC(1637170f) SHA1(fd17a0e7794f01bf4ad7a16b185f87cb060c70ab) )
	ROM_LOAD16_BYTE( "am1g.p1",  0x100000, 0x80000,  CRC(e8cf8203) SHA1(e9f42e5c18b97807f51284ad2416346578ed73c4) )
	ROM_LOAD16_BYTE( "am1g.p2",  0x100001, 0x80000,  CRC(7b036151) SHA1(7b0040c296059b1e1798ddedf0ecb4582d67ee70) )
	ROM_LOAD16_BYTE( "am1g.p3",  0x200000, 0x80000,  CRC(48f17b20) SHA1(711c46fcfd86ded8ff7da883188d70560d20e42f) )
	ROM_LOAD16_BYTE( "am1g.p4",  0x200001, 0x80000,  CRC(2b3d9a97) SHA1(7468fffd90d840d245a70475b42308f1e48c5017) )
	ROM_LOAD16_BYTE( "am1g.p5",  0x300000, 0x80000,  CRC(68286bb1) SHA1(c307e3ad1e0fd92314216c8e554aafa949559452) )
	ROM_LOAD16_BYTE( "am1g.p6",  0x300001, 0x80000,  CRC(a6b498ad) SHA1(117e1a4ec7e2d3c7d530c5a56cbc1d24b0ddc747) )
	ROM_LOAD16_BYTE( "am1g.p7",  0x400000, 0x80000,  CRC(15882699) SHA1(b29a331e51a37554323b91330a7c2b62b33a943a) )
	ROM_LOAD16_BYTE( "am1g.p8",  0x400001, 0x80000,  CRC(6f0f855b) SHA1(ab411d1af0f88049a6c435bafd4b1fa63f5519b1) )
ROM_END

//The New Crystal Maze Featuring Ocean Zone
ROM_START( crmaze2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "cm2s.p1", 0x000000, 0x80000,  CRC(2bcdff56) SHA1(5ae4d4960db032ec9b8d66c53bc151830d009f61) )
	ROM_LOAD16_BYTE( "cm2s.p2", 0x000001, 0x80000,  CRC(92126def) SHA1(531593dee05954000d9836018aeff9460aecbd26) )
	ROM_LOAD16_BYTE( "cm2.p3",  0x100000, 0x80000,  CRC(88324715) SHA1(c6c8de4e5aeda14232ec7b026da389774b3c7bb1) )
	ROM_LOAD16_BYTE( "cm2.p4",  0x100001, 0x80000,  CRC(8d54a81d) SHA1(37753cf8595647aaf8b8267ca177b6744de9c6d4) )
	ROM_LOAD16_BYTE( "cm2.p5",  0x200000, 0x80000,  CRC(5cf8a2bf) SHA1(2514e78e82842fa5c85d26de35637269cd08b21d) )
	ROM_LOAD16_BYTE( "cm2.p6",  0x200001, 0x80000,  CRC(cf793d2d) SHA1(579c759f57fb6bb87aa27c9d5fb684058913dedc) )
	ROM_LOAD16_BYTE( "cm2.p7",  0x300000, 0x80000,  CRC(008aa4b0) SHA1(b4cec6d11abd0e111c295533700595398ff59075) )
	ROM_LOAD16_BYTE( "cm2.p8",  0x300001, 0x80000,  CRC(bac04f5a) SHA1(130721b7abf28dea1f8162705c8bfc5a4bb78152) )
ROM_END

ROM_START( crmaze2d )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "cm2d.p1", 0x000000, 0x80000,  CRC(a150027b) SHA1(f737b0e3b8954c1589a929b7e567cb55df4a3997) )
	ROM_LOAD16_BYTE( "cm2d.p2", 0x000001, 0x80000,  CRC(84ed6bce) SHA1(7a11473e7ec277508952f7ae6cfc7ed28e1b5c99) )
	ROM_LOAD16_BYTE( "cm2.p3",  0x100000, 0x80000,  CRC(88324715) SHA1(c6c8de4e5aeda14232ec7b026da389774b3c7bb1) )
	ROM_LOAD16_BYTE( "cm2.p4",  0x100001, 0x80000,  CRC(8d54a81d) SHA1(37753cf8595647aaf8b8267ca177b6744de9c6d4) )
	ROM_LOAD16_BYTE( "cm2.p5",  0x200000, 0x80000,  CRC(5cf8a2bf) SHA1(2514e78e82842fa5c85d26de35637269cd08b21d) )
	ROM_LOAD16_BYTE( "cm2.p6",  0x200001, 0x80000,  CRC(cf793d2d) SHA1(579c759f57fb6bb87aa27c9d5fb684058913dedc) )
	ROM_LOAD16_BYTE( "cm2.p7",  0x300000, 0x80000,  CRC(008aa4b0) SHA1(b4cec6d11abd0e111c295533700595398ff59075) )
	ROM_LOAD16_BYTE( "cm2.p8",  0x300001, 0x80000,  CRC(bac04f5a) SHA1(130721b7abf28dea1f8162705c8bfc5a4bb78152) )
ROM_END

ROM_START( crmaze2a )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "am2z.p1",  0x000000, 0x80000,  CRC(f27e02f0) SHA1(8637904c201ece4f08ad63b4fd6d06a860fa762f) )
	ROM_LOAD16_BYTE( "am2z.p2",  0x000001, 0x80000,  CRC(4d24f482) SHA1(9e3687db9d0233e56999017f3ed59ec543bce303) )
	ROM_LOAD16_BYTE( "am2g.p1",  0x100000, 0x80000,  CRC(115402db) SHA1(250f2eded1b88a1abf82febb009eadbb90936f8a) )
	ROM_LOAD16_BYTE( "am2g.p2",  0x100001, 0x80000,  CRC(5d804fbb) SHA1(8dc02eb9329f9c29d4bcc9a0315ae96085625d3e) )
	ROM_LOAD16_BYTE( "am2g.p3",  0x200000, 0x80000,  CRC(5ead0c06) SHA1(35d9aefc60e2c391e32f8119a6dc44434d91c09e) )
	ROM_LOAD16_BYTE( "am2g.p4",  0x200001, 0x80000,  CRC(de4fb542) SHA1(4bf8f8f6850fd819d91827d3c474bd488e61e5ac) )
	ROM_LOAD16_BYTE( "am2g.p5",  0x300000, 0x80000,  CRC(80b01ce2) SHA1(4a3a4bcff4bd9affd1a5eeca5781b6af05bbcc16) )
	ROM_LOAD16_BYTE( "am2g.p6",  0x300001, 0x80000,  CRC(3e134ecc) SHA1(1f8cdce62e693eb07c4620b64cc467339c0563de) )
	ROM_LOAD16_BYTE( "am2g.p7",  0x400000, 0x80000,  CRC(6eb36f1d) SHA1(08b9ec184d64bdbdfa61d3e991a3647e74a7756f) )
	ROM_LOAD16_BYTE( "am2g.p8",  0x400001, 0x80000,  CRC(dda353ef) SHA1(56a5b43f0b0bd9dbf348946a5758ebe63eadb8cf) )
ROM_END

//The Crystal Maze Team Challenge
ROM_START( crmaze3 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "cm3.p1",  0x000000, 0x80000,  CRC(2d2edee5) SHA1(0281ec97aaaaf4c7969340bd5995ac1541dbad54) )
	ROM_LOAD16_BYTE( "cm3.p2",  0x000001, 0x80000,  CRC(c223d7b9) SHA1(da9d730716a30d0e93f2a02c1efa7f19457ae010) )
	ROM_LOAD16_BYTE( "cm3.p3",  0x100000, 0x80000,  CRC(2959c77b) SHA1(8de533bfad48ad19a635dddcafa2a0825133b4de) )
	ROM_LOAD16_BYTE( "cm3.p4",  0x100001, 0x80000,  CRC(b7873e9a) SHA1(a71fac883e02d5f49aee0a20f92dbdb00640ce8d) )
	ROM_LOAD16_BYTE( "cm3.p5",  0x200000, 0x80000,  CRC(c8375070) SHA1(da2ba6591d8765f896c40d6526da8e945d02a182) )
	ROM_LOAD16_BYTE( "cm3.p6",  0x200001, 0x80000,  CRC(1ea36938) SHA1(43f62935b21232d23f662e1e124663267edb1283) )
	ROM_LOAD16_BYTE( "cm3.p7",  0x300000, 0x80000,  CRC(9de3802e) SHA1(ec792f115a0708d68046ba0beb314b7e1f1eb422) )
	ROM_LOAD16_BYTE( "cm3.p8",  0x300001, 0x80000,  CRC(1e6e60b0) SHA1(5e71714747073dd89852a84585642388ee440325) )
	ROM_LOAD16_BYTE( "cm3.p9",  0x400000, 0x80000,  CRC(bfba55a7) SHA1(22eb9b1f9fe83d3b424fd521b68e2976a1940df9) )
	ROM_LOAD16_BYTE( "cm3.p10",  0x400001, 0x80000,  CRC(07edda81) SHA1(e94525be03f30e407051992925bb0d693f3d809b) )
ROM_END

ROM_START( crmaze3d )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "cm3d.p1",  0x000000, 0x80000,  CRC(245f00fa) SHA1(ff5ae1a2ae024dfc0b4104360626e4106f4cd36f) )
	ROM_LOAD16_BYTE( "cm3d.p2",  0x000001, 0x80000,  CRC(091adbcb) SHA1(1466b036d06f6335c90426095ad0f60ea958a29d) )
	ROM_LOAD16_BYTE( "cm3.p3",  0x100000, 0x80000,  CRC(2959c77b) SHA1(8de533bfad48ad19a635dddcafa2a0825133b4de) )
	ROM_LOAD16_BYTE( "cm3.p4",  0x100001, 0x80000,  CRC(b7873e9a) SHA1(a71fac883e02d5f49aee0a20f92dbdb00640ce8d) )
	ROM_LOAD16_BYTE( "cm3.p5",  0x200000, 0x80000,  CRC(c8375070) SHA1(da2ba6591d8765f896c40d6526da8e945d02a182) )
	ROM_LOAD16_BYTE( "cm3.p6",  0x200001, 0x80000,  CRC(1ea36938) SHA1(43f62935b21232d23f662e1e124663267edb1283) )
	ROM_LOAD16_BYTE( "cm3.p7",  0x300000, 0x80000,  CRC(9de3802e) SHA1(ec792f115a0708d68046ba0beb314b7e1f1eb422) )
	ROM_LOAD16_BYTE( "cm3.p8",  0x300001, 0x80000,  CRC(1e6e60b0) SHA1(5e71714747073dd89852a84585642388ee440325) )
	ROM_LOAD16_BYTE( "cm3.p9",  0x400000, 0x80000,  CRC(bfba55a7) SHA1(22eb9b1f9fe83d3b424fd521b68e2976a1940df9) )
	ROM_LOAD16_BYTE( "cm3.p10",  0x400001, 0x80000,  CRC(07edda81) SHA1(e94525be03f30e407051992925bb0d693f3d809b) )
ROM_END

ROM_START( crmaze3a )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "am3z.p1",  0x000000, 0x80000,  CRC(9484e656) SHA1(3c35a8ebeddea56d73ce8db4e93c51cdd9546d59) )
	ROM_LOAD16_BYTE( "am3z.p2",  0x000001, 0x80000,  CRC(1865ee80) SHA1(b3ff8e1631d811b8e88664dd84ae82231ce1f5aa) )
	ROM_LOAD16_BYTE( "am3g.p1",  0x100000, 0x80000,  CRC(49fe36af) SHA1(7c39223b07f53ff57a56c3817299734491372170) )
	ROM_LOAD16_BYTE( "am3g.p2",  0x100001, 0x80000,  CRC(b8823cbd) SHA1(206d3b1b2daff1979f97841041661f8407c35d4d) )
	ROM_LOAD16_BYTE( "am3g.p3",  0x200000, 0x80000,  CRC(b1870f17) SHA1(54c6cabb56e4daa4ccf801d5e44b2789b116d562) )
	ROM_LOAD16_BYTE( "am3g.p4",  0x200001, 0x80000,  CRC(c015d446) SHA1(669007e841afeb1084d9062d0a47c159e4c83cc9) )
	ROM_LOAD16_BYTE( "am3g.p5",  0x300000, 0x80000,  CRC(9de3802e) SHA1(ec792f115a0708d68046ba0beb314b7e1f1eb422) )
	ROM_LOAD16_BYTE( "am3g.p6",  0x300001, 0x80000,  CRC(1e6e60b0) SHA1(5e71714747073dd89852a84585642388ee440325) )
	ROM_LOAD16_BYTE( "am3g.p7",  0x400000, 0x80000,  CRC(a4611e29) SHA1(91b164eea5dbdd1129ad12d7af2dbdb3cd68bcec) )
	ROM_LOAD16_BYTE( "am3g.p8",  0x400001, 0x80000,  CRC(1a10c22e) SHA1(8533a5db3922b80b6e9f74e4e432a2b64bc24fc0) )
ROM_END

ROM_START( turnover )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "tos.p1",  0x000000, 0x010000,  CRC(4044dbeb) SHA1(3aa553055c56f14564b1e33e1c1975337e639f70) )
	ROM_LOAD16_BYTE( "to.p2",   0x000001, 0x010000,  CRC(4bc4659a) SHA1(0e282134c4fe4e8c1cc7b16957903179e23c7abc) )
	ROM_LOAD16_BYTE( "to.p3",   0x020000, 0x010000,  CRC(273c7c14) SHA1(71feb555a05a0ff1ec674505cab72d93c9fbdf65) )
	ROM_LOAD16_BYTE( "to.p4",   0x020001, 0x010000,  CRC(83d29546) SHA1(cef90455b9d8a92424fe1aa10f20fd075d0e3091) )
	ROM_LOAD16_BYTE( "to.p5",   0x040000, 0x010000,  CRC(dceac511) SHA1(7a6d65464e23d832943f771c4cf580aabc6f0e44) )
	ROM_LOAD16_BYTE( "to.p6",   0x040001, 0x010000,  CRC(54c6afb7) SHA1(b724b87b6f4e47d220310b38c97be2fa73dcd617) )
	ROM_LOAD16_BYTE( "to.p7",   0x060000, 0x010000,  CRC(acf19542) SHA1(ad46ffb3c2c078a8e3712eff27aa61f0d1a7c059) )
	ROM_LOAD16_BYTE( "to.p8",   0x060001, 0x010000,  CRC(a5ca385d) SHA1(8df26a33ea7f5b577761c6f9d2fa4eaed74661f8) )
	ROM_LOAD16_BYTE( "to.p9",   0x080000, 0x010000,  CRC(6e85fde3) SHA1(14868d58829e13987e66f52e1899c4385987a87b) )
	ROM_LOAD16_BYTE( "to.p10",  0x080001, 0x010000,  CRC(fadd11a2) SHA1(2b2fbb0769ef6035688d495464f3ea3bc8c7c660) )
	ROM_LOAD16_BYTE( "to.p11",  0x0a0000, 0x010000,  CRC(2d72a61a) SHA1(ce455ab6fea452f96a3ad365178e0e5a0b437867) )
	ROM_LOAD16_BYTE( "to.p12",  0x0a0001, 0x010000,  CRC(a14eedb6) SHA1(219b887a334ff28a88ed2e50f0caff4b510cd549) )
	ROM_LOAD16_BYTE( "to.p13",  0x0c0000, 0x010000,  CRC(3f66ef6b) SHA1(60be6d3f8da1f3084db15ac1bb2470e55c0271de) )
	ROM_LOAD16_BYTE( "to.p14",  0x0c0001, 0x010000,  CRC(127ba65d) SHA1(e34dcd19efd31dc712daac940277bb17694ea61a) )
	ROM_LOAD16_BYTE( "to.p15",  0x0e0000, 0x010000,  CRC(ad787e31) SHA1(314ba312adfc71e4b3b2d52355ec692c192b74eb) )
	ROM_LOAD16_BYTE( "to.p16",  0x0e0001, 0x010000,  CRC(e635c942) SHA1(08f8b5fdb738647bc0b49938da05533be42a2d60) )
ROM_END

ROM_START( skiltrek )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "st.p1",  0x000000, 0x010000,  CRC(d9de47a5) SHA1(625bf40780203293fc34cd8cea8278b4b4a52a75) )
	ROM_LOAD16_BYTE( "st.p2",  0x000001, 0x010000,  CRC(b62575c2) SHA1(06d75e8a364750663d329650720021279e195236) )
	ROM_LOAD16_BYTE( "st.p3",  0x020000, 0x010000,  CRC(9506da76) SHA1(6ef28ab8ec1af455be8ecfab20243f0823dca7c1) )
	ROM_LOAD16_BYTE( "st.p4",  0x020001, 0x010000,  CRC(6ab447bc) SHA1(d01c209dbf4d19a6a7f878fa54ff1cb51e7dcba5) )
	ROM_LOAD16_BYTE( "st.q1",  0x040000, 0x010000,  CRC(4faca475) SHA1(69b498c543600b8e37ab0ed1863ba57845648f3c) )
	ROM_LOAD16_BYTE( "st.q2",  0x040001, 0x010000,  CRC(9f2c5938) SHA1(85527c4c0b7a1e66576d56607d89750fab082580) )
	ROM_LOAD16_BYTE( "st.q3",  0x060000, 0x010000,  CRC(6b6cb194) SHA1(aeac5dcc0827c17e758e3e821ae8a78a3a16ddce) )
	ROM_LOAD16_BYTE( "st.q4",  0x060001, 0x010000,  CRC(ec57bc17) SHA1(d9f522739dbb190fb941ca654299bbedbb8fb703) )
	ROM_LOAD16_BYTE( "st.q5",  0x080000, 0x010000,  CRC(7740a88b) SHA1(d9a683d3e0d6c1b4b59520f90f825124b7a61168) )
	ROM_LOAD16_BYTE( "st.q6",  0x080001, 0x010000,  CRC(95e97796) SHA1(f1a8de0ad02aca31f79a4fe8ba5044546163e3c4) )
	ROM_LOAD16_BYTE( "st.q7",  0x0a0000, 0x010000,  CRC(f3b8fe7f) SHA1(52d5be3f8cab419103f4727d0fb9d30f34c8f651) )
	ROM_LOAD16_BYTE( "st.q8",  0x0a0001, 0x010000,  CRC(b85e75a2) SHA1(b7b03b090c0ec6d92e9a25abb7fec0507356bdfc) )
	ROM_LOAD16_BYTE( "st.q9",  0x0c0000, 0x010000,  CRC(835f6001) SHA1(2cd9084c102d74bcb578c8ea22bbc9ea58f0ceab) )
	ROM_LOAD16_BYTE( "st.qa",  0x0c0001, 0x010000,  CRC(3fc62a0e) SHA1(0628de4b962d3fcca3757cd4e89b3005c9bfd218) )
ROM_END

ROM_START( timemchn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "tm20.p1",  0x000000, 0x010000,  CRC(6919697c) SHA1(786d7b9ab218dbf54ff839d1f83580c409c725b3) )
	ROM_LOAD16_BYTE( "tm20.p2",  0x000001, 0x010000,  CRC(d13b56e4) SHA1(623e73995da93c07b51ce0a5843dba1f853529dd) )
	ROM_LOAD16_BYTE( "tm20.p3",  0x020000, 0x010000,  CRC(efd3ae64) SHA1(9d2a3b65048e04842205751c6921d2550f38bd52) )
	ROM_LOAD16_BYTE( "tm20.p4",  0x020001, 0x010000,  CRC(602ba3fb) SHA1(7243f58df9a26adfd1a149a1e60630b187787dd0) )
	ROM_LOAD16_BYTE( "q12.p5" ,  0x040000, 0x010000,  CRC(adddd8a7) SHA1(73a8dd191eda2f4b41b79d4b55723731953b8970) )
	ROM_LOAD16_BYTE( "q11.p6" ,  0x040001, 0x010000,  CRC(e8ed736f) SHA1(e7068c550aa39a6e8f1692a16794147e996D36b4) )
	ROM_LOAD16_BYTE( "q14.p7" ,  0x060000, 0x010000,  CRC(02abb026) SHA1(42224678e5913090c91c21672661beb8e27127a8) )
	ROM_LOAD16_BYTE( "q13.p8" ,  0x060001, 0x010000,  CRC(3de147dd) SHA1(d2111d54d1604fe2da0133102bbfee706f8f542e) )
	ROM_LOAD16_BYTE( "q16.p9" ,  0x080000, 0x010000,  CRC(ce2bf15e) SHA1(29c7f2e718bce415b0b8dc6d902bf74dad6b1ef4) )
	ROM_LOAD16_BYTE( "q15.p10",  0x080001, 0x010000,  CRC(7894ac8b) SHA1(dc46bd108ac4f67a9062bb7ace91aa51f069cbc8) )
	ROM_LOAD16_BYTE( "q18.p11",  0x0a0000, 0x010000,  CRC(27de90b3) SHA1(625c98e555f7b627ea96653926b8917996a2fdb7) )
	ROM_LOAD16_BYTE( "q17.p12",  0x0a0001, 0x010000,  CRC(5cab773e) SHA1(59a235c51a975b341bdbb88e909729507408f75b) )
	ROM_LOAD16_BYTE( "q20.p13",  0x0c0000, 0x010000,  CRC(083f6c65) SHA1(291ad39ee5f8eba9da293d9206b1f6a6d852f9bd) )
	ROM_LOAD16_BYTE( "q19.p14",  0x0c0001, 0x010000,  CRC(73747644) SHA1(ae252fc95c069a3c82e155220fbfcb74dd43bf89) )
ROM_END

ROM_START( mating )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "mats.p1",  0x000000, 0x080000,  CRC(ebc7ea7e) SHA1(11015489a803ba5c8dbdafd632424bbd6080aece) )
	ROM_LOAD16_BYTE( "mats.p2",  0x000001, 0x080000,  CRC(a4c7e9b4) SHA1(30148c0257181bb88159e02d2b7cd79995ee84a7) )
	ROM_LOAD16_BYTE( "matg.p3",  0x100000, 0x080000,  CRC(571f4e8e) SHA1(51babacb5d9fb1cc9e1e56a3b2a355597d04f178) )
	ROM_LOAD16_BYTE( "matg.p4",  0x100001, 0x080000,  CRC(52d8657b) SHA1(e44e1db13c4abd4fedcd72df9dce1df594f74e44) )
	ROM_LOAD16_BYTE( "matg.p5",  0x200000, 0x080000,  CRC(9f0c9552) SHA1(8b1197f20853e18841a8f64fd5ff58cdd0bd1dbd) )
	ROM_LOAD16_BYTE( "matg.p6",  0x200001, 0x080000,  CRC(59f2b6a8) SHA1(4921cf1fc4c3bc50d2598b63726f61f68b41658c) )
	ROM_LOAD16_BYTE( "matg.p7",  0x300000, 0x080000,  CRC(64c0031a) SHA1(a519addd5d8f4696967ec84c163d28cb81ff9f32) )
	ROM_LOAD16_BYTE( "matg.p8",  0x300001, 0x080000,  CRC(22370dae) SHA1(72b1686b458750b5ee9dfe5599c308329d2c79d5) )
	ROM_LOAD16_BYTE( "matq.p9",  0x400000, 0x040000,  CRC(2d42e982) SHA1(80e476d5d65662059daa93a2fd383aecb74903c1) )
	ROM_LOAD16_BYTE( "matq.p10", 0x400001, 0x040000,  CRC(90364c3c) SHA1(6a4d2a3dd2cf9040887503888e6f38341578ad64) )

	/* Mating Game has an extra OKI sound chip */
	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "matsnd.p1",  0x000000, 0x080000,  CRC(f16df9e3) SHA1(fd9b82d73e18e635a9ea4aabd8c0b4aa2c8c6fdb) )
	ROM_LOAD( "matsnd.p2",  0x080000, 0x080000,  CRC(0c041621) SHA1(9156bf17ef6652968d9fbdc0b2bde64d3a67459c) )
	ROM_LOAD( "matsnd.p3",  0x100000, 0x080000,  CRC(c7435af9) SHA1(bd6080afaaaecca0d65e6d4125b46849aa4d1f33) )
	ROM_LOAD( "matsnd.p4",  0x180000, 0x080000,  CRC(d7e65c5b) SHA1(5575fb9f948158f2e94c986bf4bca9c9ee66a489) )
ROM_END

ROM_START( matingd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "matd.p1",  0x000000, 0x080000,  CRC(e660909f) SHA1(0acd990264fd7faf1f91a796d2438e8c2c7b83d1) )
	ROM_LOAD16_BYTE( "matd.p2",  0x000001, 0x080000,  CRC(a4c7e9b4) SHA1(30148c0257181bb88159e02d2b7cd79995ee84a7) )
	ROM_LOAD16_BYTE( "matg.p3",  0x100000, 0x080000,  CRC(571f4e8e) SHA1(51babacb5d9fb1cc9e1e56a3b2a355597d04f178) )
	ROM_LOAD16_BYTE( "matg.p4",  0x100001, 0x080000,  CRC(52d8657b) SHA1(e44e1db13c4abd4fedcd72df9dce1df594f74e44) )
	ROM_LOAD16_BYTE( "matg.p5",  0x200000, 0x080000,  CRC(9f0c9552) SHA1(8b1197f20853e18841a8f64fd5ff58cdd0bd1dbd) )
	ROM_LOAD16_BYTE( "matg.p6",  0x200001, 0x080000,  CRC(59f2b6a8) SHA1(4921cf1fc4c3bc50d2598b63726f61f68b41658c) )
	ROM_LOAD16_BYTE( "matg.p7",  0x300000, 0x080000,  CRC(64c0031a) SHA1(a519addd5d8f4696967ec84c163d28cb81ff9f32) )
	ROM_LOAD16_BYTE( "matg.p8",  0x300001, 0x080000,  CRC(22370dae) SHA1(72b1686b458750b5ee9dfe5599c308329d2c79d5) )
	ROM_LOAD16_BYTE( "matq.p9",  0x400000, 0x040000,  CRC(2d42e982) SHA1(80e476d5d65662059daa93a2fd383aecb74903c1) )
	ROM_LOAD16_BYTE( "matq.p10", 0x400001, 0x040000,  CRC(90364c3c) SHA1(6a4d2a3dd2cf9040887503888e6f38341578ad64) )

	/* Mating Game has an extra OKI sound chip */
	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "matsnd.p1",  0x000000, 0x080000,  CRC(f16df9e3) SHA1(fd9b82d73e18e635a9ea4aabd8c0b4aa2c8c6fdb) )
	ROM_LOAD( "matsnd.p2",  0x080000, 0x080000,  CRC(0c041621) SHA1(9156bf17ef6652968d9fbdc0b2bde64d3a67459c) )
	ROM_LOAD( "matsnd.p3",  0x100000, 0x080000,  CRC(c7435af9) SHA1(bd6080afaaaecca0d65e6d4125b46849aa4d1f33) )
	ROM_LOAD( "matsnd.p4",  0x180000, 0x080000,  CRC(d7e65c5b) SHA1(5575fb9f948158f2e94c986bf4bca9c9ee66a489) )
ROM_END

ROM_START( adders )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "al.p1",  0x000000, 0x10000,  CRC(97579eb2) SHA1(68f08ab73e0c8044de32f7eea1b3abc1fefff830) )
	ROM_LOAD16_BYTE( "al.p2",  0x000001, 0x10000,  CRC(ca877046) SHA1(7d0e1bb471b7acc89d04953ee5e0b34f64d58325) )
	ROM_LOAD16_BYTE( "al.p3",  0x020000, 0x10000,  CRC(92b5e1c4) SHA1(910126527ddb9d83f5c9c29caf2424ae4ebbc640) )
	ROM_LOAD16_BYTE( "al.p4",  0x020001, 0x10000,  CRC(5e0e3b23) SHA1(3862e7b873a1e2762032151289ce2ad886a8cc3c) )
	ROM_LOAD16_BYTE( "al.q1",  0x040000, 0x10000,  CRC(b9b50b70) SHA1(1887ab00ee004e3f27902d6880fa31277a981891) )
	ROM_LOAD16_BYTE( "al.q2",  0x040001, 0x10000,  CRC(1bed86ac) SHA1(8e6563b5441ad9ddd468a3d9ae906733fed7912a) )
	ROM_LOAD16_BYTE( "al.q3",  0x060000, 0x10000,  CRC(294d8f28) SHA1(9f9aca491ba6c4dc5cfb91da867990a9610c3a28) )
	ROM_LOAD16_BYTE( "al.q4",  0x060001, 0x10000,  CRC(aa3e9fbd) SHA1(59d0868f4c8b3f56ca31a11d2e6af83b202bb735) )
	ROM_LOAD16_BYTE( "al.q5",  0x080000, 0x10000,  CRC(503f4193) SHA1(86df379c736598ba59446961bf0666e155164e1d) )
	ROM_LOAD16_BYTE( "al.q6",  0x080001, 0x10000,  CRC(9fd77e52) SHA1(d8fdebb0fd57ab9ea9797dd386168581a45ebc62) )
	ROM_LOAD16_BYTE( "al.q7",  0x0a0000, 0x10000,  CRC(cf3fa7c7) SHA1(fa1edf09c6d3a8b5737474117b0306ef64f7741c) )
	ROM_LOAD16_BYTE( "al.q8",  0x0a0001, 0x10000,  CRC(55058a63) SHA1(cf9edef5264f4301be4ee11f221ab67a5183a603) )
	ROM_LOAD16_BYTE( "al.q9",  0x0c0000, 0x10000,  CRC(22274191) SHA1(9bee5709edcd853e96408f37447c0f5324610903) )
	ROM_LOAD16_BYTE( "al.qa",  0x0c0001, 0x10000,  CRC(1fe98b4d) SHA1(533afeaea42903905f6f1206bba1a023b141bdd9) )
ROM_END


ROM_START( strikeit )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "sils.p1", 0x000000, 0x020000,  CRC(66ed6696) SHA1(a6aa68eb212254213db5573dfb9da1e9e06a8e39) )
	ROM_LOAD16_BYTE( "sil.p2",  0x000001, 0x020000,  CRC(1afc07b7) SHA1(38777d56192b640b003d8dbf4b793cee0c81d9b2) )
	ROM_LOAD16_BYTE( "sil.p3",  0x040000, 0x020000,  CRC(40f5851c) SHA1(0e3dc0dd2a257a955a1f250556d047481ae87269) )
	ROM_LOAD16_BYTE( "sil.p4",  0x040001, 0x020000,  CRC(657e297e) SHA1(306f40376115ca40099a0010650b5edc183a2c57) )
	ROM_LOAD16_BYTE( "sil.p5",  0x080000, 0x020000,  CRC(28bced09) SHA1(7ba5013f1e0f4e921581b23c4a1d4c005a043b66) )
	ROM_LOAD16_BYTE( "sil.p6",  0x080001, 0x020000,  CRC(6f5fc296) SHA1(bd32a937581df6b5a4f08e6ef40c37a2b4278936) )

	ROM_LOAD( "strikeitlucky_questions",  0x0c0000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( strikeitd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "sild.p1", 0x000000, 0x020000,  CRC(6886e7c8) SHA1(9ef0895c9a8df762d17a3ea036bf0c6c2a440582) )
	ROM_LOAD16_BYTE( "sil.p2",  0x000001, 0x020000,  CRC(1afc07b7) SHA1(38777d56192b640b003d8dbf4b793cee0c81d9b2) )
	ROM_LOAD16_BYTE( "sil.p3",  0x040000, 0x020000,  CRC(40f5851c) SHA1(0e3dc0dd2a257a955a1f250556d047481ae87269) )
	ROM_LOAD16_BYTE( "sil.p4",  0x040001, 0x020000,  CRC(657e297e) SHA1(306f40376115ca40099a0010650b5edc183a2c57) )
	ROM_LOAD16_BYTE( "sil.p5",  0x080000, 0x020000,  CRC(28bced09) SHA1(7ba5013f1e0f4e921581b23c4a1d4c005a043b66) )
	ROM_LOAD16_BYTE( "sil.p6",  0x080001, 0x020000,  CRC(6f5fc296) SHA1(bd32a937581df6b5a4f08e6ef40c37a2b4278936) )

	ROM_LOAD( "strikeitlucky_questions",  0x0c0000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( strikeit2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "sil3.p1", 0x000000, 0x020000,  CRC(51c22f94) SHA1(dbdd30e67df4662d13b32a1df6cfabef2cf9471f) )
	ROM_LOAD16_BYTE( "sil.p2",  0x000001, 0x020000,  CRC(1afc07b7) SHA1(38777d56192b640b003d8dbf4b793cee0c81d9b2) )
	ROM_LOAD16_BYTE( "sil.p3",  0x040000, 0x020000,  CRC(40f5851c) SHA1(0e3dc0dd2a257a955a1f250556d047481ae87269) )
	ROM_LOAD16_BYTE( "sil.p4",  0x040001, 0x020000,  CRC(657e297e) SHA1(306f40376115ca40099a0010650b5edc183a2c57) )
	ROM_LOAD16_BYTE( "sil.p5",  0x080000, 0x020000,  CRC(28bced09) SHA1(7ba5013f1e0f4e921581b23c4a1d4c005a043b66) )
	ROM_LOAD16_BYTE( "sil.p6",  0x080001, 0x020000,  CRC(6f5fc296) SHA1(bd32a937581df6b5a4f08e6ef40c37a2b4278936) )

	ROM_LOAD( "strikeitlucky_questions",  0x0c0000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( strikeit2d )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "sil3d.p1", 0x000000, 0x020000,  CRC(82def1b5) SHA1(d3e90e8e4e905a3779889305bb6441907d9169f1) )
	ROM_LOAD16_BYTE( "sil.p2",  0x000001, 0x020000,  CRC(1afc07b7) SHA1(38777d56192b640b003d8dbf4b793cee0c81d9b2) )
	ROM_LOAD16_BYTE( "sil.p3",  0x040000, 0x020000,  CRC(40f5851c) SHA1(0e3dc0dd2a257a955a1f250556d047481ae87269) )
	ROM_LOAD16_BYTE( "sil.p4",  0x040001, 0x020000,  CRC(657e297e) SHA1(306f40376115ca40099a0010650b5edc183a2c57) )
	ROM_LOAD16_BYTE( "sil.p5",  0x080000, 0x020000,  CRC(28bced09) SHA1(7ba5013f1e0f4e921581b23c4a1d4c005a043b66) )
	ROM_LOAD16_BYTE( "sil.p6",  0x080001, 0x020000,  CRC(6f5fc296) SHA1(bd32a937581df6b5a4f08e6ef40c37a2b4278936) )

	ROM_LOAD( "strikeitlucky_questions",  0x0c0000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( eyesdown )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "eds.p1", 0x000000, 0x010000, CRC(bd4f4c5d) SHA1(4ad34d92d45c0b8c70af8454525dd669cab94c5c) )
	ROM_LOAD16_BYTE( "ed.p2",  0x000001, 0x010000, CRC(8804b926) SHA1(cf9d49ca090a435819f2cd2d32dec3a6767f9e10) )
	ROM_LOAD16_BYTE( "ed.p3",  0x020000, 0x010000, CRC(969d2264) SHA1(b55f6881852f81ec9fb2c57c8137c872f4714710) )
	ROM_LOAD16_BYTE( "ed.p4",  0x020001, 0x010000, CRC(80d9addd) SHA1(f6359354c928e69a90cdf6d4f514c4992d3fa64c) )

	ROM_LOAD( "eyesdown_questions",  0x040000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( eyesdownd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "edd.p1", 0x000000, 0x010000, CRC(369a145c) SHA1(c54305690798086b4f809f38d1a6454d1d34fd0c) )
	ROM_LOAD16_BYTE( "ed.p2",  0x000001, 0x010000, CRC(8804b926) SHA1(cf9d49ca090a435819f2cd2d32dec3a6767f9e10) )
	ROM_LOAD16_BYTE( "ed.p3",  0x020000, 0x010000, CRC(969d2264) SHA1(b55f6881852f81ec9fb2c57c8137c872f4714710) )
	ROM_LOAD16_BYTE( "ed.p4",  0x020001, 0x010000, CRC(80d9addd) SHA1(f6359354c928e69a90cdf6d4f514c4992d3fa64c) )

	ROM_LOAD( "eyesdown_questions",  0x040000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( quidgrid )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "qgs.p1", 0x000000, 0x010000, CRC(e6d072a9) SHA1(b2d656818c2fb12a26ef8a170152087336c00c66) )
	ROM_LOAD16_BYTE( "qg.p2",  0x000001, 0x010000, CRC(e4d74b11) SHA1(d23ccc57c54823ffaa7869ca824e049dc80f8945) )
	ROM_LOAD16_BYTE( "qg.p3",  0x020000, 0x010000, CRC(5dda7cd8) SHA1(246a801e862990aade98fa358477e53707714b42) )
	ROM_LOAD16_BYTE( "qg.p4",  0x020001, 0x010000, CRC(2106cf5d) SHA1(2073589775139ad92daef05a67afb2c70ece168c) )

	ROM_LOAD( "quidgrid_questions",  0x040000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( quidgridd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "qgd.p1", 0x000000, 0x010000, CRC(c8d37bdf) SHA1(e39a41015dd551fd62efa52faf2c5106bf7d23ac) )
	ROM_LOAD16_BYTE( "qg.p2",  0x000001, 0x010000, CRC(e4d74b11) SHA1(d23ccc57c54823ffaa7869ca824e049dc80f8945) )
	ROM_LOAD16_BYTE( "qg.p3",  0x020000, 0x010000, CRC(5dda7cd8) SHA1(246a801e862990aade98fa358477e53707714b42) )
	ROM_LOAD16_BYTE( "qg.p4",  0x020001, 0x010000, CRC(2106cf5d) SHA1(2073589775139ad92daef05a67afb2c70ece168c) )

	ROM_LOAD( "quidgrid_questions",  0x040000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( quidgrid2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "q2gs.p1", 0x000000, 0x010000, CRC(6d4b92c0) SHA1(05777e0ecc8bf9f3564d8395bb7bfa245498d132) )
	ROM_LOAD16_BYTE( "q2g.p2",  0x000001, 0x010000, CRC(9bf74ae3) SHA1(11e75acad496a9304f612714afbef90e48d1b9cb) )
	ROM_LOAD16_BYTE( "q2g.p3",  0x020000, 0x010000, CRC(78555155) SHA1(e1218fc00f08c19a0cafb203e46044efa617ac16) )
	ROM_LOAD16_BYTE( "q2g.p4",  0x020001, 0x010000, CRC(2a4295b6) SHA1(2aa0b5dbe6b934a7a4c8069c91fd6d85cae02836) )

	ROM_LOAD( "quidgrid_questions",  0x040000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END

ROM_START( quidgrid2d )
	ROM_REGION( 0x10000, "maincpu", 0 )
	VID_BIOS

	ROM_REGION( 0x800000, "video", 0 )
	ROM_LOAD16_BYTE( "q2gd.p1", 0x000000, 0x010000, CRC(26185449) SHA1(921d355020788d53e8ffa83af44223dbe5a2dd5e) )
	ROM_LOAD16_BYTE( "q2g.p2",  0x000001, 0x010000, CRC(9bf74ae3) SHA1(11e75acad496a9304f612714afbef90e48d1b9cb) )
	ROM_LOAD16_BYTE( "q2g.p3",  0x020000, 0x010000, CRC(78555155) SHA1(e1218fc00f08c19a0cafb203e46044efa617ac16) )
	ROM_LOAD16_BYTE( "q2g.p4",  0x020001, 0x010000, CRC(2a4295b6) SHA1(2aa0b5dbe6b934a7a4c8069c91fd6d85cae02836) )

	ROM_LOAD( "quidgrid_questions",  0x040000, 0x020000,  NO_DUMP ) // no dumps of question ROMs for this game..
ROM_END


/*Deal 'Em was a conversion kit designed to make early MPU4 machines into video games by replacing the top glass
and reel assembly with this kit and a supplied monitor.
The real Deal 'Em ran on Summit Coin hardware, and was made by someone else.
A further different release was made in 2000, running on the Barcrest MPU4 Video, rather than this one. */

GAME( 1987,  dealem,    0,        dealem,   dealem,   0,        ROT0, "Zenitone",		"Deal 'Em (MPU4 Conversion Kit, v7.0)",								GAME_IMPERFECT_GRAPHICS )

GAME( 199?,  bctvidbs,  0,        mpu4mod2, mpu4,     0,        ROT0, "Barcrest",		"MPU4 Video Firmware",												GAME_IS_BIOS_ROOT )

/* Complete sets */
/* Standard sets are the most common setups, while Datapak releases use a BACTA datalogger (not emulated) to record more information about the game operation, for security etc.
AMLD versions do not pay out, and instead just feature highscore tables. These were mainly intended for locations unwilling to pay for gaming licenses.
The AMLD versions appear to be a mixture of the original game modules and Team Challenge's scoring system. This would suggest they were all made ~1994. */

GAME( 1993,  crmaze,    bctvidbs, crmaze,   crmaze,   crmaze,   ROT0, "Barcrest",		"The Crystal Maze (v1.3)",											GAME_NOT_WORKING )//SWP 0.9
GAME( 1993,  crmazed,   crmaze,   crmaze,   crmaze,   crmaze,   ROT0, "Barcrest",		"The Crystal Maze (v1.3, Datapak)",									GAME_NOT_WORKING )//SWP 0.9D
GAME( 1993,  crmazea,   crmaze,   crmaze,   crmaze,   crmazea,  ROT0, "Barcrest",		"The Crystal Maze (v0.1, AMLD)",									GAME_NOT_WORKING )//SWP 0.9

GAME( 1993,  crmaze2,   bctvidbs, crmaze,   crmaze,   crmaze2,  ROT0, "Barcrest",		"The New Crystal Maze Featuring Ocean Zone (v2.2)",					GAME_NOT_WORKING )//SWP 1.0
GAME( 1993,  crmaze2d,  crmaze2,  crmaze,   crmaze,   crmaze2,  ROT0, "Barcrest",		"The New Crystal Maze Featuring Ocean Zone (v2.2, Datapak)",				GAME_NOT_WORKING )//SWP 1.0D
GAME( 1993,  crmaze2a,  crmaze2,  crmaze,   crmaze,   crmaze2a, ROT0, "Barcrest",		"The New Crystal Maze Featuring Ocean Zone (v0.1, AMLD)",			GAME_NOT_WORKING )//SWP 1.0 /* unprotected? bootleg? */

GAME( 1994,  crmaze3,   bctvidbs, crmaze,   crmaze,   crmaze3,  ROT0, "Barcrest",		"The Crystal Maze Team Challenge (v0.9)",							GAME_NOT_WORKING )//SWP 0.7
GAME( 1994,  crmaze3d,  crmaze3,  crmaze,   crmaze,   crmaze3,  ROT0, "Barcrest",		"The Crystal Maze Team Challenge (v0.9, Datapak)",					GAME_NOT_WORKING )//SWP 0.7D
GAME( 1994,  crmaze3a,  crmaze3,  crmaze,   crmaze,   crmaze3a, ROT0, "Barcrest",		"The Crystal Maze Team Challenge (v1.2, AMLD)",						GAME_NOT_WORKING )//SWP 0.7

GAME( 199?,  turnover,  bctvidbs, mpu4_vid, turnover, turnover, ROT0, "Barcrest",		"Turnover (v2.3)",													GAME_NOT_WORKING )

GAME( 1990,  skiltrek,  bctvidbs, mpu4_vid, skiltrek, skiltrek, ROT0, "Barcrest",		"Skill Trek (v1.1)",												GAME_NOT_WORKING )

GAME( 1989,  adders,    bctvidbs, mpu4_vid, adders,   adders,   ROT0, "Barcrest",		"Adders and Ladders (v2.0)",										GAME_NOT_WORKING )

GAME( 1989,  timemchn,  bctvidbs, mpu4_vid, skiltrek, timemchn, ROT0, "Barcrest",		"Time Machine (v2.0)",												GAME_NOT_WORKING )

GAME( 1996?,  mating,    bctvidbs, mating,   mating,   mating,   ROT0, "Barcrest",		"The Mating Game (v0.4)",											GAME_NOT_WORKING )//SWP 0.2 /* Using crmaze controls for now, cabinet has trackball */
GAME( 1996?,  matingd,   mating,   mating,   mating,   mating,   ROT0, "Barcrest",		"The Mating Game (v0.4, Datapak)",									GAME_NOT_WORKING )//SWP 0.2D

/* Barquest */
/* Barquest II */
/* Wize Move */

/* Games below are missing question ROMs */
GAME( 199?,  strikeit,  bctvidbs, mpu4_vid, mpu4,     strikeit, ROT0, "Barcrest",		"Strike it Lucky (v0.5)",											GAME_NOT_WORKING )
GAME( 199?,  strikeitd, strikeit, mpu4_vid, mpu4,     strikeit, ROT0, "Barcrest",		"Strike it Lucky (v0.5, Datapak)",									GAME_NOT_WORKING )
GAME( 199?,  strikeit2, strikeit, mpu4_vid, mpu4,     strikeit, ROT0, "Barcrest",		"Strike it Lucky (v0.53)",											GAME_NOT_WORKING )
GAME( 199?,  strikeit2d,strikeit, mpu4_vid, mpu4,     strikeit, ROT0, "Barcrest",		"Strike it Lucky (v0.53, Datapak)",									GAME_NOT_WORKING )

GAME( 199?,  eyesdown,  bctvidbs, mpu4_vid, mpu4,     eyesdown, ROT0, "Barcrest",		"Eyes Down (v1.3)",													GAME_NOT_WORKING )
GAME( 199?,  eyesdownd, eyesdown, mpu4_vid, mpu4,     eyesdown, ROT0, "Barcrest",		"Eyes Down (v1.3, Datapak)",										GAME_NOT_WORKING )

GAME( 199?,  quidgrid,  bctvidbs, mpu4_vid, mpu4,     quidgrid, ROT0, "Barcrest",		"Ten Quid Grid (v1.2)",												GAME_NOT_WORKING )
GAME( 199?,  quidgridd, quidgrid, mpu4_vid, mpu4,     quidgrid, ROT0, "Barcrest",		"Ten Quid Grid (v1.2, Datapak)",									GAME_NOT_WORKING )
GAME( 199?,  quidgrid2, quidgrid, mpu4_vid, mpu4,     quidgrid, ROT0, "Barcrest",		"Ten Quid Grid (v2.4)",												GAME_NOT_WORKING )
GAME( 199?,  quidgrid2d,quidgrid, mpu4_vid, mpu4,     quidgrid, ROT0, "Barcrest",		"Ten Quid Grid (v2.4, Datapak)",									GAME_NOT_WORKING )

/* Games below are newer BwB games and use their own BIOS ROMs and hardware setups*/
GAME( 199?,  vgpoker,   0,        vgpoker,  mpu4,     0,        ROT0, "BwB",			"Vegas Poker (prototype, release 2)",								GAME_NOT_WORKING )
GAME( 199?,  prizeinv,  0,        bwbvid,	mpu4,     prizeinv, ROT0, "BwB",			"Prize Space Invaders (20\" v1.1)",									GAME_NOT_WORKING )
GAME( 199?,  blox,      0,        bwbvid,	mpu4,     0,        ROT0, "BwB",			"Blox (v2.0)",														GAME_NOT_WORKING )
GAME( 199?,  bloxd,     blox,     bwbvid,	mpu4,     0,        ROT0, "BwB",			"Blox (v2.0, Datapak)",												GAME_NOT_WORKING )
GAME( 1996,  renoreel,  0,        bwbvid5,  mpu4,     prizeinv,	ROT0, "BwB",			"Reno Reels (20p/10GBP Cash, release A)",							GAME_NOT_WORKING )
GAME( 199?,  redhtpkr,  0,        bwbvid,   mpu4,     0,	    ROT0, "BwB",			"Red Hot Poker (20p/10GBP Cash, release 3)",						GAME_NOT_WORKING )
GAME( 199?,  bwbtetrs,  0,        bwbvid,   mpu4,     0,	    ROT0, "BwB",			"BwB Tetris v 2.2",													GAME_NOT_WORKING )
