/***************************************************************************
    zx.c

    video hardware
    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

    The ZX has a very unorthodox video RAM system.  To start a scanline,
    the CPU must jump to video RAM at 0xC000, which is a mirror of the
    RAM at 0x4000.  The video chip (ULA?) pulls a switcharoo and changes
    the video bytes as they are fetched by the CPU.

    The video chip draws the scanline until a HALT instruction (0x76) is
    reached, which indicates no further video RAM for this scanline.  Any
    other video byte is used to generate a tile and at the same time,
    appears to the CPU as a NOP (0x00) instruction.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"
#include "sound/dac.h"

emu_timer *ula_nmi = NULL;
//emu_timer *ula_irq = NULL;
//int ula_nmi_active;
int ula_frame_vsync = 0;
//int ula_scancode_count = 0;
int ula_scanline_count = 0;

/*
 * Toggle the video output between black and white.
 * This happens whenever the ULA scanline IRQs are enabled/disabled.
 * Normally this is done during the synchronized zx_ula_r() function,
 * which outputs 8 pixels per code, but if the video sync is off
 * (during tape IO or sound output) zx_ula_bkgnd() is used to
 * simulate the display of a ZX80/ZX81.
 */
void zx_ula_bkgnd(running_machine &machine, int color)
{
	zx_state *state = machine.driver_data<zx_state>();
	screen_device *screen = machine.first_screen();
	int width = screen->width();
	int height = screen->height();
	const rectangle &visarea = screen->visible_area();

	if (state->m_ula_frame_vsync == 0 && color != state->m_old_c)
	{
		int y, new_x, new_y;
		rectangle r;
		bitmap_t *bitmap = machine.generic.tmpbitmap;

		new_y = machine.primary_screen->vpos();
		new_x = machine.primary_screen->hpos();
/*      logerror("zx_ula_bkgnd: %3d,%3d - %3d,%3d\n", state->m_old_x, state->m_old_y, new_x, new_y);*/
		y = state->m_old_y;
		for (;;)
		{
			if (y == new_y)
			{
				r.min_x = state->m_old_x;
				r.max_x = new_x;
				r.min_y = r.max_y = y;
				bitmap_fill(bitmap, &r, color);
				break;
			}
			else
			{
				r.min_x = state->m_old_x;
				r.max_x = visarea.max_x;
				r.min_y = r.max_y = y;
				bitmap_fill(bitmap, &r, color);
				state->m_old_x = 0;
			}
			if (++y == height)
				y = 0;
		}
		state->m_old_x = (new_x + 1) % width;
		state->m_old_y = new_y;
		state->m_old_c = color;
	}
}

/*
 * PAL:  310 total lines,
 *            0.. 55 vblank
 *           56..247 192 visible lines
 *          248..303 vblank
 *          304...   vsync
 * NTSC: 262 total lines
 *            0.. 31 vblank
 *           32..223 192 visible lines
 *          224..233 vblank
 */
static TIMER_CALLBACK(zx_ula_nmi)
{
	zx_state *state = machine.driver_data<zx_state>();
	/*
     * An NMI is issued on the ZX81 every 64us for the blanked
     * scanlines at the top and bottom of the display.
     */
	screen_device *screen = machine.first_screen();
	int height = screen->height();
	const rectangle r1 = screen->visible_area();
	rectangle r;

	bitmap_t *bitmap = machine.generic.tmpbitmap;
	r.min_x = r1.min_x;
	r.max_x = r1.max_x;
	r.min_y = r.max_y = state->m_ula_scanline_count;
	bitmap_fill(bitmap, &r, 1);
//  logerror("ULA %3d[%d] NMI, R:$%02X, $%04x\n", machine.primary_screen->vpos(), ula_scancode_count, (unsigned) cpu_get_reg(machine.device("maincpu"), Z80_R), (unsigned) cpu_get_reg(machine.device("maincpu"), Z80_PC));
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
	if (++state->m_ula_scanline_count == height)
		state->m_ula_scanline_count = 0;
}

