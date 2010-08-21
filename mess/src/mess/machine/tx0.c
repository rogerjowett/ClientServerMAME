/*
    TX-0
*
    Raphael Nabet, 2004
*/

#include "emu.h"
#include "cpu/pdp1/tx0.h"
#include "includes/tx0.h"
#include "video/crt.h"


static TIMER_CALLBACK(reader_callback);
static TIMER_CALLBACK(puncher_callback);
static TIMER_CALLBACK(prt_callback);
static TIMER_CALLBACK(dis_callback);


/* tape reader registers */
typedef struct tape_reader_t
{
	device_image_interface *fd;	/* file descriptor of tape image */

	int motor_on;	/* 1-bit reader motor on */

	int rcl;		/* 1-bit reader clutch */
	int rc;			/* 2-bit reader counter */

	emu_timer *timer;	/* timer to simulate reader timing */
} tape_reader_t;

static tape_reader_t tape_reader;


/* tape puncher registers */
typedef struct tape_puncher_t
{
	device_image_interface *fd;	/* file descriptor of tape image */

	emu_timer *timer;	/* timer to generate completion pulses */
} tape_puncher_t;

static tape_puncher_t tape_puncher;


/* typewriter registers */
typedef struct typewriter_t
{
	device_image_interface *fd;	/* file descriptor of output image */

	emu_timer *prt_timer;/* timer to generate completion pulses */
} typewriter_t;

static typewriter_t typewriter;


/* crt display timer */
static emu_timer *dis_timer;


enum state_t
{
	MTS_UNSELECTED,
	MTS_SELECTING,
	MTS_SELECTED,
	MTS_UNSELECTING
};
enum backspace_state_t
{
	MTBSS_STATE0,
	MTBSS_STATE1,
	MTBSS_STATE2,
	MTBSS_STATE3,
	MTBSS_STATE4,
	MTBSS_STATE5,
	MTBSS_STATE6
};
enum state_2_t
{
	MTRDS_STATE0,
	MTRDS_STATE1,
	MTRDS_STATE2,
	MTRDS_STATE3,
	MTRDS_STATE4,
	MTRDS_STATE5,
	MTRDS_STATE6
};
enum state_3_t
{
	MTWTS_STATE0,
	MTWTS_STATE1,
	MTWTS_STATE2,
	MTWTS_STATE3
};

enum irg_pos_t
{
	MTIRGP_START,
	MTIRGP_ENDMINUS1,
	MTIRGP_END
};
/* magnetic tape unit registers */
typedef struct magtape_t
{
	device_image_interface *img;		/* image descriptor */

	state_t state;

	int command;
	int binary_flag;

	union
	{
		backspace_state_t backspace_state;
		struct
		{
			state_2_t state;
			int space_flag;
		} read;
		struct
		{
			state_3_t state;
			int counter;
		} write;
	} u;

	int sel_pending;
	int cpy_pending;

	irg_pos_t irg_pos;			/* position relative to inter-record gap */

	int long_parity;

	emu_timer *timer;	/* timer to simulate reader timing */
} magtape_t;

static magtape_t magtape;

enum
{
	PF_RWC = 040,
	PF_EOR = 020,
	PF_PC  = 010,
	PF_EOT = 004
};


MACHINE_RESET( tx0 )
{
	/* reset device state */
	tape_reader.rcl = tape_reader.rc = 0;
}


static void tx0_machine_stop(running_machine &machine)
{
	/* the core will take care of freeing the timers, BUT we must set the variables
    to NULL if we don't want to risk confusing the tape image init function */
	tape_reader.timer = tape_puncher.timer = typewriter.prt_timer = dis_timer = NULL;
}


MACHINE_START( tx0 )
{
	tape_reader.timer = timer_alloc(machine, reader_callback, NULL);
	tape_puncher.timer = timer_alloc(machine, puncher_callback, NULL);
	typewriter.prt_timer = timer_alloc(machine, prt_callback, NULL);
	dis_timer = timer_alloc(machine, dis_callback, NULL);

	machine->add_notifier(MACHINE_NOTIFY_EXIT, tx0_machine_stop);
}


/*
    perforated tape handling
*/

