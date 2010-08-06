/***************************************************************************

    cpuexec.h

    Core multi-CPU execution engine.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __EMU_H__
#error Dont include this file directly; include emu.h instead.
#endif

#ifndef __CPUEXEC_H__
#define __CPUEXEC_H__

/***************************************************************************
    CONSTANTS
***************************************************************************/

/* flags for MDRV_CPU_FLAGS */
enum CPUDisableType
{
	/* set this flag to disable execution of a CPU (if one is there for documentation */
	/* purposes only, for example */
	CPU_DISABLE = 0x0001
};


/* suspension reasons for cpunum_suspend */
enum SuspendReasonType
{
	SUSPEND_REASON_HALT 	= 0x0001,
	SUSPEND_REASON_RESET	= 0x0002,
	SUSPEND_REASON_SPIN 	= 0x0004,
	SUSPEND_REASON_TRIGGER	= 0x0008,
	SUSPEND_REASON_DISABLE	= 0x0010,
	SUSPEND_REASON_TIMESLICE= 0x0020,
	SUSPEND_ANY_REASON		= ~0
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* opaque definition of CPU internal and debugging info */
typedef struct _cpu_debug_data cpu_debug_data;


/* interrupt callback for VBLANK and timed interrupts */
typedef void (*cpu_interrupt_func)(running_device *device);


/* CPU description for drivers */
typedef struct _cpu_config cpu_config;
struct _cpu_config
{
	cpu_type			type;						/* index for the CPU type */
	UINT32				flags;						/* flags; see #defines below */
	cpu_interrupt_func	vblank_interrupt;			/* for interrupts tied to VBLANK */
	int 				vblank_interrupts_per_frame;/* usually 1 */
	const char *		vblank_interrupt_screen;	/* the screen that causes the VBLANK interrupt */
	cpu_interrupt_func	timed_interrupt;			/* for interrupts not tied to VBLANK */
	UINT64				timed_interrupt_period;		/* period for periodic interrupts */
};


/* public data at the end of the device->token */
typedef struct _cpu_class_header cpu_class_header;
struct _cpu_class_header
{
	cpu_debug_data *		debug;					/* debugging data */
	cpu_set_info_func		set_info;				/* this CPU's set_info function */
};

/* internal information about the state of inputs */
typedef struct _cpu_input_data cpu_input_data;
struct _cpu_input_data
{
	INT32			vector;				/* most recently written vector */
	INT32			curvector;			/* most recently processed vector */
	UINT8			curstate;			/* most recently processed state */
	INT32			queue[MAX_INPUT_EVENTS]; /* queue of pending events */
	int				qindex;				/* index within the queue */
};

/* internal data hanging off of the classtoken */
typedef struct _cpu_class_data cpu_class_data;
struct _cpu_class_data
{
	/* execution lists */
	running_device *device;					/* pointer back to our device */
	cpu_class_data *next;					/* pointer to the next CPU to execute, in order */
	cpu_execute_func execute;				/* execute function pointer */

	/* cycle counting and executing */
	int				profiler;				/* profiler tag */
	int *			icount;					/* pointer to the icount */
	int 			cycles_running;			/* number of cycles we are executing */
	int				cycles_stolen;			/* number of cycles we artificially stole */

	/* input states and IRQ callbacks */
	cpu_irq_callback driver_irq;			/* driver-specific IRQ callback */
	cpu_input_data	input[MAX_INPUT_LINES]; /* data about inputs */

	/* suspend states */
	UINT8			suspend;				/* suspend reason mask (0 = not suspended) */
	UINT8			nextsuspend;			/* pending suspend reason mask */
	UINT8			eatcycles;				/* true if we eat cycles while suspended */
	UINT8			nexteatcycles;			/* pending value */
	INT32			trigger;				/* pending trigger to release a trigger suspension */
	INT32			inttrigger;				/* interrupt trigger index */

	/* clock and timing information */
	UINT64			totalcycles;			/* total CPU cycles executed */
	attotime		localtime;				/* local time, relative to the timer system's global time */
	INT32			clock;					/* current active clock */
	double			clockscale;				/* current active clock scale factor */
	INT32			divisor;				/* 32-bit attoseconds_per_cycle divisor */
	UINT8			divshift;				/* right shift amount to fit the divisor into 32 bits */
	emu_timer *		timedint_timer;			/* reference to this CPU's periodic interrupt timer */
	UINT32			cycles_per_second;		/* cycles per second, adjusted for multipliers */
	attoseconds_t	attoseconds_per_cycle;	/* attoseconds per adjusted clock cycle */

	/* internal state reflection */
	const cpu_state_table *state;			/* pointer to the base table */
	const cpu_state_entry *regstate[MAX_REGS];/* pointer to the state entry for each register */

