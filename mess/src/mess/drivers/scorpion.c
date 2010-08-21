/***************************************************************************

    NOTE: ****** Specbusy: press N, R, or E to boot *************


        Spectrum/Inves/TK90X etc. memory map:

    CPU:
        0000-3fff ROM
        4000-ffff RAM

        Spectrum 128/+2/+2a/+3 memory map:

        CPU:
                0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
                4000-7fff Banked RAM
                8000-bfff Banked RAM
                c000-ffff Banked RAM

        TS2068 memory map: (Can't have both EXROM and DOCK active)
        The 8K EXROM can be loaded into multiple pages.

    CPU:
                0000-1fff     ROM / EXROM / DOCK (Cartridge)
                2000-3fff     ROM / EXROM / DOCK
                4000-5fff \
                6000-7fff  \
                8000-9fff  |- RAM / EXROM / DOCK
                a000-bfff  |
                c000-dfff  /
                e000-ffff /


Interrupts:

Changes:

29/1/2000   KT -    Implemented initial +3 emulation.
30/1/2000   KT -    Improved input port decoding for reading and therefore
            correct keyboard handling for Spectrum and +3.
31/1/2000   KT -    Implemented buzzer sound for Spectrum and +3.
            Implementation copied from Paul Daniel's Jupiter driver.
            Fixed screen display problems with dirty chars.
            Added support to load .Z80 snapshots. 48k support so far.
13/2/2000   KT -    Added Interface II, Kempston, Fuller and Mikrogen
            joystick support.
17/2/2000   DJR -   Added full key descriptions and Spectrum+ keys.
            Fixed Spectrum +3 keyboard problems.
17/2/2000   KT -    Added tape loading from WAV/Changed from DAC to generic
            speaker code.
18/2/2000   KT -    Added tape saving to WAV.
27/2/2000   KT -    Took DJR's changes and added my changes.
27/2/2000   KT -    Added disk image support to Spectrum +3 driver.
27/2/2000   KT -    Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000   DJR -   Tape handling dipswitch.
26/3/2000   DJR -   Snapshot files are now classifed as snapshots not
            cartridges.
04/4/2000   DJR -   Spectrum 128 / +2 Support.
13/4/2000   DJR -   +4 Support (unofficial 48K hack).
13/4/2000   DJR -   +2a Support (rom also used in +3 models).
13/4/2000   DJR -   TK90X, TK95 and Inves support (48K clones).
21/4/2000   DJR -   TS2068 and TC2048 support (TC2048 Supports extra video
            modes but doesn't have bank switching or sound chip).
09/5/2000   DJR -   Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000   DJR -   Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000   DJR -   Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000   DJR -   Fixed +3 Floppy support
10/2/2001   KT  -   Re-arranged code and split into each model emulated.
            Code is split into 48k, 128k, +3, tc2048 and ts2048
            segments. 128k uses some of the functions in 48k, +3
            uses some functions in 128, and tc2048/ts2048 use some
            of the functions in 48k. The code has been arranged so
            these functions come in some kind of "override" order,
            read functions changed to use  READ8_HANDLER and write
            functions changed to use WRITE8_HANDLER.
            Added Scorpion256 preliminary.
18/6/2001   DJR -   Added support for Interface 2 cartridges.
xx/xx/2001  KS -    TS-2068 sound fixed.
            Added support for DOCK cartridges for TS-2068.
            Added Spectrum 48k Psycho modified rom driver.
            Added UK-2086 driver.
23/12/2001  KS -    48k machines are now able to run code in screen memory.
                Programs which keep their code in screen memory
                like monitors, tape copiers, decrunchers, etc.
                works now.
                Fixed problem with interrupt vector set to 0xffff (much
            more 128k games works now).
                A useful used trick on the Spectrum is to set
                interrupt vector to 0xffff (using the table
                which contain 0xff's) and put a byte 0x18 hex,
                the opcode for JR, at this address. The first
                byte of the ROM is a 0xf3 (DI), so the JR will
                jump to 0xfff4, where a long JP to the actual
                interrupt routine is put. Due to unideal
                bankswitching in MAME this JP were to 0001 what
                causes Spectrum to reset. Fixing this problem
                made much more software runing (i.e. Paperboy).
            Corrected frames per second value for 48k and 128k
            Sincalir machines.
                There are 50.08 frames per second for Spectrum
                48k what gives 69888 cycles for each frame and
                50.021 for Spectrum 128/+2/+2A/+3 what gives
                70908 cycles for each frame.
            Remaped some Spectrum+ keys.
                Presing F3 to reset was seting 0xf7 on keyboard
                input port. Problem occured for snapshots of
                some programms where it was readed as pressing
                key 4 (which is exit in Tapecopy by R. Dannhoefer
                for example).
            Added support to load .SP snapshots.
            Added .BLK tape images support.
                .BLK files are identical to .TAP ones, extension
                is an only difference.
08/03/2002  KS -    #FF port emulation added.
                Arkanoid works now, but is not playable due to
                completly messed timings.

Initialisation values used when determining which model is being emulated:
 48K        Spectrum doesn't use either port.
 128K/+2    Bank switches with port 7ffd only.
 +3/+2a     Bank switches with both ports.

Notes:
 1. No contented memory.
 2. No hi-res colour effects (need contended memory first for accurate timing).
 3. Multiface 1 and Interface 1 not supported.
 4. Horace and the Spiders cartridge doesn't run properly.
 5. Tape images not supported:
    .TZX, .SPC, .ITM, .PAN, .TAP(Warajevo), .VOC, .ZXS.
 6. Snapshot images not supported:
    .ACH, .PRG, .RAW, .SEM, .SIT, .SNX, .ZX, .ZXS, .ZX82.
 7. 128K emulation is not perfect - the 128K machines crash and hang while
    running quite a lot of games.
 8. Disk errors occur on some +3 games.
 9. Video hardware of all machines is timed incorrectly.
10. EXROM and HOME cartridges are not emulated.
11. The TK90X and TK95 roms output 0 to port #df on start up.
12. The purpose of this port is unknown (probably display mode as TS2068) and
    thus is not emulated.

Very detailed infos about the ZX Spectrum +3e can be found at

http://www.z88forever.org.uk/zxplus3e/

*******************************************************************************/