void tx0_tape_get_open_mode(int id,	unsigned int *readable, unsigned int *writeable, unsigned int *creatable)
{
	/* unit 0 is read-only, unit 1 is write-only */
	if (id)
	{
		*readable = 0;
		*writeable = 1;
		*creatable = 1;
	}
	else
	{
		*readable = 1;
		*writeable = 0;
		*creatable = 0;
	}
}

#if 0
DEVICE_START( tx0_tape )
{
}


/*
    Open a perforated tape image

    unit 0 is reader (read-only), unit 1 is puncher (write-only)
*/
DEVICE_IMAGE_LOAD( tx0_tape )
{
	int id = image_index_in_device(image);

	switch (id)
	{
	case 0:
		/* reader unit */
		tape_reader.fd = image;

		/* start motor */
		tape_reader.motor_on = 1;

		/* restart reader IO when necessary */
		/* note that this function may be called before tx0_init_machine, therefore
        before tape_reader.timer is allocated.  It does not matter, as the clutch is never
        down at power-up, but we must not call timer_enable with a NULL parameter! */
		if (tape_reader.timer)
		{
			if (tape_reader.motor_on && tape_reader.rcl)
			{
				/* delay is approximately 1/400s */
				timer_adjust_oneshot(tape_reader.timer, ATTOTIME_IN_USEC(2500), 0);
			}
			else
			{
				timer_enable(tape_reader.timer, 0);
			}
		}
		break;

	case 1:
		/* punch unit */
		tape_puncher.fd = image;
		break;
	}

	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( tx0_tape )
{
	int id = image_index_in_device(image);

	switch (id)
	{
	case 0:
		/* reader unit */
		tape_reader.fd = NULL;

		/* stop motor */
		tape_reader.motor_on = 0;

		if (tape_reader.timer)
			timer_enable(tape_reader.timer, 0);
		break;

	case 1:
		/* punch unit */
		tape_puncher.fd = NULL;
		break;
	}
}
#endif
/*
    Read a byte from perforated tape
*/
static int tape_read(UINT8 *reply)
{
	if (tape_reader.fd && (tape_reader.fd->fread(reply, 1) == 1))
		return 0;	/* unit OK */
	else
		return 1;	/* unit not ready */
}

/*
    Write a byte to perforated tape
*/
static void tape_write(UINT8 data)
{
	if (tape_puncher.fd)
		tape_puncher.fd->fwrite(& data, 1);
}

/*
    common code for tape read commands (R1C, R3C, and read-in mode)
*/
static void begin_tape_read(int binary)
{
	tape_reader.rcl = 1;
	tape_reader.rc = (binary) ? 1 : 3;

	/* set up delay if tape is advancing */
	if (tape_reader.motor_on && tape_reader.rcl)
	{
		/* delay is approximately 1/400s */
		timer_adjust_oneshot(tape_reader.timer, ATTOTIME_IN_USEC(2500), 0);
	}
	else
	{
		timer_enable(tape_reader.timer, 0);
	}
}


/*
    timer callback to simulate reader IO
*/
static TIMER_CALLBACK(reader_callback)
{
	int not_ready;
	UINT8 data;
	int ac;

	if (tape_reader.rc)
	{
		not_ready = tape_read(& data);
		if (not_ready)
		{
			tape_reader.motor_on = 0;	/* let us stop the motor */
		}
		else
		{
			if (data & 0100)
			{
				/* read current AC */
				ac = cpu_get_reg(machine->device("maincpu"), TX0_AC);
				/* cycle right */
				ac = (ac >> 1) | ((ac & 1) << 17);
				/* shuffle and insert data into AC */
				ac = (ac /*& 0333333*/) | ((data & 001) << 17) | ((data & 002) << 13) | ((data & 004) << 9) | ((data & 010) << 5) | ((data & 020) << 1) | ((data & 040) >> 3);
				/* write modified AC */
				cpu_set_reg(machine->device("maincpu"), TX0_AC, ac);

				tape_reader.rc = (tape_reader.rc+1) & 3;

				if (tape_reader.rc == 0)
				{	/* IO complete */
					tape_reader.rcl = 0;
					cpu_set_reg(machine->device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
				}
			}
		}
	}

	if (tape_reader.motor_on && tape_reader.rcl)
		/* delay is approximately 1/400s */
		timer_adjust_oneshot(tape_reader.timer, ATTOTIME_IN_USEC(2500), 0);
	else
		timer_enable(tape_reader.timer, 0);
}

/*
    timer callback to generate punch completion pulse
*/
static TIMER_CALLBACK(puncher_callback)
{
	cpu_set_reg(machine->device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
}

/*
    Initiate read of a 6-bit word from tape
*/
void tx0_io_r1l(running_device *device)
{
	begin_tape_read(0);
}

/*
    Initiate read of a 18-bit word from tape (used in read-in mode)
*/
void tx0_io_r3l(running_device *device)
{
	begin_tape_read(1);
}

/*
    Write a 7-bit word to tape (7th bit clear)
*/
void tx0_io_p6h(running_device *device)
{
	int ac;

	/* read current AC */
	ac = cpu_get_reg(device, TX0_AC);
	/* shuffle and punch 6-bit word */
	tape_write(((ac & 0100000) >> 15) | ((ac & 0010000) >> 11) | ((ac & 0001000) >> 7) | ((ac & 0000100) >> 3) | ((ac & 0000010) << 1) | ((ac & 0000001) << 5));

	timer_adjust_oneshot(tape_puncher.timer, ATTOTIME_IN_USEC(15800), 0);
}

/*
    Write a 7-bit word to tape (7th bit set)
*/
void tx0_io_p7h(running_device *device)
{
	int ac;

	/* read current AC */
	ac = cpu_get_reg(device, TX0_AC);
	/* shuffle and punch 6-bit word */
	tape_write(((ac & 0100000) >> 15) | ((ac & 0010000) >> 11) | ((ac & 0001000) >> 7) | ((ac & 0000100) >> 3) | ((ac & 0000010) << 1) | ((ac & 0000001) << 5) | 0100);

	timer_adjust_oneshot(tape_puncher.timer, ATTOTIME_IN_USEC(15800), 0);
}


/*
    Typewriter handling

    The alphanumeric on-line typewriter is a standard device on tx-0: it can
    both handle keyboard input and print output text.
*/

/*
    Open a file for typewriter output
*/
DEVICE_IMAGE_LOAD(tx0_typewriter)
{
	/* open file */
	typewriter.fd = &image;

	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD(tx0_typewriter)
{
	typewriter.fd = NULL;
}

/*
    Write a character to typewriter
*/
static void typewriter_out(running_machine *machine, UINT8 data)
{
	tx0_typewriter_drawchar(machine, data);
	if (typewriter.fd)
		typewriter.fd->fwrite(& data, 1);
}

/*
    timer callback to generate typewriter completion pulse
*/
static TIMER_CALLBACK(prt_callback)
{
	cpu_set_reg(machine->device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
}

/*
    prt io callback
*/
void tx0_io_prt(running_device *device)
{
	int ac;
	int ch;

	/* read current AC */
	ac = cpu_get_reg(device, TX0_AC);
	/* shuffle and print 6-bit word */
	ch = ((ac & 0100000) >> 15) | ((ac & 0010000) >> 11) | ((ac & 0001000) >> 7) | ((ac & 0000100) >> 3) | ((ac & 0000010) << 1) | ((ac & 0000001) << 5);
	typewriter_out(device->machine, ch);

	timer_adjust_oneshot(typewriter.prt_timer, ATTOTIME_IN_MSEC(100), 0);
}


/*
    timer callback to generate crt completion pulse
*/
static TIMER_CALLBACK(dis_callback)
{
	cpu_set_reg(machine->device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
}

/*
    Plot one point on crt
*/
void tx0_io_dis(running_device *device)
{
	int ac;
	int x;
	int y;

	ac = cpu_get_reg(device, TX0_AC);
	x = ac >> 9;
	y = ac & 0777;
	tx0_plot(x, y);

	timer_adjust_oneshot(dis_timer, ATTOTIME_IN_USEC(50), 0);
}


/*
    Magtape support

    Magtape format:

    7-track tape, 6-bit data, 1-bit parity


*/

static void schedule_select(void)
{
	attotime delay = attotime_zero;

	switch (magtape.command)
	{
	case 0:	/* backspace */
		delay = ATTOTIME_IN_USEC(4600);
		break;
	case 1:	/* read */
		delay = ATTOTIME_IN_USEC(8600);
		break;
	case 2:	/* rewind */
		delay = ATTOTIME_IN_USEC(12000);
		break;
	case 3:	/* write */
		delay = ATTOTIME_IN_USEC(4600);
		break;
	}
	timer_adjust_oneshot(magtape.timer, delay, 0);
}

static void schedule_unselect(void)
{
	attotime delay = attotime_zero;

	switch (magtape.command)
	{
	case 0:	/* backspace */
		delay = ATTOTIME_IN_USEC(5750);
		break;
	case 1:	/* read */
		delay = ATTOTIME_IN_USEC(1750);
		break;
	case 2:	/* rewind */
		delay = ATTOTIME_IN_USEC(0);
		break;
	case 3:	/* write */
		delay = ATTOTIME_IN_USEC(5750);
		break;
	}
	timer_adjust_oneshot(magtape.timer, delay, 0);
}

DEVICE_START( tx0_magtape )
{
	magtape.img = dynamic_cast<device_image_interface *>(device);
}

/*
    Open a magnetic tape image
*/
DEVICE_IMAGE_LOAD( tx0_magtape )
{
	magtape.img = &image;

	magtape.irg_pos = MTIRGP_END;

	/* restart IO when necessary */
	/* note that this function may be called before tx0_init_machine, therefore
    before magtape.timer is allocated.  We must not call timer_enable with a
    NULL parameter! */
	if (magtape.timer)
	{
		if (magtape.state == MTS_SELECTING)
			schedule_select();
	}

	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( tx0_magtape )
{
	magtape.img = NULL;

	if (magtape.timer)
	{
		if (magtape.state == MTS_SELECTING)
			/* I/O has not actually started, we can cancel the selection */
			timer_enable(tape_reader.timer, 0);
		if ((magtape.state == MTS_SELECTED) || ((magtape.state == MTS_SELECTING) && (magtape.command == 2)))
		{	/* unit has become unavailable */
			magtape.state = MTS_UNSELECTING;
			cpu_set_reg(image.device().machine->device("maincpu"), TX0_PF, cpu_get_reg(image.device().machine->device("maincpu"), TX0_PF) | PF_RWC);
			schedule_unselect();
		}
	}
}

static void magtape_callback(running_device *device)
{
	UINT8 buf = 0;
	int lr;

	switch (magtape.state)
	{
	case MTS_UNSELECTING:
		magtape.state = MTS_UNSELECTED;

	case MTS_UNSELECTED:
		if (magtape.sel_pending)
		{
			int mar;

			mar = cpu_get_reg(device, TX0_MAR);

			if ((mar & 03) != 1)
			{	/* unimplemented device: remain in unselected state and set rwc
                flag? */
				cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_RWC);
			}
			else
			{
				magtape.state = MTS_SELECTING;

				magtape.command = (mar & 014 >> 2);

				magtape.binary_flag = (mar & 020 >> 4);

				if (magtape.img)
					schedule_select();
			}

			magtape.sel_pending = FALSE;
			cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
		}
		break;

	case MTS_SELECTING:
		magtape.state = MTS_SELECTED;
		switch (magtape.command)
		{
		case 0:	/* backspace */
			magtape.long_parity = 0177;
			magtape.u.backspace_state = MTBSS_STATE0;
			break;
		case 1:	/* read */
			magtape.long_parity = 0177;
			magtape.u.read.state = MTRDS_STATE0;
			break;
		case 2:	/* rewind */
			break;
		case 3:	/* write */
			magtape.long_parity = 0177;
			magtape.u.write.state = MTWTS_STATE0;
			switch (magtape.irg_pos)
			{
			case MTIRGP_START:
				magtape.u.write.counter = 150;
				break;
			case MTIRGP_ENDMINUS1:
				magtape.u.write.counter = 1;
				break;
			case MTIRGP_END:
				magtape.u.write.counter = 0;
				break;
			}
			break;
		}

	case MTS_SELECTED:
		switch (magtape.command)
		{
		case 0:	/* backspace */
			if (magtape.img->ftell() == 0)
			{	/* tape at ldp */
				magtape.state = MTS_UNSELECTING;
				cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_RWC);
				schedule_unselect();
			}
			else if (magtape.img->fseek( -1, SEEK_CUR))
			{	/* eject tape */
				magtape.img->unload();
			}
			else if (magtape.img->fread(&buf, 1) != 1)
			{	/* eject tape */
				magtape.img->unload();
			}
			else if (magtape.img->fseek( -1, SEEK_CUR))
			{	/* eject tape */
				magtape.img->unload();
			}
			else
			{
				buf &= 0x7f;	/* 7-bit tape, ignore 8th bit */
				magtape.long_parity ^= buf;
				switch (magtape.u.backspace_state)
				{
				case MTBSS_STATE0:
					/* STATE0 -> initial interrecord gap, longitudinal parity;
                    if longitudinal parity was all 0s, gap between longitudinal
                    parity and data, first byte of data */
					if (buf != 0)
						magtape.u.backspace_state = MTBSS_STATE1;
					break;
				case MTBSS_STATE1:
					/* STATE1 -> first byte of gap between longitudinal parity and
                    data, second byte of data */
					if (buf == 0)
						magtape.u.backspace_state = MTBSS_STATE2;
					else
						magtape.u.backspace_state = MTBSS_STATE5;
					break;
				case MTBSS_STATE2:
					/* STATE2 -> second byte of gap between longitudinal parity and
                    data */
					if (buf == 0)
						magtape.u.backspace_state = MTBSS_STATE3;
					else
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					break;
				case MTBSS_STATE3:
					/* STATE3 -> third byte of gap between longitudinal parity and
                    data */
					if (buf == 0)
						magtape.u.backspace_state = MTBSS_STATE4;
					else
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					break;
				case MTBSS_STATE4:
					/* STATE4 -> first byte of data word, first byte of
                    interrecord gap after data */
					if (buf == 0)
					{
						if (magtape.long_parity)
							logerror("invalid longitudinal parity\n");
						/* set EOR and unselect... */
						magtape.state = MTS_UNSELECTING;
						cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_EOR);
						schedule_unselect();
						magtape.irg_pos = MTIRGP_ENDMINUS1;
					}
					else
						magtape.u.backspace_state = MTBSS_STATE5;
					break;
				case MTBSS_STATE5:
					/* STATE5 -> second byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					else
						magtape.u.backspace_state = MTBSS_STATE6;
					break;
				case MTBSS_STATE6:
					/* STATE6 -> third byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					else
						magtape.u.backspace_state = MTBSS_STATE6;
					break;
				}
				if (magtape.state != MTS_UNSELECTING)
					timer_adjust_oneshot(magtape.timer, ATTOTIME_IN_USEC(66), 0);
			}
			break;

		case 1:	/* read */
			if (magtape.img->fread(&buf, 1) != 1)
			{	/* I/O error or EOF? */
				/* The MAME fileio layer makes it very hard to make the
                difference...  MAME seems to assume that I/O errors never
                happen, whereas it is really easy to cause one by
                deconnecting an external drive the image is located on!!! */
				UINT64 offs;
				offs = magtape.img->ftell();
				if (magtape.img->fseek( 0, SEEK_END) || (offs != magtape.img->ftell()))
				{	/* I/O error */
					/* eject tape */
					magtape.img->unload();
				}
				else
				{	/* end of tape -> ??? */
					/* maybe we run past end of tape, so that tape is ejected from
                    upper reel and unit becomes unavailable??? */
					/*magtape.img->unload();*/
					/* Or do we stop at EOT mark??? */
					magtape.state = MTS_UNSELECTING;
					cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_EOT);
					schedule_unselect();
				}
			}
			else
			{
				buf &= 0x7f;	/* 7-bit tape, ignore 8th bit */
				magtape.long_parity ^= buf;
				switch (magtape.u.read.state)
				{
				case MTRDS_STATE0:
					/* STATE0 -> interrecord blank or first byte of data */
					if (buf != 0)
					{
						if (magtape.cpy_pending)
						{	/* read command */
							magtape.u.read.space_flag = FALSE;
							cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
							cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
														| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
							/* check parity */
							if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ magtape.binary_flag))
								cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
						}
						else
						{	/* space command */
							magtape.u.read.space_flag = TRUE;
						}
						magtape.u.read.state = MTRDS_STATE1;
					}
					break;
				case MTRDS_STATE1:
					/* STATE1 -> second byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					if (!magtape.u.read.space_flag)
					{
						cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
													| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
						/* check parity */
						if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ magtape.binary_flag))
							cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
					}
					magtape.u.read.state = MTRDS_STATE2;
					break;
				case MTRDS_STATE2:
					/* STATE2 -> third byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					if (!magtape.u.read.space_flag)
					{
						cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
													| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
						/* check parity */
						if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ magtape.binary_flag))
							cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
						/* synchronize with cpy instruction */
						if (magtape.cpy_pending)
							cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
						else
							cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_RWC);
					}
					magtape.u.read.state = MTRDS_STATE3;
					break;
				case MTRDS_STATE3:
					/* STATE3 -> first byte of new word of data, or first byte
                    of gap between data and longitudinal parity */
					if (buf != 0)
					{
						magtape.u.read.state = MTRDS_STATE1;
						if (!magtape.u.read.space_flag)
						{
							cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
														| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
							/* check parity */
							if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ magtape.binary_flag))
								cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
						}
					}
					else
						magtape.u.read.state = MTRDS_STATE4;
					break;
				case MTRDS_STATE4:
					/* STATE4 -> second byte of gap between data and
                    longitudinal parity */
					if (buf != 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					else
						magtape.u.read.state = MTRDS_STATE5;
					break;

				case MTRDS_STATE5:
					/* STATE5 -> third byte of gap between data and
                    longitudinal parity */
					if (buf != 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						magtape.img->unload();
					}
					else
						magtape.u.read.state = MTRDS_STATE6;
					break;

				case MTRDS_STATE6:
					/* STATE6 -> longitudinal parity */
					/* check parity */
					if (magtape.long_parity)
					{
						logerror("invalid longitudinal parity\n");
						/* no idea if the original tx-0 magtape controller
                        checks parity, but can't harm if we do */
						cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
					}
					/* set EOR and unselect... */
					magtape.state = MTS_UNSELECTING;
					cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_EOR);
					schedule_unselect();
					magtape.irg_pos = MTIRGP_START;
					break;
				}
				if (magtape.state != MTS_UNSELECTING)
					timer_adjust_oneshot(magtape.timer, ATTOTIME_IN_USEC(66), 0);
			}
			break;

		case 2:	/* rewind */
			magtape.state = MTS_UNSELECTING;
			/* we rewind at 10*read speed (I don't know the real value) */
			timer_adjust_oneshot(magtape.timer, attotime_mul(ATTOTIME_IN_NSEC(6600), magtape.img->ftell()), 0);
			//schedule_unselect();
			magtape.img->fseek( 0, SEEK_END);
			magtape.irg_pos = MTIRGP_END;
			break;

		case 3:	/* write */
			switch (magtape.u.write.state)
			{
			case MTWTS_STATE0:
				if (magtape.u.write.counter != 0)
				{
					magtape.u.write.counter--;
					buf = 0;
					break;
				}
				else
				{
					magtape.u.write.state = MTWTS_STATE1;
				}

			case MTWTS_STATE1:
				if (magtape.u.write.counter)
				{
					magtape.u.write.counter--;
					lr = cpu_get_reg(device, TX0_LR);
					buf = ((lr >> 10) & 040) | ((lr >> 8) & 020) | ((lr >> 6) & 010) | ((lr >> 4) & 004) | ((lr >> 2) & 002) | (lr & 001);
					buf |= ((buf << 1) ^ (buf << 2) ^ (buf << 3) ^ (buf << 4) ^ (buf << 5) ^ (buf << 6) ^ ((!magtape.binary_flag) << 6)) & 0100;
					cpu_set_reg(device, TX0_LR, lr >> 1);
				}
				else
				{
					if (magtape.cpy_pending)
					{
						cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
						lr = cpu_get_reg(device, TX0_LR);
						buf = ((lr >> 10) & 040) | ((lr >> 8) & 020) | ((lr >> 6) & 010) | ((lr >> 4) & 004) | ((lr >> 2) & 002) | (lr & 001);
						buf |= ((buf << 1) ^ (buf << 2) ^ (buf << 3) ^ (buf << 4) ^ (buf << 5) ^ (buf << 6) ^ ((!magtape.binary_flag) << 6)) & 0100;
						cpu_set_reg(device, TX0_LR, lr >> 1);
						magtape.u.write.counter = 2;
						break;
					}
					else
					{
						magtape.u.write.state = MTWTS_STATE2;
						magtape.u.write.counter = 3;
					}
				}

			case MTWTS_STATE2:
				if (magtape.u.write.counter != 0)
				{
					magtape.u.write.counter--;
					buf = 0;
					break;
				}
				else
				{
					buf = magtape.long_parity;
					magtape.state = (state_t)MTWTS_STATE3;
					magtape.u.write.counter = 150;
				}
				break;

			case MTWTS_STATE3:
				if (magtape.u.write.counter != 0)
				{
					magtape.u.write.counter--;
					buf = 0;
					break;
				}
				else
				{
					magtape.state = MTS_UNSELECTING;
					schedule_unselect();
					magtape.irg_pos = MTIRGP_END;
				}
				break;
			}
			if (magtape.state != MTS_UNSELECTING)
			{	/* write data word */
				magtape.long_parity ^= buf;
				if (magtape.img->fwrite(&buf, 1) != 1)
				{	/* I/O error */
					/* eject tape */
					magtape.img->unload();
				}
				else
					timer_adjust_oneshot(magtape.timer, ATTOTIME_IN_USEC(66), 0);
			}
			break;
		}
		break;
	}
}