	/* these below are hacks to support multiple interrupts per frame */
	INT32			iloops; 				/* number of interrupts remaining this frame */
	emu_timer *		partial_frame_timer;	/* the timer that triggers partial frame interrupts */
	attotime		partial_frame_period;	/* the length of one partial frame for interrupt purposes */
};


/***************************************************************************
    CPU DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_CPU_ADD(_tag, _type, _clock) \
	MDRV_DEVICE_ADD(_tag, CPU, _clock) \
	MDRV_DEVICE_CONFIG_DATAPTR(cpu_config, type, CPU_##_type)

#define MDRV_CPU_MODIFY(_tag) \
	MDRV_DEVICE_MODIFY(_tag)

#define MDRV_CPU_TYPE(_type) \
	MDRV_DEVICE_CONFIG_DATAPTR(cpu_config, type, CPU_##_type)

#define MDRV_CPU_CLOCK(_clock) \
	MDRV_DEVICE_CLOCK(_clock)

#define MDRV_CPU_REPLACE(_tag, _type, _clock) \
	MDRV_DEVICE_MODIFY(_tag) \
	MDRV_DEVICE_CONFIG_DATAPTR(cpu_config, type, CPU_##_type) \
	MDRV_DEVICE_CLOCK(_clock)

#define MDRV_CPU_FLAGS(_flags) \
	MDRV_DEVICE_CONFIG_DATA32(cpu_config, flags, _flags)

#define MDRV_CPU_CONFIG(_config) \
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_CPU_PROGRAM_MAP(_map) \
	MDRV_DEVICE_PROGRAM_MAP(_map)

#define MDRV_CPU_DATA_MAP(_map) \
	MDRV_DEVICE_DATA_MAP(_map)

#define MDRV_CPU_IO_MAP(_map) \
	MDRV_DEVICE_IO_MAP(_map)

#define MDRV_CPU_VBLANK_INT(_tag, _func) \
	MDRV_DEVICE_CONFIG_DATAPTR(cpu_config, vblank_interrupt, _func) \
	MDRV_DEVICE_CONFIG_DATAPTR(cpu_config, vblank_interrupt_screen, _tag) \
	MDRV_DEVICE_CONFIG_DATA32(cpu_config, vblank_interrupts_per_frame, 0)

#define MDRV_CPU_PERIODIC_INT(_func, _rate)	\
	MDRV_DEVICE_CONFIG_DATAPTR(cpu_config, timed_interrupt, _func) \
	MDRV_DEVICE_CONFIG_DATA64(cpu_config, timed_interrupt_period, UINT64_ATTOTIME_IN_HZ(_rate))



/***************************************************************************
    MACROS
***************************************************************************/

#define INTERRUPT_GEN(func)		void func(running_device *device)

/* helpers for using machine/cputag instead of cpu objects */
#define cputag_get_address_space(mach, tag, spc)						(mach)->device(tag)->space(spc)
#define cputag_suspend(mach, tag, reason, eat)							cpu_suspend((mach)->device(tag), reason, eat)
#define cputag_resume(mach, tag, reason)								cpu_resume((mach)->device(tag), reason)
#define cputag_is_suspended(mach, tag, reason)							cpu_is_suspended((mach)->device(tag), reason)
#define cputag_get_clock(mach, tag)										cpu_get_clock((mach)->device(tag))
#define cputag_set_clock(mach, tag, clock)								cpu_set_clock((mach)->device(tag), clock)
#define cputag_clocks_to_attotime(mach, tag, clocks)					cpu_clocks_to_attotime((mach)->device(tag), clocks)
#define cputag_attotime_to_clocks(mach, tag, duration)					cpu_attotime_to_clocks((mach)->device(tag), duration)
#define cputag_get_local_time(mach, tag)								cpu_get_local_time((mach)->device(tag))
#define cputag_get_total_cycles(mach, tag)								cpu_get_total_cycles((mach)->device(tag))
#define cputag_set_input_line(mach, tag, line, state)					cpu_set_input_line((mach)->device(tag), line, state)
#define cputag_set_input_line_and_vector(mach, tag, line, state, vec)	cpu_set_input_line_and_vector((mach)->device(tag), line, state, vec)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- core CPU execution ----- */

/* prepare CPUs for execution */
void cpuexec_init(running_machine *machine);

/* execute for a single timeslice */
void cpuexec_timeslice(running_machine *machine);

/* temporarily boosts the interleave factor */
void cpuexec_boost_interleave(running_machine *machine, attotime timeslice_time, attotime boost_duration);



/* ----- CPU device interface ----- */

/* device get info callback */
#define CPU DEVICE_GET_INFO_NAME(cpu)
DEVICE_GET_INFO( cpu );



/* ----- global helpers ----- */

/* abort execution for the current timeslice */
void cpuexec_abort_timeslice(running_machine *machine);

/* return a string describing which CPUs are currently executing and their PC */
const char *cpuexec_describe_context(running_machine *machine);



/* ----- CPU scheduling----- */

/* suspend the given CPU for a specific reason */
void cpu_suspend(running_device *device, int reason, int eatcycles);

/* resume the given CPU for a specific reason */
void cpu_resume(running_device *device, int reason);

/* return TRUE if the given CPU is within its execute function */
int cpu_is_executing(running_device *device);

/* returns TRUE if the given CPU is suspended for any of the given reasons */
int cpu_is_suspended(running_device *device, int reason);



/* ----- CPU clock management ----- */

/* returns the current CPU's unscaled running clock speed */
int cpu_get_clock(running_device *device);

/* sets the current CPU's clock speed and then adjusts for scaling */
void cpu_set_clock(running_device *device, int clock);

/* returns the current scaling factor for a CPU's clock speed */
double cpu_get_clockscale(running_device *device);

/* sets the current scaling factor for a CPU's clock speed */
void cpu_set_clockscale(running_device *device, double clockscale);

/* converts a number of clock ticks to an attotime */
attotime cpu_clocks_to_attotime(running_device *device, UINT64 clocks);

/* converts a duration as attotime to CPU clock ticks */
UINT64 cpu_attotime_to_clocks(running_device *device, attotime duration);



/* ----- CPU timing ----- */

/* returns the current local time for a CPU */
attotime cpu_get_local_time(running_device *device);

/* returns the total number of CPU cycles for a given CPU */
UINT64 cpu_get_total_cycles(running_device *device);

/* safely eats cycles so we don't cross a timeslice boundary */
void cpu_eat_cycles(running_device *device, int cycles);

/* apply a +/- to the current icount */
void cpu_adjust_icount(running_device *device, int delta);

/* abort execution for the current timeslice, allowing other CPUs to run before we run again */
void cpu_abort_timeslice(running_device *device);



/* ----- synchronization helpers ----- */

/* yield the given CPU until the end of the current timeslice */
void cpu_yield(running_device *device);

/* burn CPU cycles until the end of the current timeslice */
void cpu_spin(running_device *device);

/* burn specified CPU cycles until a trigger */
void cpu_spinuntil_trigger(running_device *device, int trigger);

/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int(running_device *device);

/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time(running_device *device, attotime duration);



/* ----- triggers ----- */

/* generate a global trigger now */
void cpuexec_trigger(running_machine *machine, int trigger);

/* generate a global trigger after a specific period of time */
void cpuexec_triggertime(running_machine *machine, int trigger, attotime duration);

/* generate a trigger corresponding to an interrupt on the given CPU */
void cpu_triggerint(running_device *device);



/* ----- interrupts ----- */

/* set the logical state (ASSERT_LINE/CLEAR_LINE) of the an input line on a CPU */
void cpu_set_input_line(running_device *cpu, int line, int state);

/* set the vector to be returned during a CPU's interrupt acknowledge cycle */
void cpu_set_input_line_vector(running_device *cpu, int irqline, int vector);

/* set the logical state (ASSERT_LINE/CLEAR_LINE) of the an input line on a CPU and its associated vector */
void cpu_set_input_line_and_vector(running_device *cpu, int line, int state, int vector);

/* install a driver-specific callback for IRQ acknowledge */
void cpu_set_irq_callback(running_device *cpu, cpu_irq_callback callback);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    cpu_get_type - return the type of the
    specified CPU
-------------------------------------------------*/

INLINE cpu_type cpu_get_type(running_device *device)
{
	const cpu_config *config = (const cpu_config *)device->baseconfig().inline_config;
	return config->type;
}


/*-------------------------------------------------
    cpu_get_class_header - return a pointer to
    the class header
-------------------------------------------------*/

INLINE cpu_class_header *cpu_get_class_header(running_device *device)
{
	if (device->token != NULL)
	{
		return (cpu_class_header *)((UINT8 *)device->token + device->tokenbytes) - 1;
	}
	return NULL;
}


/*-------------------------------------------------
    cpu_get_debug_data - return a pointer to
    the given CPU's debugger data
-------------------------------------------------*/

INLINE cpu_debug_data *cpu_get_debug_data(running_device *device)
{
	cpu_class_header *classheader = cpu_get_class_header(device);
	return classheader->debug;
}


/*-------------------------------------------------
    cpu_get_address_space - return a pointer to
    the given CPU's address space
-------------------------------------------------*/

INLINE const address_space *cpu_get_address_space(running_device *device, int spacenum)
{
	return device->space(spacenum);
}

#endif	/* __CPUEXEC_H__ */