#include "emu.h"
#include "includes/spectrum.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"
#include "formats/tzx_cas.h"
#include "machine/beta.h"
#include "devices/messram.h"

/****************************************************************************************************/
/* Zs Scorpion 256 */

/*
port 7ffd. full compatibility with Zx spectrum 128. digits are:

D0-D2 - number of RAM page to put in C000-FFFF
D3    - switch of address for RAM of screen. 0 - 4000, 1 - c000
D4    - switch of ROM : 0-zx128, 1-zx48
D5    - 1 in this bit will block further output in port 7FFD, until reset.
*/

/*
port 1ffd - additional port for resources of computer.

D0    - block of ROM in 0-3fff. when set to 1 - allows read/write page 0 of RAM
D1    - selects ROM expansion. this rom contains main part of service monitor.
D2    - not used
D3    - used for output in RS-232C
D4    - extended RAM. set to 1 - connects RAM page with number 8-15 in
    C000-FFFF. number of page is given in gidits D0-D2 of port 7FFD
D5    - signal of strobe for interface centronics. to form the strobe has to be
    set to 1.
D6-D7 - not used. ( yet ? )
*/

/* rom 0=zx128, 1=zx48, 2 = service monitor, 3=tr-dos */

static void scorpion_update_memory(running_machine *machine)
{
	spectrum_state *state = machine->driver_data<spectrum_state>();
	UINT8 *messram = messram_get_ptr(machine->device("messram"));
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	state->screen_location = messram + ((state->port_7ffd_data & 8) ? (7<<14) : (5<<14));

	memory_set_bankptr(machine, "bank4", messram + (((state->port_7ffd_data & 0x07) | ((state->port_1ffd_data & 0x10)>>1)) * 0x4000));

	if ((state->port_1ffd_data & 0x01)==0x01)
	{
		memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
		memory_set_bankptr(machine, "bank1", messram+(8<<14));
		logerror("RAM\n");
	}
	else
	{
		if ((state->port_1ffd_data & 0x02)==0x02)
		{
			state->ROMSelection = 2;
		}
		else
		{
			state->ROMSelection = ((state->port_7ffd_data>>4) & 0x01) ? 1 : 0;
		}
		memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
		memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x010000 + (state->ROMSelection<<14));
	}


}

