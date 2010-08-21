
/*********************************************************************

    includes/x07.h

*********************************************************************/

/***************************************************************************
    CONSTANTS
***************************************************************************/
static const char *udk_ini[12] = {
	"tim?TIME$\15",
	"cldCLOAD\"",
	"locLOCATE ",
	"lstLIST ",
	"runRUN\15",
	"",
	"dat?DATE$\15",
	"csvCSAVE\"",
	"prtPRINT ",
	"slpSLEEP",
	"cntCONT\15",
	""
};

static const UINT8 printer_charcode[256] = {
	0xff, 0x7f, 0xbf, 0x3f, 0xdf, 0x5f, 0x9f, 0x1f,
	0xef, 0x6f, 0xaf, 0x2f, 0xcf, 0x4f, 0x8f, 0x0f,
	0xf7, 0x77, 0xb7, 0x37, 0xd7, 0x57, 0x97, 0x17,
	0xe7, 0x67, 0xa7, 0x27, 0xc7, 0x47, 0x87, 0x07,
	0xfb, 0x7b, 0xbb, 0x3b, 0xdb, 0x5b, 0x9b, 0x1b,
	0xeb, 0x6b, 0xab, 0x2b, 0xcb, 0x4b, 0x8b, 0x0b,
	0xf3, 0x73, 0xb3, 0x33, 0xd3, 0x53, 0x93, 0x13,
	0xe3, 0x63, 0xa3, 0x23, 0xc3, 0x43, 0x83, 0x03,
	0xfd, 0x7d, 0xbd, 0x3d, 0xdd, 0x5d, 0x9d, 0x1d,
	0xed, 0x6d, 0xad, 0x2d, 0xcd, 0x4d, 0x8d, 0x0d,
	0xf5, 0x75, 0xb5, 0x35, 0xd5, 0x55, 0x95, 0x15,
	0xe5, 0x65, 0xa5, 0x25, 0xc5, 0x45, 0x85, 0x05,
	0xf9, 0x79, 0xb9, 0x39, 0xd9, 0x59, 0x99, 0x19,
	0xe9, 0x69, 0xa9, 0x29, 0xc9, 0x49, 0x89, 0x09,
	0xf1, 0x71, 0xb1, 0x31, 0xd1, 0x51, 0x91, 0x11,
	0xe1, 0x61, 0xa1, 0x21, 0xc1, 0x41, 0x81, 0x01,
	0xfe, 0x7e, 0xbe, 0x3e, 0xde, 0x5e, 0x9e, 0x1e,
	0xee, 0x6e, 0xae, 0x2e, 0xce, 0x4e, 0x8e, 0x0e,
	0xf6, 0x76, 0xb6, 0x36, 0xd6, 0x56, 0x96, 0x16,
	0xe6, 0x66, 0xa6, 0x26, 0xc6, 0x46, 0x86, 0x06,
	0xfa, 0x7a, 0xba, 0x3a, 0xda, 0x5a, 0x9a, 0x1a,
	0xea, 0x6a, 0xaa, 0x2a, 0xca, 0x4a, 0x8a, 0x0a,
	0xf2, 0x72, 0xb2, 0x32, 0xd2, 0x52, 0x92, 0x12,
	0xe2, 0x62, 0xa2, 0x22, 0xc2, 0x42, 0x82, 0x02,
	0xfc, 0x7c, 0xbc, 0x3c, 0xdc, 0x5c, 0x9c, 0x1c,
	0xec, 0x6c, 0xac, 0x2c, 0xcc, 0x4c, 0x8c, 0x0c,
	0xf4, 0x74, 0xb4, 0x34, 0xd4, 0x54, 0x94, 0x14,
	0xe4, 0x64, 0xa4, 0x24, 0xc4, 0x44, 0x84, 0x04,
	0xf8, 0x78, 0xb8, 0x38, 0xd8, 0x58, 0x98, 0x18,
	0xe8, 0x68, 0xa8, 0x28, 0xc8, 0x48, 0x88, 0x08,
	0xf0, 0x70, 0xb0, 0x30, 0xd0, 0x50, 0x90, 0x10,
	0xe0, 0x60, 0xa0, 0x20, 0xc0, 0x40, 0x80, 0x00
};

static const UINT16 udk_ptr[12] = {
	0x0000, 0x002a, 0x0054, 0x007e, 0x00a8, 0x00d2,
	0x0100, 0x012a, 0x0154, 0x017e, 0x01a8, 0x01d2
};

static const UINT8 t6834_cmd_len[0x47] = {
	0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x04, 0x03,
	0x01, 0x02, 0x09, 0x01, 0x09, 0x01, 0x01, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x05, 0x04, 0x82, 0x02,
	0x01, 0x01, 0x0a, 0x02, 0x01, 0x81, 0x81, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x03,
	0x02, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x82, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x09, 0x01, 0x03, 0x03, 0x01, 0x01
};