static TIMER_CALLBACK(zx_ula_irq)
{
	zx_state *state = machine.driver_data<zx_state>();

	/*
     * An IRQ is issued on the ZX80/81 whenever the R registers
     * bit 6 goes low. In MESS this IRQ timed from the first read
     * from the copy of the DFILE in the upper 32K in zx_ula_r().
     */
	if (state->m_ula_irq_active)
	{
//      logerror("ULA %3d[%d] IRQ, R:$%02X, $%04x\n", machine.primary_screen->vpos(), ula_scancode_count, (unsigned) cpu_get_reg(machine.device("maincpu"), Z80_R), (unsigned) cpu_get_reg(machine.device("maincpu"), Z80_PC));

		state->m_ula_irq_active = 0;
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
	}
}

void zx_ula_r(running_machine &machine, int offs, const char *region, const UINT8 param)
{
	zx_state *state = machine.driver_data<zx_state>();
	screen_device *screen = machine.first_screen();
	int offs0 = offs & 0x7fff;
	UINT8 *rom = machine.region("maincpu")->base();
	UINT8 chr = rom[offs0];

	if ((!state->m_ula_irq_active) && (chr == 0x76))
	{
		bitmap_t *bitmap = machine.generic.tmpbitmap;
		UINT16 y, *scanline;
		UINT16 ireg = cpu_get_reg(machine.device("maincpu"), Z80_I) << 8;
		UINT8 data, *chrgen, creg;

		if (param)
			creg = cpu_get_reg(machine.device("maincpu"), Z80_B);
		else
			creg = cpu_get_reg(machine.device("maincpu"), Z80_C);

		chrgen = machine.region(region)->base();

		if ((++state->m_ula_scanline_count == screen->height()) || (creg == 32))
		{
			state->m_ula_scanline_count = 0;
			state->m_offs1 = offs0;
		}

		state->m_ula_frame_vsync = 3;

		state->m_charline_ptr = 0;

		for (y = state->m_offs1+1; ((y < offs0) && (state->m_charline_ptr < ARRAY_LENGTH(state->m_charline))); y++)
		{
			state->m_charline[state->m_charline_ptr] = rom[y];
			state->m_charline_ptr++;
		}
		for (y = state->m_charline_ptr; y < ARRAY_LENGTH(state->m_charline); y++)
			state->m_charline[y] = 0;

		machine.scheduler().timer_set(machine.device<cpu_device>("maincpu")->cycles_to_attotime(((32 - state->m_charline_ptr) << 2)), FUNC(zx_ula_irq));
		state->m_ula_irq_active++;

		scanline = BITMAP_ADDR16(bitmap, state->m_ula_scanline_count, 0);
		y = 0;

		for (state->m_charline_ptr = 0; state->m_charline_ptr < ARRAY_LENGTH(state->m_charline); state->m_charline_ptr++)
		{
			chr = state->m_charline[state->m_charline_ptr];
			data = chrgen[ireg | ((chr & 0x3f) << 3) | ((8 - creg)&7) ];
			if (chr & 0x80) data ^= 0xff;

			scanline[y++] = (data >> 7) & 1;
			scanline[y++] = (data >> 6) & 1;
			scanline[y++] = (data >> 5) & 1;
			scanline[y++] = (data >> 4) & 1;
			scanline[y++] = (data >> 3) & 1;
			scanline[y++] = (data >> 2) & 1;
			scanline[y++] = (data >> 1) & 1;
			scanline[y++] = (data >> 0) & 1;
			state->m_charline[state->m_charline_ptr] = 0;
		}

		if (creg == 1) state->m_offs1 = offs0;
	}
}

VIDEO_START( zx )
{
	zx_state *state = machine.driver_data<zx_state>();
	state->m_ula_nmi = machine.scheduler().timer_alloc(FUNC(zx_ula_nmi));
	state->m_ula_irq_active = 0;
	VIDEO_START_CALL(generic_bitmapped);
}

SCREEN_EOF( zx )
{
	zx_state *state = machine.driver_data<zx_state>();
	/* decrement video synchronization counter */
	if (state->m_ula_frame_vsync)
		--state->m_ula_frame_vsync;
}