void tx0_sel(running_device *device)
{
	magtape.sel_pending = TRUE;

	if (magtape.state == MTS_UNSELECTED)
	{
		if (0)
			magtape_callback(device);
		timer_adjust_oneshot(magtape.timer, attotime_zero, 0);
	}
}

void tx0_io_cpy(running_device *device)
{
	switch (magtape.state)
	{
	case MTS_UNSELECTED:
	case MTS_UNSELECTING:
		/* ignore instruction and set rwc flag? */
		cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
		break;

	case MTS_SELECTING:
	case MTS_SELECTED:
		switch (magtape.command)
		{
		case 0:	/* backspace */
		case 2:	/* rewind */
			/* ignore instruction and set rwc flag? */
			cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
			break;
		case 1:	/* read */
		case 3:	/* write */
			magtape.cpy_pending = TRUE;
			break;
		}
		break;
	}
}


/*
    callback which is called when reset line is pulsed

    IO devices should reset
*/
void tx0_io_reset_callback(running_device *device)
{
	tape_reader.rcl = tape_reader.rc = 0;
	if (tape_reader.timer)
		timer_enable(tape_reader.timer, 0);

	if (tape_puncher.timer)
		timer_enable(tape_puncher.timer, 0);

	if (typewriter.prt_timer)
		timer_enable(typewriter.prt_timer, 0);

	if (dis_timer)
		timer_enable(dis_timer, 0);
}