static DIRECT_UPDATE_HANDLER( scorpion_direct )
{
	spectrum_state *state = space->machine->driver_data<spectrum_state>();
	running_device *beta = space->machine->device(BETA_DISK_TAG);
	UINT16 pc = cpu_get_reg(space->machine->device("maincpu"), STATE_GENPCBASE);

	if (betadisk_is_active(beta))
	{
		if (pc >= 0x4000)
		{
			state->ROMSelection = ((state->port_7ffd_data>>4) & 0x01) ? 1 : 0;
			betadisk_disable(beta);
			memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
			memory_set_bankptr(space->machine, "bank1", memory_region(space->machine, "maincpu") + 0x010000 + (state->ROMSelection<<14));
		}
	}
	else if (((pc & 0xff00) == 0x3d00) && (state->ROMSelection==1))
	{
		state->ROMSelection = 3;
		betadisk_enable(beta);
	}
	if((address>=0x0000) && (address<=0x3fff))
	{
		memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
		direct->raw = direct->decrypted =  memory_region(space->machine, "maincpu") + 0x010000 + (state->ROMSelection<<14);
		memory_set_bankptr(space->machine, "bank1", direct->raw);
		return ~0;
	}
	return address;
}

static TIMER_CALLBACK(nmi_check_callback)
{
	spectrum_state *state = machine->driver_data<spectrum_state>();

	if ((input_port_read(machine, "NMI") & 1)==1)
	{
		state->port_1ffd_data |= 0x02;
		scorpion_update_memory(machine);
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
	}
}

static WRITE8_HANDLER(scorpion_port_7ffd_w)
{
	spectrum_state *state = space->machine->driver_data<spectrum_state>();

	/* disable paging */
	if (state->port_7ffd_data & 0x20)
		return;

	/* store new state */
	state->port_7ffd_data = data;

	/* update memory */
	scorpion_update_memory(space->machine);
}

static WRITE8_HANDLER(scorpion_port_1ffd_w)
{
	spectrum_state *state = space->machine->driver_data<spectrum_state>();

	/* if paging not disabled */
	if ((state->port_7ffd_data & 0x20)==0)
	{
		state->port_1ffd_data = data;
		scorpion_update_memory(space->machine);
	}
}

static ADDRESS_MAP_START (scorpion_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x001f, 0x001f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_status_r,betadisk_command_w) AM_MIRROR(0xff00)
	AM_RANGE(0x003f, 0x003f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_track_r,betadisk_track_w) AM_MIRROR(0xff00)
	AM_RANGE(0x005f, 0x005f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_sector_r,betadisk_sector_w) AM_MIRROR(0xff00)
	AM_RANGE(0x007f, 0x007f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_data_r,betadisk_data_w) AM_MIRROR(0xff00)
	AM_RANGE(0x00fe, 0x00fe) AM_READWRITE(spectrum_port_fe_r,spectrum_port_fe_w) AM_MIRROR(0xff00) AM_MASK(0xffff)
	AM_RANGE(0x00ff, 0x00ff) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_state_r, betadisk_param_w) AM_MIRROR(0xff00)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(scorpion_port_7ffd_w)  AM_MIRROR(0x3ffd)
	AM_RANGE(0x8000, 0x8000) AM_DEVWRITE("ay8912", ay8910_data_w) AM_MIRROR(0x3ffd)
	AM_RANGE(0xc000, 0xc000) AM_DEVREADWRITE("ay8912", ay8910_r, ay8910_address_w) AM_MIRROR(0x3ffd)
	AM_RANGE(0x1000, 0x1000) AM_WRITE(scorpion_port_1ffd_w) AM_MIRROR(0x0ffd)
ADDRESS_MAP_END


static MACHINE_RESET( scorpion )
{
	spectrum_state *state = machine->driver_data<spectrum_state>();
	UINT8 *messram = messram_get_ptr(machine->device("messram"));
	running_device *beta = machine->device(BETA_DISK_TAG);
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");

	betadisk_disable(beta);
	betadisk_clear_status(beta);

	memory_set_direct_update_handler(space, scorpion_direct);

	memset(messram,0,256*1024);

	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(machine, "bank2", messram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(machine, "bank3", messram + (2<<14));

	state->port_7ffd_data = 0;
	state->port_1ffd_data = 0;
	scorpion_update_memory(machine);
}
static MACHINE_START( scorpion )
{
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, nmi_check_callback);
}


/* F4 Character Displayer */
static const gfx_layout spectrum_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	96,					/* 96 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout quorum_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	160,					/* 160 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout profi_8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	224,					/* 224 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( scorpion )
	GFXDECODE_ENTRY( "maincpu", 0x17d00, spectrum_charlayout, 0, 8 )
GFXDECODE_END

static GFXDECODE_START( profi )
	GFXDECODE_ENTRY( "maincpu", 0x17d00, spectrum_charlayout, 0, 8 )
	GFXDECODE_ENTRY( "maincpu", 0x1abfc, profi_8_charlayout, 0, 8 )
	/* There are more characters after this, that haven't been decoded */
