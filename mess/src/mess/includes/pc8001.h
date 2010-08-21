#ifndef __PC8001__
#define __PC8001__

#define Z80_TAG			"z80"
#define I8251_TAG		"i8251"
#define I8255A_TAG		"i8255"
#define I8257_TAG		"i8257"
#define UPD1990A_TAG	"upd1990a"
#define UPD3301_TAG		"upd3301"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define SCREEN_TAG		"screen"
#define SPEAKER_TAG		"speaker"

class pc8001_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, pc8001_state(machine)); }

	pc8001_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* video state */
	UINT8 *char_rom;
	int width80;
	int color;

	/* devices */
	running_device *i8257;
	running_device *upd1990a;
	running_device *upd3301;
	running_device *cassette;
	running_device *centronics;
	running_device *speaker;
};

#endif