/*
    typewriter keyboard handler
*/
static void tx0_keyboard(running_machine *machine)
{
	int i;
	int j;

	int typewriter_keys[4];
	static int old_typewriter_keys[4];

	int typewriter_transitions;
	int charcode, lr;
	static const char *const twrnames[] = { "TWR0", "TWR1", "TWR2", "TWR3" };

	for (i=0; i<4; i++)
	{
		typewriter_keys[i] = input_port_read(machine, twrnames[i]);
	}

	for (i=0; i<4; i++)
	{
		typewriter_transitions = typewriter_keys[i] & (~ old_typewriter_keys[i]);
		if (typewriter_transitions)
		{
			for (j=0; (((typewriter_transitions >> j) & 1) == 0) /*&& (j<16)*/; j++)
				;
			charcode = (i << 4) + j;
			/* shuffle and insert data into LR */
			/* BTW, I am not sure how the char code is combined with the
            previous LR */
			lr = (1 << 17) | ((charcode & 040) << 10) | ((charcode & 020) << 8) | ((charcode & 010) << 6) | ((charcode & 004) << 4) | ((charcode & 002) << 2) | ((charcode & 001) << 1);
			/* write modified LR */
			cpu_set_reg(machine->device("maincpu"), TX0_LR, lr);
			tx0_typewriter_drawchar(machine, charcode);	/* we want to echo input */
			break;
		}
	}

	for (i=0; i<4; i++)
		old_typewriter_keys[i] = typewriter_keys[i];
}