static const UINT8 x07_keycodes[6*7][8] =
{
	/* normal */
	{0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38},
	{0x39, 0x30, 0x2d, 0x5e, 0x51, 0x57, 0x45, 0x52},
	{0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0x40, 0x5b},
	{0x41, 0x53, 0x44, 0x46, 0x47, 0x48, 0x4a, 0x4b},
	{0x4c, 0x3b, 0x3a, 0x5d, 0x5a, 0x58, 0x43, 0x56},
	{0x42, 0x4e, 0x4d, 0x2c, 0x2e, 0x2f, 0x3f, 0x20},
	{0x1d, 0x1c, 0x1e, 0x1f, 0x16, 0x0b, 0x0d, 0x12},

	/* shift */
	{0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28},
	{0x29, 0x7c, 0x3d, 0x7e, 0x71, 0x77, 0x65, 0x72},
	{0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x60, 0x7b},
	{0x61, 0x73, 0x64, 0x66, 0x67, 0x68, 0x6a, 0x6b},
	{0x6c, 0x2b, 0x2a, 0x7d, 0x7a, 0x78, 0x63, 0x76},
	{0x62, 0x6e, 0x6d, 0x3c, 0x3e, 0x3f, 0xa5, 0x20},
	{0x1d, 0x1c, 0x1e, 0x1f, 0x16, 0x0b, 0x0d, 0x12},

	/* ctrl */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x11, 0x17, 0x05, 0x12},
	{0x14, 0x19, 0x15, 0x09, 0x0f, 0x10, 0x00, 0x00},
	{0x01, 0x13, 0x04, 0x06, 0x07, 0x08, 0x0a, 0x0b},
	{0x0c, 0x00, 0x00, 0x00, 0x1a, 0x18, 0x03, 0x16},
	{0x02, 0x0e, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x20},
	{0x1d, 0x1c, 0x1e, 0x1f, 0x16, 0x0b, 0x0d, 0x12},

	/* kana */
	{0xc7, 0xcc, 0xb1, 0xb3, 0xb4, 0xb5, 0xd4, 0xd5},
	{0xd6, 0xdc, 0xce, 0xcd, 0xc0, 0xc3, 0xb2, 0xbd},
	{0xb6, 0xdd, 0xc5, 0xc6, 0xd7, 0xbe, 0xde, 0xdf},
	{0xc1, 0xc4, 0xbc, 0xca, 0xb7, 0xb8, 0xcf, 0xc9},
	{0xd8, 0xda, 0xb9, 0xd1, 0xc2, 0xbb, 0xbf, 0xcb},
	{0xba, 0xd0, 0xd3, 0xc8, 0xd9, 0xd2, 0x00, 0x20},
	{0x1d, 0x1c, 0x1e, 0x1f, 0x16, 0x0b, 0x0d, 0x12},

	/*  shift + kana */
	{0x00, 0x00, 0xa7, 0xa9, 0xaa, 0xab, 0xac, 0xad},
	{0xae, 0xa6, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa2},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0xa3, 0xaf, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0xa4, 0xa1, 0xa5, 0x00, 0x20},
	{0x1d, 0x1c, 0x1e, 0x1f, 0x16, 0x0b, 0x0d, 0x12},

	 /*  graph */
	{0xe9, 0x90, 0x91, 0x92, 0x93, 0xec, 0xe0, 0xf2},
	{0xf1, 0x8a, 0xf0, 0xfc, 0x8b, 0xfb, 0x99, 0xf6},
	{0x97, 0x96, 0x94, 0xf9, 0x9e, 0xf7, 0xe7, 0x84},
	{0x88, 0x9f, 0xef, 0xfd, 0x9d, 0xfe, 0xe5, 0x9b},
	{0xf4, 0x82, 0x81, 0x85, 0xe1, 0x98, 0xe4, 0x95},
	{0xed, 0x89, 0xf5, 0x9c, 0x9a, 0x80, 0x00, 0x20},
	{0x1d, 0x1c, 0x1e, 0x1f, 0x16, 0x0b, 0x0d, 0x12}
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static PALETTE_INIT( x07 );
static VIDEO_UPDATE( x07 );
static READ8_HANDLER( x07_IO_r );
static WRITE8_HANDLER( x07_IO_w );
static NVRAM_HANDLER( x07 );
static TIMER_CALLBACK( irq_clear );
static TIMER_CALLBACK( beep_stop );
static TIMER_CALLBACK( keyboard_clear );
static TIMER_DEVICE_CALLBACK( keyboard_poll );
static MACHINE_START( x07 );
static MACHINE_RESET( x07 );