GFXDECODE_END

static GFXDECODE_START( quorum )
	GFXDECODE_ENTRY( "maincpu", 0x1fb00, quorum_charlayout, 0, 8 )
GFXDECODE_END

static MACHINE_DRIVER_START( scorpion )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_IO_MAP(scorpion_io)

	MDRV_MACHINE_START( scorpion )
	MDRV_MACHINE_RESET( scorpion )
	MDRV_GFXDECODE(scorpion)

	MDRV_BETA_DISK_ADD(BETA_DISK_TAG)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("256K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( profi )
	MDRV_IMPORT_FROM( scorpion )
	MDRV_GFXDECODE(profi)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( quorum )
	MDRV_IMPORT_FROM( scorpion )
	MDRV_GFXDECODE(quorum)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(scorpion)
	ROM_REGION(0x90000, "maincpu", 0)
	ROM_SYSTEM_BIOS(0, "v1", "V.2.92")
	ROMX_LOAD("scorp0.rom", 0x010000, 0x4000, CRC(0eb40a09) SHA1(477114ff0fe1388e0979df1423602b21248164e5), ROM_BIOS(1))
	ROMX_LOAD("scorp1.rom", 0x014000, 0x4000, CRC(9d513013) SHA1(367b5a102fb663beee8e7930b8c4acc219c1f7b3), ROM_BIOS(1))
	ROMX_LOAD("scorp2.rom", 0x018000, 0x4000, CRC(fd0d3ce1) SHA1(07783ee295274d8ff15d935bfd787c8ac1d54900), ROM_BIOS(1))
	ROMX_LOAD("scorp3.rom", 0x01c000, 0x4000, CRC(1fe1d003) SHA1(33703e97cc93b7edfcc0334b64233cf81b7930db), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v2", "V.2.92 joined")
	ROMX_LOAD( "scorpion.rom", 0x010000, 0x10000, CRC(fef73c28) SHA1(66cecdadf992d8adb9c66deee929eb56600dc9bc), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v3", "V.2.94")
	ROMX_LOAD( "scorp294.rom", 0x010000, 0x10000, CRC(99f57ce1) SHA1(083bb57ad52cc871b92d3e1794fd9790872c3584), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "v4", "SMUC V.4.02")
	ROMX_LOAD( "scorp402.rom", 0x010000, 0x20000, CRC(9fcf893d) SHA1(0cc7ba60f5cfc36e75bd3a5c90e26b2a1905a970), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "v5", "ProfROM V.3.9f")
	ROMX_LOAD( "prof_39f.rom", 0x010000, 0x20000, CRC(c55e64da) SHA1(cec7770fe26350f57f6c325a29db78787dc4521e), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "v6", "ProfROM V.4.01")
	ROMX_LOAD( "profrom.rom", 0x010000, 0x80000, CRC(b02d89de) SHA1(4cb85341e2a400e0e88869304d80af430266cdd1), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "v7", "NeOS 256")
	ROMX_LOAD("neos_256.rom", 0x010000, 0x4000, CRC(364ae09a) SHA1(bb6db1947415503a6bc48f33c603fb3a0dbb3690), ROM_BIOS(7))
	ROMX_LOAD("scorp1.rom",   0x014000, 0x4000, CRC(9d513013) SHA1(367b5a102fb663beee8e7930b8c4acc219c1f7b3), ROM_BIOS(7))
	ROMX_LOAD("scorp2.rom",   0x018000, 0x4000, CRC(fd0d3ce1) SHA1(07783ee295274d8ff15d935bfd787c8ac1d54900), ROM_BIOS(7))
	ROMX_LOAD("scorp3.rom",   0x01c000, 0x4000, CRC(1fe1d003) SHA1(33703e97cc93b7edfcc0334b64233cf81b7930db), ROM_BIOS(7))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION(0x01000, "keyboard", 0)
	ROM_LOAD( "scrpkey.rom", 0x0000, 0x1000, CRC(e938a510) SHA1(2753993c97ff0fc6cff26ed792929abc1288dc6f))

	ROM_REGION(0x010000, "gsound", 0)
	ROM_LOAD( "gs104.rom", 0x0000, 0x8000, CRC(7a365ba6) SHA1(c865121306eb3a7d811d82fbcc653b4dc1d6fa3d))
	ROM_LOAD( "gs105a.rom", 0x8000, 0x8000, CRC(1cd490c6) SHA1(1ba78923dc7d803e7995c165992e14e4608c2153))
ROM_END

ROM_START(profi)
	ROM_REGION(0x020000, "maincpu", 0)
	ROM_SYSTEM_BIOS(0, "v1", "ver 1")
	ROMX_LOAD( "profi.rom", 0x010000, 0x10000, CRC(285a0985) SHA1(2b33ab3561e7bc5997da7f0d3a2a906fe7ea960f), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v2", "ver 2")
	ROMX_LOAD( "profi_my.rom", 0x010000, 0x10000, CRC(2ffd6cd9) SHA1(1b74a3251358c5f102bb87654f47b02281e15e9c), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v3", "ver 3")
	ROMX_LOAD( "profi-p1.rom", 0x010000, 0x10000, CRC(537ddb81) SHA1(00a23e8dc722b248d4f98cb14a600ce7487f2b9c), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "v4", "ver 4")
	ROMX_LOAD( "profi_1.rom", 0x010000, 0x10000, CRC(f07fbee8) SHA1(b29c81a94658a4d50274ba953775a49e855534de), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "v5", "T-Rex")
	ROMX_LOAD( "profi-p.rom", 0x010000, 0x10000, CRC(314f6b57) SHA1(1507f53ec64dcf5154b5cfce6922f69f70296a53), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "v6", "JV Kramis V0.2")
	ROMX_LOAD( "profi32.rom", 0x010000, 0x10000, CRC(77327f52) SHA1(019bd00cc7939741d99b99beac6ae1298652e652), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "v7", "Power Of Sound Group")
	ROMX_LOAD( "profi1k.rom", 0x010000, 0x10000, CRC(a932676f) SHA1(907ac56219f325949a7c2fe8168799d9cdd5ba6c), ROM_BIOS(7))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(quorum)
	ROM_REGION(0x020000, "maincpu", 0)
	ROM_LOAD("qu7v42.rom",   0x010000, 0x10000, CRC(e950eee5) SHA1(f8e22672722b0038689c6c8bc4acf5392acc9d8c))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(bestzx)
	ROM_REGION(0x020000, "maincpu", 0)
	ROM_LOAD( "bestzx.rom", 0x010000, 0x10000, CRC(fc7936e8) SHA1(0d6378c51b2f08a3e2b4c75e64c76c15ae5dc76d))
ROM_END

ROM_START( kay1024 )
	ROM_REGION(0x020000, "maincpu", ROMREGION_ERASEFF)
	ROM_SYSTEM_BIOS(0, "v1", "ver 1")
	ROMX_LOAD( "kay98.rom",    0x010000, 0x08000, CRC(7fbf2d43) SHA1(e555f2ed01ecf2231d493bd70a4d79b436e9f10e), ROM_BIOS(1))
	ROMX_LOAD( "trd503.rom",   0x01c000, 0x04000, CRC(10751aba) SHA1(21695e3f2a8f796386ce66eea8a246b0ac44810c), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v2", "Kramis V0.2")
	ROMX_LOAD( "kay1024b.rom", 0x010000, 0x10000, CRC(ab99c31e) SHA1(cfa9e6553aea72956fce4f0130c007981d684734), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v3", "Kramis V0.3")
	ROMX_LOAD( "kay1024s.rom", 0x010000, 0x10000, CRC(67351caa) SHA1(1d9c0606b380c000ca1dfa33f90a122ecf9df1f1), ROM_BIOS(3))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    COMPANY     FULLNAME */
COMP( 1994, scorpion, spec128,	 0,	scorpion,	spec_plus,	0,		"Zonov and Co.",		"Scorpion ZS-256", GAME_NOT_WORKING )
COMP( 1991, profi,    spec128,	 0,	profi,  	spec_plus,	0,		"Kondor and Kramis",		"Profi", GAME_NOT_WORKING )
COMP( 1998, kay1024,  spec128,	 0,	scorpion,	spec_plus,	0,		"NEMO",		"Kay 1024", GAME_NOT_WORKING )
COMP( 19??, quorum,   spec128,	 0,	quorum, 	spec_plus,	0,		"<unknown>",		"Quorum", GAME_NOT_WORKING )
COMP( 19??, bestzx,   spec128,	 0,	scorpion,	spec_plus,	0,		"<unknown>",		"BestZX", GAME_NOT_WORKING )