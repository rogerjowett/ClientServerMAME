#ifndef __C80__
#define __C80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d2"
#define Z80PIO1_TAG		"d11"
#define Z80PIO2_TAG		"d12"
#define CASSETTE_TAG	"cassette"

class c80_state : public driver_device
{
public:
	c80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int keylatch;

	/* display state */
	int digit;
	int pio1_a5;
	int pio1_brdy;

	/* devices */
	device_t *z80pio;
	device_t *cassette;
};

#endif