/*
    Not a real interrupt - just handle keyboard input
*/
INTERRUPT_GEN( tx0_interrupt )
{
	int control_keys;
	int tsr_keys;

	static int old_control_keys;
	static int old_tsr_keys;
	static int tsr_index = 0;

	int control_transitions;
	int tsr_transitions;


	/* read new state of control keys */
	control_keys = input_port_read(device->machine, "CSW");

	if (control_keys & tx0_control)
	{
		/* compute transitions */
		control_transitions = control_keys & (~ old_control_keys);

		if (control_transitions & tx0_stop_cyc0)
		{
			cpu_set_reg(device->machine->device("maincpu"), TX0_STOP_CYC0, !cpu_get_reg(device->machine->device("maincpu"), TX0_STOP_CYC0));
		}
		if (control_transitions & tx0_stop_cyc1)
		{
			cpu_set_reg(device->machine->device("maincpu"), TX0_STOP_CYC1, !cpu_get_reg(device->machine->device("maincpu"), TX0_STOP_CYC1));
		}
		if (control_transitions & tx0_gbl_cm_sel)
		{
			cpu_set_reg(device->machine->device("maincpu"), TX0_GBL_CM_SEL, !cpu_get_reg(device->machine->device("maincpu"), TX0_GBL_CM_SEL));
		}
		if (control_transitions & tx0_stop)
		{
			cpu_set_reg(device->machine->device("maincpu"), TX0_RUN, (UINT64)0);
			cpu_set_reg(device->machine->device("maincpu"), TX0_RIM, (UINT64)0);
		}
		if (control_transitions & tx0_restart)
		{
			cpu_set_reg(device->machine->device("maincpu"), TX0_RUN, 1);
			cpu_set_reg(device->machine->device("maincpu"), TX0_RIM, (UINT64)0);
		}
		if (control_transitions & tx0_read_in)
		{	/* set cpu to read instructions from perforated tape */
			cpu_set_reg(device->machine->device("maincpu"), TX0_RESET, (UINT64)0);
			cpu_set_reg(device->machine->device("maincpu"), TX0_RUN, (UINT64)0);
			cpu_set_reg(device->machine->device("maincpu"), TX0_RIM, 1);
		}
		if (control_transitions & tx0_toggle_dn)
		{
			tsr_index++;
			if (tsr_index == 18)
				tsr_index = 0;
		}
		if (control_transitions & tx0_toggle_up)
		{
			tsr_index--;
			if (tsr_index == -1)
				tsr_index = 17;
		}
		if (control_transitions & tx0_cm_sel)
		{
			if (tsr_index >= 2)
			{
				UINT32 cm_sel = (UINT32) cpu_get_reg(device->machine->device("maincpu"), TX0_CM_SEL);
				cpu_set_reg(device->machine->device("maincpu"), TX0_CM_SEL, cm_sel ^ (1 << (tsr_index - 2)));
			}
		}
		if (control_transitions & tx0_lr_sel)
		{
			if (tsr_index >= 2)
			{
				UINT32 lr_sel = (UINT32) cpu_get_reg(device->machine->device("maincpu"), TX0_LR_SEL);
				cpu_set_reg(device->machine->device("maincpu"), TX0_LR_SEL, (lr_sel ^ (1 << (tsr_index - 2))));
			}
		}

		/* remember new state of control keys */
		old_control_keys = control_keys;


		/* handle toggle switch register keys */
		tsr_keys = (input_port_read(device->machine, "MSW") << 16) | input_port_read(device->machine, "LSW");

		/* compute transitions */
		tsr_transitions = tsr_keys & (~ old_tsr_keys);

		/* update toggle switch register */
		if (tsr_transitions)
			cpu_set_reg(device->machine->device("maincpu"), TX0_TBR+tsr_index, cpu_get_reg(device->machine->device("maincpu"), TX0_TBR+tsr_index) ^ tsr_transitions);

		/* remember new state of toggle switch register keys */
		old_tsr_keys = tsr_keys;
	}
	else
	{
		old_control_keys = 0;
		old_tsr_keys = 0;

		tx0_keyboard(device->machine);
	}
}

