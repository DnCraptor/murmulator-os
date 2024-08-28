#include "m-os-api.h"
// TODO:
//#define HASKEYBOARD

//--------------------------------------------------------
// TODO: support for static libraries in M-OS
//--------------------------------------------------------
#include "runtime.c"
//--------------------------------------------------------

/*----------------------------------------------------------------
 * Please read this before compiling: 
 *  - Review hardware.h for settings specific hardware settings.
 *      Super important on Arduino and Raspberry PI.
 *  - language.h controls the language features.
 *      For Arduino Integer BASIC is default and a limited language 
 *      set. For the larger boards this can be extended a lot.
 *-----------------------------------------------------------------
 *
 *	$Id: basic.c,v 1.5 2024/03/02 15:38:20 stefan Exp stefan $ 
 *
 *	Stefan's IoT BASIC interpreter - BASIC for everywhere.
 *
 * 	See the licence file on 
 *	https://github.com/slviajero/tinybasic for copyright/left.
 *    (GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007)
 *
 *	Author: Stefan Lenz, sl001@serverfabrik.de
 *
 *	Currently there are two versions of the runtime environment.
 *		One contains all platforms compiled in the Arduino IDE
 *		(ESP8266, ESP32, AVR, MEGAAVR, SAM*, RP2040)
 *
 *		Anothers contains all platforms compiled in gcc with a POSIX OS
 *		(Mac, Raspberry, Windows/MINGW) plus rudimentary MSDOS with tc2.0. The 
 *		latter will be removed soon.
 * 
 * 	The interface to BASIC is identical. 
 */

/* the runtime environment */
#include "hardware.h"
#include "runtime.h"

/*
 * the core basic language headers 
 */
#include "language.h"
#include "basic.h"

/* use long jump for error handling */
#if USELONGJUMP == 1
#include "setjmp.h"
#endif

/* Global BASIC definitions */

/*
 *	All BASIC keywords for the tokens, PROGMEM on Arduino
 *  Normal memory elsewhere. 
 */
const char sge[]   PROGMEM = "=>";
const char sle[]   PROGMEM = "<=";
const char sne[]   PROGMEM = "<>";
/* Palo Alto language set */
const char sprint[]  PROGMEM = "PRINT";
const char slet[]    PROGMEM = "LET";
const char sinput[]  PROGMEM = "INPUT";
const char sgoto[]   PROGMEM = "GOTO";
const char sgosub[]  PROGMEM = "GOSUB";
const char sreturn[] PROGMEM = "RETURN";
const char sif[]     PROGMEM = "IF";
const char sfor[]    PROGMEM = "FOR";
const char sto[]     PROGMEM = "TO";
const char sstep[]   PROGMEM = "STEP";
const char snext[]   PROGMEM = "NEXT";
const char sstop[]   PROGMEM = "STOP";
const char slist[]   PROGMEM = "LIST";
const char snew[]    PROGMEM = "NEW";
const char srun[]  	 PROGMEM = "RUN";
const char sabs[]    PROGMEM = "ABS";
const char srnd[]    PROGMEM = "RND";
const char ssize[]   PROGMEM = "SIZE";
const char srem[]    PROGMEM = "REM";
/* Apple 1 language set */
#ifdef HASAPPLE1
const char snot[]    PROGMEM = "NOT";
const char sand[]    PROGMEM = "AND";
const char sor[]     PROGMEM = "OR";
const char slen[]    PROGMEM = "LEN";
const char ssgn[]    PROGMEM = "SGN";
const char speek[]   PROGMEM = "PEEK";
const char sdim[]    PROGMEM = "DIM";
const char sclr[]    PROGMEM = "CLR";
const char shimem[]  PROGMEM = "HIMEM";
const char stab[]    PROGMEM = "TAB";
const char sthen[]   PROGMEM = "THEN";
const char sbend[]   PROGMEM = "END";
const char spoke[]   PROGMEM = "POKE";
#endif
/* Stefan's basic additions */
#ifdef HASSTEFANSEXT
const char scont[]   PROGMEM = "CONT";
const char ssqr[]    PROGMEM = "SQR";
const char spow[]    PROGMEM = "POW";
const char smap[]    PROGMEM = "MAP";
const char sdump[]   PROGMEM = "DUMP";
const char sbreak[]  PROGMEM = "BREAK";
#endif
/* LOAD and SAVE is always there */
const char ssave[]   PROGMEM = "SAVE";
const char sload[]   PROGMEM = "LOAD";
#ifdef HASSTEFANSEXT
const char sget[]    PROGMEM = "GET";
const char sput[]    PROGMEM = "PUT";
const char sset[]    PROGMEM = "SET";
const char scls[]    PROGMEM = "CLS";
const char slocate[]  PROGMEM  = "LOCATE";
const char selse[]  PROGMEM  = "ELSE";
#endif
/* Arduino functions */
#ifdef HASARDUINOIO
const char spinm[]    PROGMEM = "PINM";
const char sdwrite[]  PROGMEM = "DWRITE";
const char sdread[]   PROGMEM = "DREAD";
const char sawrite[]  PROGMEM = "AWRITE";
const char saread[]   PROGMEM = "AREAD";
const char sdelay[]   PROGMEM = "DELAY";
const char smillis[]	PROGMEM = "MILLIS";
const char sazero[]		PROGMEM = "AZERO";
const char sled[]		PROGMEM = "LED";
#endif
#ifdef HASTONE
const char stone[]	PROGMEM = "PLAY";
#endif
#ifdef HASPULSE
const char spulse[]	PROGMEM = "PULSE";
#endif
/* DOS functions */
#ifdef HASFILEIO
const char scatalog[]	PROGMEM = "CATALOG";
const char sdelete[]	PROGMEM = "DELETE";
const char sfopen[]		PROGMEM = "OPEN";
const char sfclose[]	PROGMEM = "CLOSE";
const char sfdisk[]		PROGMEM = "FDISK";
#endif
/* low level access functions */
#ifdef HASSTEFANSEXT
const char susr[]   PROGMEM = "USR";
const char scall[]  PROGMEM = "CALL";
#endif
/* mathematics */
#ifdef HASFLOAT
const char ssin[]   PROGMEM = "SIN";
const char scos[]   PROGMEM = "COS";
const char stan[]   PROGMEM = "TAN";
const char satan[]  PROGMEM = "ATAN";
const char slog[]   PROGMEM = "LOG";
const char sexp[]   PROGMEM = "EXP";
#endif
/* INT is always needed to make float/int programs compatible */
const char sint[]   PROGMEM = "INT"; 
/* elemetars graphics */
#ifdef HASGRAPH
const char scolor[]     PROGMEM  = "COLOR";
const char splot[]      PROGMEM  = "PLOT";
const char sline[]      PROGMEM  = "LINE";
const char scircle[]    PROGMEM  = "CIRCLE";
const char srect[]      PROGMEM  = "RECT";
const char sfcircle[]   PROGMEM  = "FCIRCLE";
const char sfrect[]     PROGMEM  = "FRECT";
#endif
/* Dartmouth BASIC extensions */
#ifdef HASDARTMOUTH
const char sdata[]      PROGMEM  = "DATA";
const char sread[]      PROGMEM  = "READ";
const char srestore[]   PROGMEM  = "RESTORE";
const char sdef[]       PROGMEM  = "DEF";
const char sfn[]        PROGMEM  = "FN";
const char son[]        PROGMEM  = "ON";
#endif
/* The Darkarts commands */
#ifdef HASDARKARTS
const char smalloc[]	PROGMEM  = "MALLOC";
const char sfind[]		PROGMEM  = "FIND";
const char seval[]		PROGMEM  = "EVAL";
#endif
/* complex error handling */
#ifdef HASERRORHANDLING
const char serror[]     PROGMEM  = "ERROR";
#endif
/* iot extensions */
#ifdef HASIOT
const char savail[]		PROGMEM  = "AVAIL";
const char sstr[]		PROGMEM  = "STR";
const char sinstr[]		PROGMEM  = "INSTR";
const char sval[]		PROGMEM  = "VAL";
const char snetstat[]	PROGMEM  = "NETSTAT";
const char ssensor[]	PROGMEM  = "SENSOR";
const char swire[]		PROGMEM  = "WIRE";
const char ssleep[]		PROGMEM  = "SLEEP";
#endif
/* events and interrupts */
#ifdef HASTIMER
const char safter[]     PROGMEM  = "AFTER";
const char severy[]     PROGMEM  = "EVERY";
#endif
#ifdef HASEVENTS
const char sevent[]     PROGMEM  = "EVENT";
#endif
#ifdef HASSTRUCT
const char swhile[]		PROGMEM	= "WHILE";
const char swend[]      PROGMEM = "WEND";
const char srepeat[]	PROGMEM	= "REPEAT";
const char suntil[]     PROGMEM	= "UNTIL";
const char sswitch[]	PROGMEM	= "SWITCH";
const char scase[]		PROGMEM	= "CASE";
const char sswend[]     PROGMEM = "SWEND";
const char sdo[]        PROGMEM = "DO";
const char sdend[]      PROGMEM = "DEND";
#endif
#ifdef HASDARTMOUTH
#ifdef HASMULTILINEFUNCTIONS
const char sfend[]      PROGMEM = "FEND";
#endif
#endif
#ifdef HASMSSTRINGS
const char sasc[]		PROGMEM	= "ASC";
const char schr[]		PROGMEM = "CHR";
const char sright[]		PROGMEM = "RIGHT";
const char sleft[]		PROGMEM = "LEFT";
const char smid[]		PROGMEM = "MID";
const char sspc[]		PROGMEM = "SPC";
#endif
#ifdef HASEDITOR
const char sedit[]		PROGMEM = "EDIT";
#endif


/* zero terminated keyword storage */
const char* const keyword[] PROGMEM = {
	sge, sle, sne, sprint, slet, sinput, 
	sgoto, sgosub, sreturn, sif, sfor, sto,
	sstep, snext, sstop, slist, snew, srun,
	sabs, srnd, ssize, srem,
#ifdef HASAPPLE1
	snot, sand, sor, slen, ssgn, speek, sdim,
	sclr, shimem, stab, sthen, 
	sbend, spoke,
#endif
#ifdef HASSTEFANSEXT
	scont, ssqr, spow, smap, sdump, sbreak, 
#endif
	ssave, sload, 
#ifdef HASSTEFANSEXT
	sget, sput, sset, scls, slocate, selse,
#endif
#ifdef HASARDUINOIO
    spinm, sdwrite, sdread, sawrite, saread, 
    sdelay, smillis, sazero, sled,
#endif
#ifdef HASTONE
	stone,
#endif
#ifdef HASPULSE
	spulse,
#endif
#ifdef HASFILEIO
    scatalog, sdelete, sfopen, sfclose, sfdisk,
#endif
#ifdef HASSTEFANSEXT
    susr, scall,
#endif
#ifdef HASFLOAT
    ssin, scos, stan, satan, slog, sexp,
#endif
    sint,
#ifdef HASGRAPH
    scolor, splot, sline, scircle, srect, 
    sfcircle, sfrect,
#endif
#ifdef HASDARTMOUTH
	sdata, sread, srestore, sdef, sfn, son,
#endif
#ifdef HASDARKARTS
	smalloc, sfind, seval, 
#endif
/* complex error handling */
#ifdef HASERRORHANDLING
    serror,
#endif
#ifdef HASIOT
	savail, sstr, sinstr, sval, 
	snetstat, ssensor, swire, ssleep, 
#endif
#ifdef HASTIMER
    safter, severy,
#endif
#ifdef HASEVENTS
    sevent,
#endif
#ifdef HASSTRUCT
	swhile, swend, srepeat, suntil, sswitch, scase, sswend,	
    sdo, sdend, 
#endif 
#ifdef HASDARTMOUTH
#ifdef HASMULTILINEFUNCTIONS
	sfend,
#endif
#endif
#ifdef HASMSSTRINGS
    sasc, schr, sright, sleft, smid, sspc,
#endif
#ifdef HASEDITOR
	sedit,
#endif
	0
};

/* the zero terminated token dictonary needed for scalability */
const token_t tokens[] PROGMEM = {
	GREATEREQUAL, LESSEREQUAL, NOTEQUAL, TPRINT, TLET,    
    TINPUT, TGOTO, TGOSUB, TRETURN, TIF, TFOR, TTO, TSTEP,
    TNEXT, TSTOP, TLIST, TNEW, TRUN, TABS, TRND, TSIZE, TREM,
#ifdef HASAPPLE1
    TNOT, TAND, TOR, TLEN, TSGN, TPEEK, TDIM, TCLR,
    THIMEM, TTAB, TTHEN, TEND, TPOKE,
#endif
#ifdef HASSTEFANSEXT
	TCONT, TSQR, TPOW, TMAP, TDUMP, TBREAK, 
#endif
	TSAVE, TLOAD, 
#ifdef HASSTEFANSEXT	
	TGET, TPUT, TSET, TCLS, TLOCATE, TELSE,
#endif
#ifdef HASARDUINOIO
	TPINM, TDWRITE, TDREAD, TAWRITE, TAREAD, TDELAY, TMILLIS,
	TAZERO, TLED,
#endif
#ifdef HASTONE
	TTONE, 
#endif
#ifdef HASPULSE
	TPULSE, 
#endif
#ifdef HASFILEIO
	TCATALOG, TDELETE, TOPEN, TCLOSE, TFDISK,
#endif
#ifdef HASSTEFANSEXT
	TUSR, TCALL,
#endif
#ifdef HASFLOAT
	TSIN, TCOS, TTAN, TATAN, TLOG, TEXP,
#endif
	TINT,
#ifdef HASGRAPH
	TCOLOR, TPLOT, TLINE, TCIRCLE, TRECT, 
	TFCIRCLE, TFRECT,
#endif
#ifdef HASDARTMOUTH
	TDATA, TREAD, TRESTORE, TDEF, TFN, TON, 
#endif
#ifdef HASDARKARTS
	TMALLOC, TFIND, TEVAL, 
#endif
#ifdef HASERRORHANDLING
    TERROR, 
#endif
#ifdef HASIOT
	TAVAIL, TSTR, TINSTR, TVAL, TNETSTAT,
	TSENSOR, TWIRE, TSLEEP,
#endif
#ifdef HASTIMER
    TAFTER, TEVERY,
#endif
#ifdef HASEVENTS
    TEVENT, 
#endif
#ifdef HASSTRUCT
	TWHILE, TWEND, TREPEAT, TUNTIL, TSWITCH, TCASE, TSWEND,
    TDO, TDEND,
#endif
#ifdef HASDARTMOUTH
#ifdef HASMULTILINEFUNCTIONS
	TFEND,
#endif
#endif
#ifdef HASMSSTRINGS
	TASC, TCHR, TRIGHT, TLEFT, TMID, TSPC,
#endif
#ifdef HASEDITOR
	TEDIT,
#endif
	0
};

/* experimental, do not use right now */
const bworkfunction_t workfunctions[] PROGMEM = { 
	0, 0, 0, xprint, 0
};

/* errors and messages */
const char mfile[]    	PROGMEM = "file.bas";
const char mprompt[]	PROGMEM = "> ";
const char mgreet[]		PROGMEM = "Stefan's Basic 2.0";
const char mline[]		PROGMEM = "LINE";
const char mnumber[]	PROGMEM = "NUMBER";
const char mvariable[]	PROGMEM = "VARIABLE";
const char marray[]		PROGMEM = "ARRAY";
const char mstring[]	PROGMEM = "STRING";
const char mstringv[]	PROGMEM = "STRINGVAR";
const char egeneral[]  	PROGMEM = "Error";
#ifdef HASERRORMSG
const char eunknown[]  	PROGMEM = "Syntax";
const char enumber[]	PROGMEM = "Number";
const char edivide[]  	PROGMEM = "Div by 0";
const char eline[]  	PROGMEM = "Unknown Line";
const char emem[]  	   	PROGMEM = "Memory";
const char estack[]    	PROGMEM = "Stack";
const char erange[]  	PROGMEM = "Range";
const char estring[]	PROGMEM = "String";
const char evariable[]  PROGMEM = "Variable";
const char eloop[]      PROGMEM = "Loop";
const char efile[]  	PROGMEM = "File";
const char efun[] 	 	PROGMEM = "Function";
const char eargs[]  	PROGMEM = "Args";
const char eeeprom[]	PROGMEM = "EEPROM";
const char esdcard[]	PROGMEM = "SD card";
#endif

const char* const message[] PROGMEM = {
	mfile, mprompt, mgreet, 
	mline, mnumber, mvariable, marray, 
	mstring, mstringv,
	egeneral
#ifdef HASERRORMSG
	, eunknown, enumber, edivide, eline, 
	emem, estack, erange,
	estring, evariable, eloop, efile, efun, eargs, 
	eeeprom, esdcard
#endif
};

/*
 *	maxnum: the maximum accurate(!) integer of a 
 *		32 bit float 
 *	strindexsize: in the new code this is simply the size of the 
 * 		stringlength type. Currently only 1 byte and 2 bytes are tested.
 */
#ifdef HASFLOAT
const number_t maxnum = 16777216; 
#else
const number_t maxnum=(number_t)~((number_t)1<<(sizeof(number_t)*8-1));
#endif
const int numsize = sizeof(number_t);
const int addrsize = sizeof(address_t);
const int eheadersize = sizeof(address_t) + 1;
const int strindexsize = sizeof(stringlength_t); /* default in the meantime, strings up to unsigned 16 bit length */
const address_t maxaddr = (address_t)(~0); 

/*
 *	The basic interpreter is implemented as a stack machine
 *	with global variable for the interpreter state, the memory
 *	and the arithmetic during run time.
 */

/* the stack, all BASIC arithmetic is done here */
//number_t stack[STACKSIZE];
static accu_t stack[STACKSIZE];
static address_t sp = 0;

/* a small buffer to process string arguments, mostly used for Arduino PROGMEM and string functions */
/* use with care as it is used in some string functions */
static char sbuffer[SBUFSIZE];

/* the input buffer, the lexer can tokenize this and run from it, bi is an index to this.
   bi must be global as it is the program cursor in interactive mode */
char ibuffer[BUFSIZE] = "\0";
static char *bi;

/* a static array of variables A-Z for the small systems that have no heap */
#ifndef HASAPPLE1
number_t vars[VARSIZE];
#endif

/* the BASIC working memory, either malloced or allocated as a global array */
#if MEMSIZE != 0
mem_t mem[MEMSIZE];
#else
mem_t* mem;
#endif
address_t himem, memsize;

/* reimplementation of the loops, will replace the forstack */
bloop_t loopstack[FORDEPTH];
index_t loopsp = 0;
 
/* the GOSUB stack remembers an address to jump to */
address_t gosubstack[GOSUBDEPTH];
index_t gosubsp = 0;

/* arithmetic accumulators */
number_t x, y;

/* the name of on object, replaced xc and xy in BASIC 1 */
name_t name;

/* an address accumulator, used a lot in string operations */
address_t ax;

/* a string index registers, new style identifying a string either in C memory or BASIC memory */
bstring_t sr; 

/* the active token */
token_t token;

/* the curent error, can be a token, hence token type */
token_t er;
/* the jmp buffer for the error handling */
#if USELONGJUMP == 1
jmp_buf sthook;
#endif

/* a trapable error */
mem_t ert;

/* the interpreter state, interactive, run or run from EEPROM */
mem_t st; 

/* the current program location */
address_t here; 

/* the topmost byte of a program in memory, beginning of free BASIC RAM */
address_t top;

/* used to format output with # */
mem_t form = 0;

/* do we use the Microsoft convention of an array starting at 0 or 1 like Apple 1 
	two seperate variables because arraylimit can be changed at runtime for existing arrays 
	msarraylimit says if an array should be created with n or n+1 elements */
#ifdef MSARRAYLIMITS
mem_t msarraylimits = 1;
address_t arraylimit = 0;
#else
mem_t msarraylimits = 0;
address_t arraylimit = 1;
#endif


/* behaviour around boolean, needed to change the interpreters personality at runtime */
/* -1 is microsoft true while 1 is Apple 1 and C style true. */
mem_t booleanmode = BOOLEANMODE;

/* setting the interpreter to integer at runtime */
mem_t forceint = 0;

/* the default size of a string now as a variable */
stringlength_t defaultstrdim = STRSIZEDEF;

/* the base of the random number generator 
 *	0 is Apple 1 style RND from 0 to n-epsilon 
 *	1 is Palo Alto style from 1 to n 
 */
mem_t randombase = 0;

/* is substring logic used or not */
#ifdef SUPPRESSSUBSTRINGS
mem_t substringmode = 0;
#else 
mem_t substringmode = 1;
#endif

/* the flag for true MS tabs */
mem_t reltab = 0;

/* the flag for lower case names */
mem_t lowercasenames = 0;

/* the number of arguments parsed from a command */
mem_t args;

/* the random number seed, this is unsigned */
#ifndef HASFLOAT
address_t rd;
#else 
unsigned long rd;
#endif

/* the RUN debuglevel */
mem_t debuglevel = 0;

/* DATA pointer, where is the current READ statement  */
#ifdef HASDARTMOUTH
address_t data = 0;
address_t datarc = 1;
#endif
    
/* 
 * process command line arguments in the POSIX world 
 * bnointafterrun is a flag to remember if called as command
 * line argument, in this case we don't return to interactive 
 */
#ifdef HASARGS
int bargc;
char** bargv;
mem_t bnointafterrun = 0;
#endif

/* formaters lastouttoken and spaceafterkeyword to make a nice LIST */
mem_t lastouttoken;
mem_t spaceafterkeyword;
mem_t outliteral = 0;
mem_t lexliteral = 0; 

/* 
 * The cache for the heap search - helps the string code.
 * The last found object on the heap is remembered. This is needed
 * because the string code sometime searches the heap twice during the 
 * same operation. Also, bfind is used to remember the length of the
 * last found object. 
 */
#ifdef HASAPPLE1
heap_t bfind_object;
#endif

/*
 * a variable for string to numerical conversion, 
 * telling you were the number ended. 
 */
address_t vlength;

/* the timer code - very simple needs to be converted to to a struct */
/* timer type */
#ifdef HASTIMER
btimer_t after_timer = {0, 0, 0, 0, 0};
btimer_t every_timer = {0, 0, 0, 0, 0};
#endif

/* the event code */
#ifdef HASEVENTS

#define EVENTLISTSIZE 4

/* the event list */
static int nevents = 0;
static int ievent = 0;
static mem_t events_enabled = 1;
static volatile bevent_t eventlist[EVENTLISTSIZE] = { 0 };

/* the extension of the GOSUB stack */
mem_t gosubarg[GOSUBDEPTH];
#endif

#ifdef HASERRORHANDLING
/* the error handler type, very simple for now */
typedef struct {
    mem_t type;
    address_t linenumber;
} berrorh_t;

berrorh_t berrorh = {0 , 0};
mem_t erh = 0;
#endif

/* the string for real time clocks */
char rtcstring[20] = { 0 }; 

/* the units pulse operates on, in microseconds*/
address_t bpulseunit = 10; 

/* only needed for POSIXNONBLOCKING */
mem_t breakcondition = 0;

/* the FN context, how deep are we in a nested function call, negative values reserved */
int fncontext = 0; 

/* the accuracy of a equal or not equal statement on numbers */
#ifdef HASFLOAT
number_t epsilon = 0;
#else 
const number_t epsilon = 0;
#endif

/*
 * BASIC timer stuff, this is a core interpreter function now
 */

/* the millis function for BASIC */
inline static void bmillis() {
	number_t m;
/* millis is processed as integer and is cyclic mod maxnumber and not cast to float!! */
	m = (number_t) (millis() / (unsigned long)pop() % (unsigned long)maxnum);
	push(m); 
}

/*
 *  Determine the possible basic memory size.
 *	using malloc causes some overhead which can be relevant on the smaller  
 *	boards. 
 *
 * 	Set MEMSIZE instead to a static value. In this case ballocmem 
 *	just returns the static MEMSIZE.
 * 
 *	If SPIRAMINTERFACE is defined, we use the memory from a serial RAM and dont 
 *	allocate it here at all.
 *
 */
#if MEMSIZE == 0 && !(defined(SPIRAMINTERFACE))
static address_t ballocmem() {
	/* on most platforms we know the free memory for BASIC, this comes from runtime */
	long m = freememorysize();
	/* we subtract some language feature depended things, this is only needed on
	small Arduino boards with memories below 16kb */
	if (m < 16000) {
#ifdef HASAPPLE1
		m -= 64; /* strings cost memory */
#endif
#ifdef USELONGJUMP
		m -= 160; /* odd but true on Ardunio UNO and the like */
#endif
#ifdef HASFLOAT 
	  	m -= 96;
#endif
#ifdef HASGRAPH
  		m -= 256; 
#endif		
	}
	/* and keep fingers crossed here */
	if (m < 0) m = 128;
	/* we allocate as much as address_t can handle */
	if (m > maxaddr) m = maxaddr;
	/* try to allocate the memory */
	mem = (mem_t*)malloc(m);
	if (mem != 0) return m - 1;
	/* fallback if allocation failed, 128 bytes */
	mem = (mem_t*)malloc(128);
	if (mem != 0) return 128;
	else return 0;
}
#else 
inline static address_t ballocmem(){ return MEMSIZE - 1; };
#endif

/*
 *	Layer 0 function - variable handling.
 *
 *	These function access variables and data
 */

/*
 *	Eeprom load / save / autorun functions 
 *	needed for SAVE and LOAD to an EEPROM
 *	autorun is generic.
 * 
 *	The eeprom is accessed through the runtime functions 
 * 	elength(), eupdate(), ewrite() and eflush();
 * 	
 */

inline static void beupdate(address_t a, mem_t v) {
	eupdate(a, v);
}

/* save a file to EEPROM, disabled if we use the EEPROM directly */
inline static void esave() {
#ifndef EEPROMMEMINTERFACE
	address_t a=0;
	
	/* does the program fit into the eeprom */
	if (top+eheadersize < elength()) {

		/* EEPROM per default is 255, 0 indicates that there is a program */
		eupdate(a++, 0); 

		/* store the size of the program in byte 1,2,... of the EEPROM*/
		setaddress(a, beupdate, top);
		a+=addrsize;

		/* store the program */
		while (a < top+eheadersize){
			eupdate(a, memread2(a-eheadersize));
			a++;
		}
		eupdate(a++,0);

		/* needed on I2C EEPROM and other platforms where we buffer */
		eflush();

	} else {
		error(EOUTOFMEMORY);
		er=0;
	}
#endif
}

/* load a file from EEPROM, disabled if the use the EEPROM directly */
inline static void eload() {
#ifndef EEPROMMEMINTERFACE
	address_t a=0;

	/* have we stored a program? */
	if (elength()>0 && (eread(a) == 0 || eread(a) == 1)) { 

		/* how long is it? */
		a++;
		top=getaddress(a, beread);
		a+=addrsize;

		/* load it to memory, memwrite2 is direct mem access */
		while (a < top+eheadersize){
			memwrite2(a-eheadersize, beread(a));
			a++;
		}
	} else { 
		/* no valid program data is stored */
		error(EEEPROM);
	}
#endif
}

/* autorun something from EEPROM or a filesystem */
inline static char autorun() {
/* autorun from EEPROM if there is an EEPROM flagged for autorun */	
	if (elength() > 0 && eread(0) == 1) { /* autorun from the EEPROM */
		top = getaddress(1, beread);
  		st = SERUN;
  		return 1; /* EEPROM autorun overrules filesystem autorun */
	} 
/* autorun from a given command line argument, if we have one */
#ifdef HASARGS
	if (bargc > 0 && ifileopen(bargv[1])) {
		xload(bargv[1]);
		st = SRUN;
		ifileclose();
		bnointafterrun = TERMINATEAFTERRUN;
		return 1;			
	}
#endif
/* on a platform with a file system, autoexec from a file */
#if defined(FILESYSTEMDRIVER) 
	if (ifileopen("autoexec.bas")) {
		xload("autoexec.bas");
  		st=SRUN;
		ifileclose();
		return 1;
	}
#endif
/* nothing to autorun */
	return 0;
}

#ifdef HASAPPLE1
/* 
 * The new malloc code. Heap structure is now 
 * payload
 * (payload size)
 * name
 * type
 *
 * In the new heap implementation himem points to the first free byte on the heap.
 * The payload is stored first and then the header. 
 * 
 * The new heap code uses name_t for the name of the object.
 * 
 */

static address_t bmalloc(name_t* name, address_t l) {
	address_t payloadsize;     /* the payload size */
	address_t heapheadersize = sizeof(name_t) + addrsize; /* this is only used to estimate the free space, it is the maximum */
	address_t b=himem; /* the current position on the heap, we store it in case of errors */

	/* Initial DEBUG message. */
	#if DEBUG 
		outsc("** bmalloc with token "); 
		outnumber(name->token); outspc(); 
		outname(name); outspc(); 
		outnumber(l); outcr();
	#endif

/* 
 *	How much space does the payload of the object need? 
 */
	register token_t t = name->token;
	if ( t == VARIABLE ) { /* a variable needs numsize bytes*/
		payloadsize = numsize;
	}
#ifndef HASMULTIDIM
	else if ( t == ARRAYVAR ) { /* a one dimensional array needs numsize*l bytes */
		payloadsize = numsize * l;
	}
#else
	else if ( t == ARRAYVAR ) { /* a two dimensional array needs numsize*l bytes plus one word for the additional dimension*/
		payloadsize = numsize * l + addrsize;
	}
#endif
#ifdef HASDARTMOUTH
	else if ( t == TFN ) { /* the jump address, the type of function/type of return value, the number of vars 
			and all variables are stored*/
		payloadsize = addrsize + 2 + sizeof(name_t) * l; 
	}
#endif
/* these are plain buffers allocated by the MALLOC call in BASIC */
	 else {
		payloadsize = l;
	}

/* enough memory ?, on an EEPROM system we limit the heap to the RAM */
#ifndef EEPROMMEMINTERFACE
	if ((himem-top) < payloadsize+heapheadersize) { error(EOUTOFMEMORY); return 0;}	
#else
	if (himem-(elength()-eheadersize) < payloadsize+heapheadersize) { error(EOUTOFMEMORY); return 0;}
#endif

/* first we reserve space for the payload, address points to the first byte of the payload */
/* b points to the first free byte after the payload*/
	b-=payloadsize; 
	bfind_object.address=b+1;

/* for ARRAYS, STRINGS and BUFFERS, store the object length now - these are variable size objects*/
	if (name->token != VARIABLE) {
		b-=(addrsize-1);
		setaddress(b, memwrite2, payloadsize);
		if (DEBUG) { 
			outsc("** bmalloc writes payloadsize "); outnumber(payloadsize); 
			outsc(" at "); outnumber(b); outcr(); 
		}
		b--;
	}

/* store the name of the objects including the type as identifier */
	b=setname_heap(b, name);

/* store the type of the object */
	memwrite2(b--, name->token);

/* if anything went wrong we exit here without changing himem */
	if (b < top || er) { error(EOUTOFMEMORY); return 0; }

/* we fill the cache here as well, both right now for compatibility */
	bfind_object.name=*name;
	bfind_object.size=payloadsize;

/* himem is the next free byte now again */
	himem=b;

	if (DEBUG) { 
		outsc("** bmalloc returns "); outnumber(bfind_object.address); 
		outsc(" himem is "); outnumber(himem); outcr(); 
	}

/* return the address of the payload */
	return bfind_object.address;
}

static address_t bfind(name_t* name) {
	address_t b, b0;
	address_t i=0;

/* Initial DEBUG message. */
	if (DEBUG) { 
		outsc("*** bfind called for "); outname(name);
		outsc(" on heap with token "); outnumber(name->token); 
	 	outsc(" himem is "); outnumber(himem);  outcr(); 
	}

/* do we have anything on the heap? */
	if (himem == memsize) return 0; else b=himem+1;

/* we have the object already in cache and return */
	if (name->token == bfind_object.name.token && cmpname(name, &bfind_object.name)) {
		if (DEBUG) { outsc("*** bfind found in cache "); outname(name); outsc(" at "); outnumber(bfind_object.address); outcr(); }
		return bfind_object.address;
	}

/* walk through the heap from the last object added to the first */
	while (b <= memsize) {

/* get the name and the type */
		bfind_object.name.token=memread2(b++);
		b=getname(b, &bfind_object.name, memread2);

/* determine the size of the object and advance */
		if (bfind_object.name.token != VARIABLE) {
			bfind_object.size=getaddress(b, memread2);
			b+=addrsize;
		} else {
			bfind_object.size=numsize;
		}

/* this is the location of the payload */
		bfind_object.address=b;

/* have we found the object */	
		if (name->token == bfind_object.name.token && cmpname(name, &bfind_object.name)) {
			if (DEBUG) { outsc("*** bfind found "); outname(name); outsc(" at "); outnumber(bfind_object.address); outcr(); }
			return bfind_object.address;
		}	

/* advance on the heap */
		b0=b;
		b+=bfind_object.size;

/* safety net */
		if (b0 > b) {
			error(EVARIABLE);
			return 0;
		}

	}

/* nothing found return 0 and clear the cache */	

	if (DEBUG) { outsc("bfind returns 0"); outcr(); }
	zeroheap(&bfind_object);
	return 0;
}

/* reimplementation bfree with name interface */
static address_t bfree(name_t* name) {
	address_t b;
	address_t i;

	if (DEBUG) { outsc("*** bfree called for "); outname(name); outsc(" on heap with token "); outnumber(name->token); outcr(); }

/* use bfind to find the place */
	b=bfind(name);

/* nothing found, return 0 */
	if (b == 0) return 0;

	if (DEBUG) { outsc("** bfree found "); outnumber(b); outcr(); }

/* clear the entire memory area */
	for (i=himem; i<=b+bfind_object.size-1; i++) memwrite2(i, 0);

/* set the number of variables to the new value */
	himem=b+bfind_object.size-1;

	if (DEBUG) { outsc("** bfree returns "); outnumber(himem); outcr(); }

/* forget the chache, because heap structure has changed !! */
	zeroheap(&bfind_object);
	return himem;
}

/* the length of an object, we directly return from the cache */
inline static address_t blength(name_t* name) {
	if (bfind(name)) return bfind_object.size; else return 0;
}
#endif /* HASAPPLE1 */

/* reimplementation of getvar and setvar with name_t */
static number_t getvar(name_t *name){
	address_t a;

	if (DEBUG) { outsc("* getvar "); outname(name); outspc(); outcr(); }

/* the special variables */
	if (name->c[0] == '@') {
		register char c = name->c[1];
		if ( c == 'A' )	return availch();
		if ( c == 'S' )	return ert|ioer;
		if ( c == 'I' )	return id;
		if ( c == 'O' )	return od;
		if ( c == 'C' )	{ if (availch()) return inch(); else return 0; }
		if ( c == 'E' )	return elength() / numsize;
		if ( c ==  0  )	return (himem-top) / numsize;
		if ( c == 'R' )	return rd;
		if ( c == 'U' )	return getusrvar();
#ifdef HASFLOAT
		if ( c == 'P' )	return epsilon;
#endif
#ifdef HASIOT
		if ( c ==  'V')	return vlength;
#endif
#if defined(DISPLAYDRIVER) || defined (GRAPHDISPLAYDRIVER)
		if ( c == 'X' )	return dspgetcursorx();
		if ( c == 'Y' )	return dspgetcursory();
#endif
	}

#ifdef HASAPPLE1
/* search the heap first */
	a=bfind(name);
	if (!USELONGJUMP && er) return 0;

/* if we don't find on the heap and it is not a static variable, we autocreate */
	if (a == 0) {
		a=bmalloc(name, 0);
		if (!USELONGJUMP && er) return 0;
	}

/* something went wrong  */
	if (a == 0) {
		error(EVARIABLE);
		return 0;
	}
/* retrieve the value */
	return getnumber(a, memread2);

#else
/* we only have the static variable array */
	if (name->c[0] >= 65 && name->c[0] <= 91 && name->c[1] == 0) return vars[name->c[0]-65];

/* systems without Apple1 extension i.e. HEAP throw an error */
	error(EVARIABLE);
	return 0;
#endif
}

/* set and create a variable */
static void setvar(name_t *name, number_t v){
	address_t a;

	if (DEBUG) { outsc("* setvar "); outname(name); outspc(); outnumber(v); outcr(); }

/* the special variables */
	if (name->c[0] == '@') {
		register char c = name->c[1];
		if ( c == 'S' ) {
			ert = v;
        	ioer = v;
			return;
		}
		if ( c == 'I' ) {
			id = v;
			return;
		}
		if ( c == 'O' ) {
			od = v;
			return;
		}
		if ( c == 'C' ) {
			outch(v);
			return;
		}
		if ( c == 'R' ) {
			rd = v;
			return;
		}
		if ( c == 'U' ) {
			setusrvar(v);
			return;
		}
#ifdef HASFLOAT
		if ( c == 'P' ) {
			epsilon = v;
			return;
		}
#endif
#ifdef HASIOT
		if ( c == 'V' ) {
			return;
		}
#endif
#if defined(DISPLAYDRIVER) || defined(GRAPHDISPLAYDRIVER)
		if ( c == 'X' ) {
        	dspsetcursorx((int)v);
/* set the charcount, this is half broken but works */
#ifdef HASMSTAB
			if (od > 0 && od <= OPRT) charcount[od-1]=v;
#endif
			return;
		}
		if ( c == 'Y' ) {
			dspsetcursory((int)v);
			return;
		}
#endif
	}
#ifdef HASAPPLE1
/* dynamically allocated vars */
	a = bfind(name);

/* autocreate if not found */
	if (a == 0) {
		a=bmalloc(name, 0);
		if (!USELONGJUMP && er) return;
	}

/* something went wrong */
	if (a == 0) {
		error(EVARIABLE);
		return;
	}

/* set the value */
	setnumber(a, memwrite2, v);
#else 	
/* the static variable array */
	if (name->c[1] == 0 && name->c[0] >= 65 && name->c[0] <= 91) {
		vars[name->c[0]-65]=v;
		return;
	}
	error(EVARIABLE);
#endif
}

/* clr all variables */
static void clrvars() {

/* delete all static variables */
	address_t i;

/* clear static variable (only on no heap systems) */
#ifndef HASAPPLE1
	for (i=0; i<VARSIZE; i++) vars[i]=0;
#endif

/* then set the entire mem area to zero */
	for (i=himem; i<memsize; i++) memwrite2(i, 0);

/* reset the heap start*/
	himem=memsize;

/* and clear the cache */
#ifdef HASAPPLE1
	zeroheap(&bfind_object);
#endif
}

/* 
 * The BASIC memory access function.
 * 
 * getnumber2, getaddress and getstrlength are purely local, they
 * can be used with any memory reader function unlike the old BASIC 1 
 * code.
 */

/* 
 * To avoid nasty warnings we encapsulate the EEPROM access functions 
 *	from runtime.c* 
 * This is only needed for the get* and set* functions as we use the
 * memreader_t and memwriter_t function pointers here. 
 * This way, types in runtime.c can be changed without changing the
 * BASIC interpreter code.
 */
inline static mem_t beread(address_t a) {
	return eread(a);
}

/* a generic memory reader for numbers  */
static number_t getnumber(address_t m, memreader_t f) {
	mem_t i;
	accu_t z;

	for (i=0; i<numsize; i++) z.c[i]=f(m++);
	return z.n;
}

/* same for addresses */
static address_t getaddress(address_t m, memreader_t f) {
	mem_t i;
	accu_t z;

	for (i=0; i<addrsize; i++) z.c[i]=f(m++);
	return z.a;
}

/* same for strings  */
inline static stringlength_t getstrlength(address_t m, memreader_t f) {
	mem_t i;
	accu_t z;

	z.a=0;
	for (i=0; i<strindexsize; i++) z.c[i]=f(m++);
	return z.a;
}

/* set a number at a memory location, new version */
static void setnumber(address_t m, memwriter_t f, number_t v){
	mem_t i;
	accu_t z;

	z.n=v;
	for (i=0; i<numsize; i++) f(m++, z.c[i]);
}

/* set an address at a memory location */
static void setaddress(address_t m, memwriter_t f, address_t a){
	mem_t i;
	accu_t z;
	z.a = a;
	for (i=0; i<addrsize; i++)
		f(m++, z.c[i]);
}

/* set a stringlength at a memory location */
static void setstrlength(address_t m, memwriter_t f, stringlength_t s){
	mem_t i;
	accu_t z;
	z.s = s;
	for (i=0; i<strindexsize; i++)
		f(m++, z.c[i]);
}

/* 
 * Code to handle names. These function mostly deal with the true 
 * name part of name_t and not the token. The token has to be 
 * processed by the caller. Name byte order conventon is
 * to have the first character in the lower byte and the second
 * character in the higher byte. Optionally, for HASLONGNAMES 
 * the length of the name is stored before the name.
 *
 * setname_* sets a name and advance the number of bytes the name uses.
 * Two versions are needed because the heap is counted down
 * while the pgm is counted up. 
 * 
 * getname needs to go through a memreader because names are 
 * read from eeproms as well!
 * 
 * Currently the old code with twobyte names is still in place
 * if HASLONGNAMES is not defined. Default is now to have long names.
 * 
 */
#ifndef HASLONGNAMES

/* this one is for the heap were we count down writing*/
address_t setname_heap(address_t m, name_t* name) {
 	memwrite2(m--, name->c[1]);
	memwrite2(m--, name->c[0]);
	return m;
}

/* this one is for the pgm were we count up writing */
address_t setname_pgm(address_t m, name_t* name) {
	memwrite2(m++, name->c[0]);
	memwrite2(m++, name->c[1]);
	return m;
}

/* get a name from a memory location */
address_t getname(address_t m, name_t* name, memreader_t f) {
	name->c[0]=f(m++);
	name->c[1]=f(m++);
	return m;
}

/* compare two names */
mem_t cmpname(name_t* a, name_t* b) {
	if (a->c[0] == b->c[0] && a->c[1] == b->c[1]) return 1; else return 0;
}

/* zero a name and a heap object */
void zeroname(name_t* name) {
	name->c[0]=0;
	name->c[1]=0;
	name->token=0;
}

void zeroheap(heap_t* heap) {
	heap->address=0;
	heap->size=0;
	zeroname(&heap->name);
}

/* output a name */
void outname(name_t* name) {
	outch(name->c[0]);
	if (name->c[1]) outch(name->c[1]);
}

#else
/* this one is for the heap were we count down writing*/	
inline static address_t setname_heap(address_t m, name_t* name) {
	mem_t l;
	for(l=name->l; l>0; l--) memwrite2(m--, name->c[l-1]);
	memwrite2(m--, name->l);
	return m;
}

/* this one is for the pgm were we count up writing */
static address_t setname_pgm(address_t m, name_t* name) {
	mem_t l;
	memwrite2(m++, name->l);
	for(l=0; l<name->l; l++) memwrite2(m++, name->c[l]);
	return m;
}

/* get a name from a memory location */
static address_t getname(address_t m, name_t* name, memreader_t f) {
	mem_t l;
	name->l=f(m++);

	for(l=0; l<name->l; l++) name->c[l]=f(m++);
	for(; l<MAXNAME; l++) name->c[l]=0; /* should not be there, is needed for 
		now because the lexer is not implemented correctly*/
	return m;
}

/* compare two names */
static mem_t cmpname(name_t* a, name_t* b) {
	mem_t l;
	if (a->l != b->l) return 0;
	for(l=0; l<a->l; l++) if (a->c[l] != b->c[l]) return 0;
	return 1;
}	

/* zero a name and a heap object */
static void zeroname(name_t* name) {
	mem_t l;
	name->l=0;
	for(l=0; l<MAXNAME; l++) name->c[l]=0;
	name->token=0;
}

static void zeroheap(heap_t* heap) {
	heap->address=0;
	heap->size=0;
	zeroname(&heap->name);
}

/* output a name */
static void outname(name_t* name) {
	mem_t l;
	for(l=0; l<name->l; l++) outch(name->c[l]);
}
#endif


/* 
 * Create an array. 
 *
 * Arrays are created on the heap. This code allows redimensioning of arrays
 * which leads to a new array with the same name on the heap. This new 
 * array is found first in a heap search. This ways we can have arrays
 * as local variables.
 * 
 * msarraylimits is a flag to indicate that arrays should be created with
 * 0-n elements like in Microsoft BASIC.
 * 
 */
static address_t createarray(name_t* variable, address_t i, address_t j) {
	address_t a;

/* if we want to me MS compatible, the array ranges from 0-n */
	if (msarraylimits) {
		i+=1;
		j+=1;
	}

/* this code allows redimension now for local variables */
#ifdef HASAPPLE1
	if (DEBUG) { 
		outsc("* create array "); outname(variable); outspc();
		outsc("* with name length "); outnumber(variable->l); outspc();
			outnumber(i); outspc(); outnumber(j); outcr(); 
	}	

#ifndef HASMULTIDIM
	return bmalloc(variable, i);
#else

/* allocate the array space */
	a=bmalloc(variable, i*j);

/* store the dimension of the array at the beginning of the array area */
	setaddress(a+i*j*numsize, memwrite2, j);

/* return value is the address of the payload area */
	return a;
#endif
#endif
	return 0;
}

/* 
 * The array function.

 * We use the lefthandside object here with the convention that i is the first index
 * and j the second index. This is inconsistent with the use in strings. Will be fixed 
 * when a true indexing type is introduced. 
 */
static void array(lhsobject_t* object, mem_t getset, number_t* value) {
	address_t a; /* the address of the array element */
	address_t h; /* the number of elements in the array */
	address_t l=arraylimit; /* the lower limit, defaults to the arraylimit, here for further use */
	address_t dim=1; /* the array dimension */

	if (DEBUG) {
		outsc("* array2: accessing "); 
		outname(&name); outspc(); outspc(); 
		outnumber(object->i); outspc(); 
		outnumber(object->j); outspc(); 
		outsc(" getset "); outch(getset); outcr();
	}

/* handling the special array, range check and access is done here */
	if (object->name.c[0] == '@') {
		register char c = object->name.c[1];
		if ( c == 'E' ) {
			h = elength() / numsize;
			a = elength() - numsize * object->i;
			if (a < eheadersize) { error(EORANGE); return; }
			if (getset == 'g') *value = getnumber(a, beread);  
			else if (getset == 's') setnumber(a, beupdate, *value);
			return;
		}
#if defined(DISPLAYDRIVER) && defined(DISPLAYCANSCROLL)
		if ( c == 'D' ) {
			if (getset == 'g') *value=dspget(object->i-1); 
			else if (getset == 's') dspset(object->i-1, *value);
			return;	
		}
#endif
#if defined(HASCLOCK)
		if ( c == 'T' ) {
			if (getset == 'g') *value=rtcget(object->i); 
			else if (getset == 's') rtcset(object->i, *value);
			return;
		}
#endif
#if defined(ARDUINO) && defined(ARDUINOSENSORS)
		if ( c == 'S' ) {
			if (getset == 'g') *value=sensorread(object->i, 0); 
			return;
		}
#endif
		if ( c == 'U' ) {
			if (getset == 'g') *value=getusrarray(object->i); 
			else if (getset == 's') setusrarray(object->i, *value);
			return;
		}
		if ( c ==  0 ) { 
			h=(himem-top)/numsize;
			a=himem-numsize*(object->i+1)+1; 
			if (object->i < 0 || a < top) { error(EORANGE); return; }
			if (getset == 'g') *value=getnumber(a, memread2); 
			else if (getset == 's') setnumber(a, memwrite2, *value);	
			return;
		}
		if ( c == 'M' ) {
			h=himem-top;
			a=himem-object->i;
			if (object->i < 0 || a < top) { error(EORANGE); return; }
			if (getset == 'g') *value=memread2(a); 
			else if (getset == 's') memwrite2(a, *value); 
			return;
		}
		error(EVARIABLE);
		return;
	} else {
/* dynamically allocated arrays */
#ifdef HASAPPLE1
		object->name.token=ARRAYVAR;
		if (!(a=bfind(&object->name))) a=createarray(&object->name, ARRAYSIZEDEF, 1);
		if (!USELONGJUMP && er) return;

/* multidim reserves one address word for the dimension, hence we have less bytes */
#ifndef HASMULTIDIM
		h=bfind_object.size/numsize;
#else
		h=(bfind_object.size-addrsize)/numsize;
#endif

		if (DEBUG) { 
			outsc("** in array dynamical base address "); outnumber(a); 
			outsc("    and array element number "); outnumber(h); 
			outcr(); 
		}

#ifdef HASMULTIDIM
		dim=getaddress(a+bfind_object.size-addrsize, memread2);
		if (DEBUG) { 
			outsc("** in array, second dimension is "); outnumber(dim); 
			outspc(); outnumber(a+bfind_object.size); 
			outcr(); 
		}
		a=a+((object->i-l)*dim+(object->j-l))*numsize;	
#else
		a=a+(object->i-l)*numsize;
#endif
#else /* no array code */
		error(EVARIABLE);
		return;
#endif
	}

/* range check */
#ifdef HASMULTIDIM
	if ( (object->j < l) || (object->j >= dim+l) || (object->i < l) || (object->i >= h/dim+l)) { error(EORANGE); return; }
#else
	if ( (object->i < l) || (object->i >= h+l) ) { error(EORANGE); return; }
#endif

/* set or get the array */
	if (getset == 'g') *value=getnumber(a, memread2); 
	else if (getset == 's') setnumber(a, memwrite2, *value);
}

/* 
 * Create a string on the heap. 
 *
 * i is the length of the string, j the dimension of the array. 
 * 
 * String objects are either plain strings or arrays. In case of arrays
 * the number of strings is stored at the end of the array.
 */
static address_t createstring(name_t* variable, address_t i, address_t j) {
#ifdef HASAPPLE1
	address_t a;
	
	if (DEBUG) { outsc("Create string "); outname(variable); outcr(); }

/* the MS string compatibility, DIM 10 creates 11 elements */
	if (msarraylimits) j+=1;

#ifndef HASMULTIDIM
/* if no string arrays are in the code, we reserve the number of bytes i and space for the index */
/* allow redimension without check right now, for local variables */
	a=bmalloc(variable, i+strindexsize);
	if (er != 0) return 0;
	return a;
#else
/* string arrays need the number of array elements which address_ hence addresize bytes and then 
		the space for j strings */
/* allow redimension without check right now, for local variables */
	a=bmalloc(variable, addrsize+j*(i+strindexsize));
	if (er != 0) return 0;

/* set the array length */
	setaddress(a+j*(i+strindexsize), memwrite2, j);

/* return the address of the first string */
	return a;
#endif
	if (er != 0) return 0;
	return a;
#else 
	return 0;	
#endif
}


/* 
 * Get a string at position b. 
 * 
 * getstring returns a pointer to the first string element in question. 
 * 
 * There is a lo tof complexity in the code to support systems with serial
 * memory. 
 * 
 */
#ifdef HASAPPLE1

/* helpers to handle strings */
/* Stores a C string to a BASIC string variable */
static void storecstring(address_t ax, address_t s, char* b) {
	address_t k; 

	for (k=0; k < s-strindexsize && b[k] != 0; k++) memwrite2(ax+k+strindexsize, b[k]); 
	setstrlength(ax, memwrite2, k);
}

/* length of a c string up to a limit l */
inline static address_t cstringlength(char* c, address_t l) {
	address_t a;

	while(a < l && c[a] != 0) a++;
	return a;
}

/* get a memory pointer to a string, new version */
static void getstring(bstring_t* strp, name_t* name, address_t b, address_t j) {	
	address_t k, zt;
	address_t ax;

/* we know nothing about the string */
	ax=0;
	strp->address=0;
	strp->ir=0;
	strp->length=0;
	strp->arraydim=1;
	strp->strdim=0;

	if (DEBUG) { 
		outsc("* getstring from var "); outname(name); outspc(); 
		outnumber(b); outspc(); 
		outnumber(j); outcr(); 
	}

/* special string variables */
	if (name->c[0] == '@') {
		register char c = name->c[1];
		if ( c == 0 ) {
			strp->ir=ibuffer+b;
			strp->length=ibuffer[0];
			strp->strdim=BUFSIZ-2;
			return;
		}
		if ( c == 'U' ) {
			makeusrstring(); /* a user definable special string in sbuffer */
			strp->ir = sbuffer+b;
			strp->length = sbuffer[0];
			return;
		}
#ifdef HASCLOCK
		if ( c == 'T' ) {
			rtcmkstr(); /* the time string */
			strp->ir=rtcstring+b;
			strp->length=rtcstring[0];
			return;
		}
#endif	
/* the arguments string on POSIX systems */
#ifdef HASARGS
		if ( c == 'A' ) {
			if (bargc > 2) {
				strp->ir=bargv[2];
				strp->length=cstringlength(bargv[2], BUFSIZE);
				return; 
			}	
			return;
		}
#endif	
		error(EVARIABLE); 
		return;
	}
/* dynamically allocated strings, create on the fly */
	if (!(ax=bfind(name))) ax=createstring(name, defaultstrdim, arraylimit);

	if (DEBUG) {
		outsc("** heap address "); outnumber(ax); outcr(); 
		if (ax) { outsc("** byte length of string memory segment "); outnumber(bfind_object.size); outcr(); }
	}

/* string creating has caused an error, typically no memoryy */
	if (!USELONGJUMP && er) return;

#ifndef HASMULTIDIM
/* the maximum length of the string */
	strp->strdim=bfind_object.size-strindexsize;

/* are we in range */
	if ((b < 1) || (b > strp->strdim )) { error(EORANGE); return; }

	if (DEBUG) { outsc("** maximum string length "); outnumber(strp->strdim); outcr(); }

/* get the actual full length, this is redundant to lenstring but lenstring does 
	not autocreate */
	strp->length=getstrlength(ax, memread2);

/* now find the payload address */
	ax=ax+strindexsize+(b-1);
#else 

/* the dimension of the string array */
/* it is at the top of the string, this uses a side effect of bfind */
	strp->arraydim=getaddress(ax + bfind_object.size - addrsize, memread2);

/* is the array index in range */
	if ((j < arraylimit) || (j >= strp->arraydim + arraylimit )) { error(EORANGE); return; }

 	if (DEBUG) { outsc("** string dimension "); outnumber(strp->arraydim); outcr(); }

/* the max length of a string */
	strp->strdim=(bfind_object.size - addrsize)/strp->arraydim-strindexsize;

/* are we in range  */
	if ((b < 1) || (b > strp->strdim )) { error(EORANGE); return; }

	if (DEBUG) { outsc("** maximum string length "); outnumber(strp->strdim); outcr(); }
	
/* the base address of a string */
	ax=ax+(j-arraylimit)*(strp->strdim + strindexsize);

	if (DEBUG) { outsc("** string base address "); outnumber(ax); outcr(); }

/* from this base address we can get the actual length of the string */
	strp->length=getstrlength(ax, memread2);

/* the address of the payload */
	ax=ax+b-1+strindexsize;

#endif

	if (DEBUG) { outsc("** payload address "); outnumber(ax); outcr(); }

/* store the payload address to the string object, length to be done if needed!! */
	strp->address=ax;	

/* return value is 0 if we have no direct memory access, the caller needs to handle the string 
	through the mem address */
#ifdef USEMEMINTERFACE
	strp->ir=0;
#else 
	strp->ir=(char *)&mem[ax];
#endif
}


/* reimplementation with name_t */
/* set the length of a string */
static void setstringlength(name_t* name, address_t l, address_t j) {
	address_t a; 
	stringlength_t stringdim;

	if (DEBUG) {
		outsc("** setstringlength "); 
		outname(name); 
		outspc(); outnumber(l); outspc(); outnumber(j);
		outcr();
	} 

/* the special strings */
	if (name->c[0] == '@') {
		char c = name->c[1];
		if ( c == 0 ) {
			*ibuffer=l;
			return;
		}
		if ( c == 'U') {
			/* do nothing here for the moment */
			return;
		}
	}
/* find the variable address */
	a=bfind(name);
	if (!USELONGJUMP && er) return;
	if (a == 0) { error(EVARIABLE); return; }

/* stringdim calculation moved here */
#ifndef HASMULTIDIM
	stringdim=bfind_object.size-strindexsize;
#else 
/* getaddress seeks the dimension of the string array directly after the payload */
	stringdim=(bfind_object.size - addrsize)/(getaddress(a + bfind_object.size - addrsize, memread2)) - strindexsize;
#endif	

/* where do we write it to */
	a=a+(stringdim+strindexsize)*(j-arraylimit);
	if (DEBUG) { outsc("**  setstringlength writing to "); outnumber(a); outsc(" value "); outnumber(l); outcr(); }
	setstrlength(a, memwrite2, l);
}

/* the BASIC string mechanism for real time clocks, create a string with the clock data */
#ifdef HASCLOCK
inline static void rtcmkstr() {
	int cc = 1;
	int t;

/* hours */
	t=rtcget(2);
	rtcstring[cc++]=t/10+'0';
	rtcstring[cc++]=t%10+'0';
	rtcstring[cc++]=':';

/* minutes */
	t=rtcget(1);
	rtcstring[cc++]=t/10+'0';
	rtcstring[cc++]=t%10+'0';
	rtcstring[cc++]=':';

/* seconds */
	t=rtcget(0);
	rtcstring[cc++]=t/10+'0';
	rtcstring[cc++]=t%10+'0';
	rtcstring[cc++]='-';

/* days */	
	t=rtcget(4);
	if (t/10 > 0) rtcstring[cc++]=t/10+'0';
	rtcstring[cc++]=t%10+'0';
	rtcstring[cc++]='/';

/* months */
	t=rtcget(5);
	if (t/10 > 0) rtcstring[cc++]=t/10+'0';
	rtcstring[cc++]=t%10+'0';
	rtcstring[cc++]='/';

/* years */
	t=rtcget(6)%100; /* only 100 years no 19xx epochs */
	if (t/10 > 0) rtcstring[cc++]=t/10+'0';
	rtcstring[cc++]=t%10+'0';
	rtcstring[cc]=0;

/* needed for BASIC strings */
	rtcstring[0]=cc-1;
}
#endif
#endif 

/* 
 * Layer 0 Extension: the user defined extension functions, use this function for a 
 * quick way to extend BASIC. 
 * 
 * @U is a single variable that can be set and get. Every access to this variable
 * 	calls getusrvar() or setusrvar(). 
 * @U() is a 1d array every access calls get or setusrarray().
 * @U$ is a read only string created by makeusrstring().
 * USR(32, V) is a user defined function created by usrfunction().
 * CALL 32 is a user defined call
 *
 */

/* read or write the variable @U, you can do anything you want here */
//inline static number_t getusrvar() { return 0; }
//void setusrvar(number_t v) { return; }

/* read or write the array @U(), you can do anything you want here */
// number_t getusrarray(address_t i) { return 0;}
// void setusrarray(address_t i, number_t v) { return; }

/* make the usr string from @U$, this function can be called multiple times for one operation */
void makeusrstring() {
/*
 * sample code could be:
 *
 *	mem_t i;
 *	const char text[] = "hello world";
 *	for(i=0; i<SBUFSIZE-1 && text[i]!=0 ; i++) sbuffer[i]=text[i];
 *
 * Always set sbuffer[0] to the string length, keep in mind that sbuffer 
 * is 32 bytes long by default
 */
	sbuffer[0]=0;
}

/* USR with arguments > 31 calls this */
// number_t usrfunction(address_t i, number_t v) { return 0; }

/* CALL with arguments > 31 calls this */
// void usrcall(address_t i) { return; }

/*
 *  Layer 0 - keyword handling - PROGMEM logic goes here
 *		getkeyword(), getmessage(), and getokenvalue() are 
 *		the only access to the keyword array in the code.  
 *
 *	Same for messages and errors.
 * 
 * 	All keywords are stored in PROGMEM, the Arduino way to store
 * 	constant data in flash memory. sbuffer is used to recall the 	
 *  keywords and messages from PROGMEM on Arduino. Other systems
 *  use the keyword array directly.
 */ 
static char* getkeyword(address_t i) {

	if (DEBUG) { outsc("** getkeyword from index "); outnumber(i); outcr(); }

#ifndef ARDUINOPROGMEM
	return (char *) keyword[i];
#else
	strcpy_P(sbuffer, (char*) pgm_read_ptr(&(keyword[i]))); 
	return sbuffer;
#endif
}

/* messages are read from the message array */
static char* getmessage(char i) {
	if (i >= sizeof(message) || i < 0) return 0;
#ifndef ARDUINOPROGMEM
	return (char *) message[i];
#else
	strcpy_P(sbuffer, (char*) pgm_read_ptr(&(message[i]))); 
	return sbuffer;
#endif
}

/* tokens read here are token_t constructed from multi byte sequences */
static token_t gettokenvalue(address_t i) {
	if (i >= sizeof(tokens)) return 0;
#ifndef ARDUINOPROGMEM
	return tokens[i];
#else
#ifndef HASLONGTOKENS
	return (token_t) pgm_read_byte(&tokens[i]);
#else 
	return (token_t) pgm_read_word(&tokens[i]);
#endif
#endif
}

/* print a message directly to the default outpur stream */
static void printmessage(char i){
#ifndef HASERRORMSG
	if (i > EGENERAL) return;
#endif
	outsc((char *)getmessage(i));
}

inline static void clrforstack() {
	loopsp = 0;
}

/*
 *	Layer 0 - error handling
 *
 * The general error handler. The static variable er
 * contains the error state. 
 * 
 * debugtoken() writes a token for debug
 * bdebug() is the general debug message function
 *
 * Strategy: the error() function writes the message and then 
 * clears the stack. All calling functions must check er and 
 * return after funtion calls with no further messages etc.
 * reseterror() sets the error state to normal and end the 
 * run loop.
 */ 
static void error(token_t e){
	address_t i;
	/* store the error number */
	er = e;
	#if TRACE_PRINTF
	printf("[error] %d; e: %d\n", token, e);
	#endif

	/* clear the stacks */
	clearst();
	clrforstack();
	clrgosubstack();

/* switch off all timers and interrupts */
#ifdef HASTIMER
	resettimer(&after_timer);
	resettimer(&every_timer);
#endif

/* is the error handler active? then silently go if we do GOTO or CONT actions in it */
#ifdef HASERRORHANDLING
#if !USELONGJUMP
	if (st != SINT && (berrorh.type == TGOTO || berrorh.type == TCONT)) return;
#else
	if (st != SINT && (berrorh.type == TGOTO || berrorh.type == TCONT)) longjmp(sthook, er);
#endif
#endif

/* set input and output device back to default, and delete the form */
	iodefaults();
 	form = 0;

/* find the line number if in RUN modes */
	if (st != SINT) {
		address_t a = myline(here);
		#if TRACE_PRINTF
		printf("[error] %d %d\n", token, a);
		#endif
		outnumber(a);
		outch(':');
		outspc();
	}

/* if we have error messages, display them */	
#ifdef HASERRORMSG
	if (e > 0)
		printmessage(e);
	else {
		for(i=0; gettokenvalue(i)!=0 && gettokenvalue(i)!=e; i++);
		outsc(getkeyword(i));
	}
	outspc();
	printmessage(EGENERAL);
#else 
	printmessage(EGENERAL);
	outspc();
	outnumber(er);
#endif
	if (DEBUG) { outsc("** at "); outnumber(here); }
	outcr();

/* reset fncontext - this is odd */
	fncontext=0;

/* we return to the statement loop, bringing the error with us */
#if USELONGJUMP == 1
	longjmp(sthook, er);
#endif 
}

void reseterror() {
	er=0;
	here=0;
	st=SINT;
}

static void debugtoken(){
	outsc("* "); 
	if (debuglevel > 2) { outnumber(here); outsc(" * "); }
	if (token == EOL) {
		outsc("EOL");
		return;	
	}

	if ( token == LINENUMBER ) {
		printmessage(MLINE);
	} else if ( token == NUMBER ) {
		printmessage(MNUMBER);
	} else if ( token == VARIABLE ) {
		printmessage(MVARIABLE);
	} else if ( token == ARRAYVAR ) {
		printmessage(MARRAY);
	} else if ( token == STRING ) {
		printmessage(MSTRING);
	} else if ( token == STRINGVAR ) {
		printmessage(MSTRINGVAR);
	}

	outspc();
	outputtoken();
}

static void bdebug(const char *c){
	outch('*');
	outspc();
	outsc(c); 
	debugtoken();
	outcr();
}

/*
 *	Arithmetic and runtime operations are mostly done
 *	on a stack of number_t.
 *
 *	push(), pop(), clearst() handle the stack
 */
static void push(number_t t){

	if (DEBUG) {
		outsc("** push sp= "); outnumber(sp); outcr(); 
		outsc("** push value= "); outnumber(t); outcr(); 
	}

/* in forced integer mode every operation is truncated */
#ifdef HASFLOAT
	if (forceint) t=trunc(t);
#endif

	if (sp == STACKSIZE)
		error(ESTACK);
	else
		stack[sp++].n=t;
}

static number_t pop(){

	if (DEBUG) {
		outsc("** pop sp= "); outnumber(sp); outcr(); 
		outsc("** pop value= "); outnumber(stack[sp-1].n); outcr();
	}

	if (sp == 0) { error(ESTACK); return 0; }

	return stack[--sp].n;	
}
/*
inline static void pushaddress2(address_t a) {

	if (DEBUG) {
		outsc("** push sp= "); outnumber(sp); outcr(); 
		outsc("** push value= "); outnumber(a); outcr(); 
	}

	if (sp == STACKSIZE)
		error(ESTACK);
	else
		stack[sp++].a=a;
}

inline static address_t popaddress2() {

	if (DEBUG) {
		outsc("** pop sp= "); outnumber(sp); outcr(); 
		outsc("** pop value= "); outnumber(stack[sp-1].a); outcr();
	}

	if (sp == 0) { error(ESTACK); return 0; }

	return stack[--sp].a;
}

void pushinteger(index_t i) {

	if (DEBUG) {
		outsc("** push sp= "); outnumber(sp); outcr(); 
		outsc("** push value= "); outnumber(i); outcr(); 
	}

	if (sp == STACKSIZE)
		error(ESTACK);
	else
		stack[sp++].i=i;
}

index_t popinteger() {

	if (DEBUG) {
		outsc("** pop sp= "); outnumber(sp); outcr(); 
		outsc("** pop value= "); outnumber(stack[sp-1].i); outcr();
	}

	if (sp == 0) { error(ESTACK); return 0; }

	return stack[--sp].i;
}
*/

/* this one gets a positive integer from the stack and traps the error*/
static address_t popaddress() {
	number_t tmp = 0;

	tmp=pop();
	if (tmp < 0) { error(EORANGE); return 0;} 
	return (address_t) tmp;
}

inline static void clearst() {
	sp = 0;
}

/* these are not really stack operations but a way to handle temp char data
address_t charsp; 

void pushchar(char ch) {}

char popchar() { return 0; }
*/

/*
 * clear the cursor for the READ/DATA mechanism
 */
static void clrdata() {
#ifdef HASDARTMOUTH
	data = 0;
#endif
}

/* 
 * Stack handling for FOR
 * Reimplementation of the for stack with names and with all loops 
 * cleaned up.
 */

/* the new stack type for loops */
static void pushloop(name_t* name, token_t t, address_t here, number_t to, number_t step) {
	address_t i;

	if (DEBUG) {
		outsc("** loopsp and here in pushloopstack "); 
		outnumber(loopsp); outspc(); outnumber(here); outcr(); 
		if (name != 0) { outsc("** loop name "); outname(name); outcr(); }
		else { outsc("** loop name is 0"); outcr(); }
		outsc("** loop token "); outnumber(t); outcr();
		outsc("** loop to "); outnumber(to); outcr();
		outsc("** loop step "); outnumber(step); outcr();
	}

/* 
 * Before pushing into the loop stack we check is an
 * old loop exists. 
 * 
 * There are two situations to handle:
 * 1. A loop is reentered because a GOTO went back to or even before 
 * 	the loop start. This is identified by the here location.
 * 2. A new FOR loop is created after the existing loop with the same
 * 	variable name. This happens if a jump or break went outside the 
 *  loop. This is identified by the variable name.
*/

/* Situation 1, scan for here */
	for(i=0; i<loopsp; i++) {
		if (loopstack[i].here == here) {
			loopsp=i; 
			break;
		}
	}

/* Situation 2, scan for the name */
	if (name != 0) {
		for(i=0; i<loopsp; i++) {
			if (cmpname(&loopstack[i].var, name)) {
				loopsp=i;
				break;
			}
		}
	}

/* Add the loop to the stack */
	if (loopsp < FORDEPTH) {
		if (t == TWHILE || t == TREPEAT) {
			loopstack[loopsp].var.token=t;
		} else {
			if (name != 0) {
				loopstack[loopsp].var=*name;
			} else {
				loopstack[loopsp].var.c[0]=0;
				loopstack[loopsp].var.l=0;
				loopstack[loopsp].var.token=0;
			}
		}
		loopstack[loopsp].here=here;
		loopstack[loopsp].to=to;	
		loopstack[loopsp].step=step;
		loopsp++;	
		return;	
	} else 
		error(ELOOP);
}

/* what is the active loop */
static bloop_t* activeloop() {
	if (loopsp>0) {
		return &loopstack[loopsp-1];
	} else {
		error(ELOOP);
		return 0;
	}
}

static void droploop() {
	if (loopsp>0) {
		loopsp--;
	} else {
		error(ELOOP);
		return;
	} 
}

/* GOSUB stack handling */
static void pushgosubstack(mem_t a){
	if (gosubsp < GOSUBDEPTH) {
		gosubstack[gosubsp]=here;
#ifdef HASEVENTS
    	gosubarg[gosubsp]=a;
#endif
    	gosubsp++;  
	} else 
		error(TGOSUB);
}

static void popgosubstack(){
	if (gosubsp>0) {
		gosubsp--;
	} else {
		error(TRETURN);
		return;
	} 
	here=gosubstack[gosubsp];
}
static void clrgosubstack() {
	gosubsp = 0;
}

/* two helper commands for structured BASIC, without the GOSUB stack */

static void pushlocation(blocation_t* l) {
	if (st == SINT) l->location = bi - ibuffer;
	else  l->location = here;
	l->token = token;
}

void poplocation(blocation_t* l) {
	if (st == SINT) bi=ibuffer+l->location;
	else here=l->location;
	token=l->token;
}

/* little helpers of the io functions */

/* send a newline */
inline static void outcr() {
#ifdef HASSERIAL1
	if (sendcr) outch('\r');
#endif
	outch('\n');
} 

/* send a space */
inline static void outspc() {
	outch(' ');
}

/* output a zero terminated string - c style */
static void outsc(const char *c) {
	while (*c != 0)
		outch(*c++);
}

/* output a zero terminated string in a formated box padding spaces 
		needed for catalog output */
inline static void outscf(const char *c, index_t f){
	int i = 0;
	while (*c != 0) { outch(*c++); i++; }
	if (f > i) {
		f=f-i;
		while (f--) outspc();
	}
}

/* 
 *	reading a positive number from a char buffer 
 *	maximum number of digits is adjusted to 16
 *	ugly here, testcode when introducting 
 *	number_t was only 16 bit before
 */
static address_t parsenumber(char *c, number_t *r) {
	address_t nd = 0;

	*r=0;
	while (*c >= '0' && *c <= '9' && *c != 0) {
		*r=*r*10+*c++-'0';
		nd++;
		if (nd == SBUFSIZE) break;
	}
	return nd;
}


#ifdef HASFLOAT
/* a poor man's atof implementation with character count */
static address_t parsenumber2(char *c, number_t *r) {
	address_t nd = 0;
	index_t i;
	number_t fraction = 0;
	number_t exponent = 0;
	mem_t nexp = 0;

	*r=0;

/* integer part */
	i=parsenumber(c, r);
	c+=i;
	nd+=i;

/* the fractional part */
	if (*c == '.') {
		c++; 
		nd++;

		i=parsenumber(c, &fraction);
		c+=i;
		nd+=i;

		if (i > 0) {
			while ((--i)>=0) fraction=fraction/10;
			*r+=fraction;
		} 
	}

/* the exponent */
	if (*c == 'E' || *c == 'e') {
		c++;
		nd++;
		if (*c == '-') {c++; nd++; nexp=1;};
		i=parsenumber(c, &exponent);
		nd+=i;  
		while ((--exponent)>=0) if (nexp) *r=*r/10; else *r=*r*10;		
	}

	return nd;
}
#endif

/*
 *	convert a number to a string
 *	the argument type controls the largest displayable integer
 *  default is int but it can be increased without any side effects
 */
static address_t writenumber(char *c, wnumber_t v){
	address_t nd = 0;	
	index_t i,j;
	mem_t s = 1;
	char c1;
  
/* the sign */
	if (v<0) s=-1; 

/* the digits */
	do {
		c[nd++]=(v%10)*s+'0';
		v=v/10;
	} while (v != 0);

/* print the minus */
	if (s < 0 ) c[nd]='-'; else nd--;

/* reverse the order of digits */
	i=0; 
	j=nd;
	while (j>i) {
		c1=c[i];
		c[i]=c[j];
		c[j]=c1;
		i++;
		j--;
	}

	nd++;
	c[nd]=0;
	return nd;
} 

#ifdef HASFLOAT
/*
 * this is for floats, handling output without library 
 * functions as well.
 */
address_t tinydtostrf(number_t v, index_t p, char* c) {  
	index_t i;
	address_t nd = 0;
	number_t f;

/* if we are in forced integer mode */
	if (forceint) {
		v=trunc(v);
		return writenumber(c, (int)v);
	}

/* we do the sign here and don't rely on writenumbers sign handling, guess why */
	if (v<0) {
		v=fabs(v);
		c[nd++]='-';
	}

/* write the integer part */
	nd+=writenumber(c+nd, (int)v); 
	c[nd++]='.';

/* only the fraction to precision p */
	f=fabs(v);

/* get p digits of the fraction */
	for (i=p; i>0; i--) {
		f=f-floor(f);
		f=f*10;
		c[nd++]=(int)floor(f)+'0';
	}

/* and a terminating 0 */
	c[nd]=0;
	return nd;
}
 
address_t writenumber2(char *c, number_t vi) {
	index_t i;
	index_t nd;
	number_t f;
	index_t exponent = 0; 
	mem_t eflag=0;
	const int p = 5;

/* pseudo integers are displayed as integer
		zero trapped here */
	f=floor(vi);
	if (f == vi && fabs(vi) < maxnum) return writenumber(c, vi);
  
/* earlier, floats where displayed in POSIx using the libraties
 * return sprintf(c, "%g", vi);
 * we dont do this any more 
 */

/* we check if we have anything to write */
	if (!isfinite(vi)) {
    	c[0]='*';
    	c[1]=0;
    	return 1; 
	}

/* normalize the number and see which exponent we have to deal with */
	f=vi;
	while (fabs(f)<1.0)   { f=f*10; exponent--; }
 	while (fabs(f)>=10.0-0.00001) { f=f/10; exponent++; }

/* there are platforms where dtostrf is broken, we do things by hand in a simple way */

	if (exponent > -2 && exponent < 7) { 
    	tinydtostrf(vi, 5, c);
	} else {
		tinydtostrf(f, 5, c);
		eflag=1;
	}

/* remove trailing zeros */
	for (i=0; (i < SBUFSIZE && c[i] !=0 ); i++);
	i--;

/* */	
	while (c[i] == '0' && i>1) { i--; }
	i++;
    
/* add the exponent */
	if (eflag && exponent != 0) {
		c[i++]='E';
		i+=writenumber(c+i, exponent);
	}

	c[i]=0;
	return i;

}
#endif

/*
 * innumber is used as a helper only by xinput(). It reads a number from an input
 * buffer and returns it in the number_t r. It returns 0 if the number is invalid, 1 if
 * the number is valid, and -1 if the user has pressed the BREAKCHAR key.
 * 
 * Unlike the old innumber() implementation it is meant to read comma separated numbers
 * through input. Handling of the buffer is done by the calling function.  
 * 
 * The TINYBASICINPUT is an alternative implementation following the Palo Alto BASIC
 * way of doing it. The expression parser is reused. This allows variables names and 
 * expressions to be used as input as well. 
 * Due to the reuse of sbuffer in the calling function, this will probably only work 
 * in RUN mode. 
 */

static int innumber(number_t *r, char* buffer, address_t k) {
	address_t i = k;
	mem_t s = 1;

#ifndef HASTINYBASICINPUT
/* result is zero*/
	*r=0;

/* remove all leading whitespaces first */
	while ((buffer[i] == ' ' || buffer[i] == '\t') && i <= (address_t) buffer[0]) i++;

/* is there anything left */
	if (i > (address_t) buffer[0]) return 0;

/* now the sign */
	if (buffer[i] == '-') { s=-1; i++; }

/* check for the break character */
	if (buffer[i] == BREAKCHAR) return -1;

/* the number */	
#ifndef HASFLOAT
	if (buffer[i] < '0' || buffer[i] > '9') return 0;
	i+=parsenumber(&buffer[i], r);
#else
	if ((buffer[i] < '0' || buffer[i] > '9') && buffer[i] != '.') return 0;
	i+=parsenumber2(&buffer[i], r);
#endif

/* the sign */
	*r*=s;
	return i;
#else
	char *b;
	token_t t;	

/* result is zero */
	*r=0;

/* save the interpreter state */
	b=bi;
	s=st;
	t=token;

/* switch to fake interactive with the buffer as input */
	st=SINT;
	bi=buffer+k;

/* BREAK handling */
	if (*bi == BREAKCHAR) {
		return -1;
	}

/* start to interpret the buffer as an expression */
	nexttoken();
	expression();

/* restore the interpreter state */
	i=bi-buffer-1; 
	bi=b;
	st=s;
	token=t;

/* error handling, we trap the error and return zero */
	if (er) {
		er=0;
		return 0;
	}

/* the result is on the stack */
	*r=pop();
	return i;
#endif
}

/* prints a number */
static void outnumber(number_t n){
	address_t nd, i;

/* number write to sbuffer, remember the number of digits in nd */
#ifndef HASFLOAT
	nd=writenumber(sbuffer, n);
#else
	nd=writenumber2(sbuffer, n);
#endif 

/* negative number format aligns right */
	if (form < 0) { i=nd; while(i < -form) {outspc(); i++;} }

/* the number */
	outs(sbuffer, nd);

/* number formats in Palo Alto style, positive numbers align left */
	if (form > 0) { while (nd < form) {outspc(); nd++;} }
}

/* 
 *	Layer 1 functions - providing data into the global variable and 
 *	changing the interpreter state
 */

/*
 *	Lexical analyser - tokenizes the input line.
 *
 *	nexttoken() increments the input buffer index bi and delivers values in the global 
 *		variable token, with arguments in the accumulators ax, x and the index register ir
 *		name is used in the routine. 
 *
 *	name, sr, ax and x change values in nexttoken and deliver the result to the calling
 *	function.
 *
 *	bi and ibuffer should not be changed or used for any other function in 
 *	interactive node as they contain the state of nexttoken(). In run mode 
 *	bi and ibuffer are not used as the program is fully tokenized in mem.
 * 
 *	all this is pretty much stateless.
 */ 

/* skip whitespaces */
static void whitespaces(){
	while (*bi == ' ' || *bi == '\t') bi++;
}

/* upper case, don't trust the buildins on microcontrollers */
static char btoupper(char c) {
	if (c >= 'a' && c <= 'z') return c-32; else return c;
}

/* the token stream */
static void nexttoken() {
	address_t k, l, i;
	char* ir;
	char quotechar;
/* RUN mode vs. INT mode, in RUN mode we read from mem via gettoken() */
	if (st == SRUN || st == SERUN) {
/* in the token stream we call fastticker - all fast timing functions are in stream */
		fastticker(); 
/* read the token from memory */
		gettoken();
/* show what we are doing */
		if (debuglevel > 1) { debugtoken(); outcr(); }
		#if TRACE_PRINTF
		printf("next token1: %c %d\n", (token & 0xFF), token);
		#endif
		return;
	}

/* after change in buffer logic the first byte is reserved for the length */
	if (bi == ibuffer) bi++;

/* literal mode only(!) EOL ends literal mode, used to have REM without quotes */
	if (lexliteral) {
		token=*bi;
		if (*bi != '\0') bi++; else lexliteral=0;
		#if TRACE_PRINTF
		printf("next token2: %d\n", token);
		#endif
		return;
	}

/* remove whitespaces outside strings */
	whitespaces();

/* end of line token */
	if (*bi == '\0') { 
		token = EOL; 
		if (DEBUG) debugtoken();
		#if TRACE_PRINTF
		printf("next token3: %d\n", token);
		#endif
		return; 
	}

/* unsigned numbers, value returned in x */
#ifndef HASFLOAT
	if (*bi <='9' && *bi >= '0') {		
		bi+=parsenumber(bi, &x);
#else
	if ((*bi <='9' && *bi >= '0') || *bi == '.') {	
		bi+=parsenumber2(bi, &x);
#endif
		token=NUMBER;
		if (DEBUG) debugtoken();
		#if TRACE_PRINTF
		printf("next token4: %d\n", token);
		#endif
		return;
	}

/* strings between " " or " EOL, value returned in ir and in sr for now */
	if (*bi == '"' || *bi == '\'') {
		quotechar=*bi;
		k=0;
		bi++;
		ir=bi;
		sr.ir=bi;
		while(*bi != quotechar && *bi !='\0') {
			k++;
			bi++;
		} 
		bi++;
		token=STRING;
		sr.length=k;
		sr.address=0; /* we don't find the string in BASIC memory, as we lex from bi */
		if (DEBUG) debugtoken();
		#if TRACE_PRINTF
		printf("next token5: %d\n", token);
		#endif
		return;
	}

/*
 *	relations
 *	single character relations are their own token
 *	>=, =<, =<, =>, <> are tokenized
 */
	if (*bi == '=') {
		bi++;
		whitespaces();
		if (*bi == '>') {
			token=GREATEREQUAL;
			bi++;
		} else if (*bi == '<') {
			token=LESSEREQUAL;
			bi++;
		} else {
			token='=';
		}
		if (DEBUG) debugtoken();
		#if TRACE_PRINTF
		printf("next token6: %d\n", token);
		#endif
		return;
	}

	if (*bi == '>'){
		bi++;
		whitespaces();
		if (*bi == '=') {
			token=GREATEREQUAL;
			bi++;
		} else  {
			token='>';
		}
		if (DEBUG) debugtoken();
		#if TRACE_PRINTF
		printf("next token7: %d\n", token);
		#endif
		return;
	}

	if (*bi == '<'){
		bi++;
		whitespaces();
		if (*bi == '=') {
			token=LESSEREQUAL;
			bi++;
		} else  if (*bi == '>') {
			token=NOTEQUAL;
			bi++;
		} else {
			token='<';
		}
		if (DEBUG) debugtoken();
		#if TRACE_PRINTF
		printf("next token8: %d\n", token);
		#endif
		return;
	}

/* 
 *	Keyworks and variables
 *
 *	Isolate a word, bi points to the beginning, l is the length of the word.
 *	ir points to the end of the word after isolating.
 *	@ is a letter here to make the special @ arrays possible.
 */

	l=0;
	ir=bi;
	while (-1) {
		if (*ir >= 'a' && *ir <= 'z') {
			if (!lowercasenames) *ir-=32; /* toupper code, changing the input buffer directly */
			ir++;
			l++;
		} else if ((*ir >= '@' && *ir <= 'Z') || *ir == '_') { 
			ir++;
			l++;
		} else {
			break;
		}
	}

/* 
 *	Ir is reused here to implement string compares
 *	scanning the keyword array. 
 *	Once a keyword is detected the input buffer is advanced 
 *	by its length, and the token value is returned. 
 *
 *	Keywords are an array of null terminated strings.
 *  They are always matched uppercase. 
 */
	k=0;
	while (gettokenvalue(k) != 0) {
		ir=getkeyword(k);
		i=0;
		while (*(ir+i) != 0) {
			if (*(ir+i) != btoupper(*(bi+i))) {
				k++;
				i=0;
				break;
			} else 
				i++;
		}
		if (i == 0) continue;
		bi+=i;
		token=gettokenvalue(k);
		if (token == TREM) lexliteral = 1;
		#if TRACE_PRINTF
		printf("next token9: %d\n", token);
		#endif
		if (DEBUG) debugtoken();
		return;
	}

/*
 * A variable has length 2 with either two letters or a letter 
 * and a number. @ can be the first letter of a variable. 
 * Here, no tokens can appear any more as they have been processed 
 * further up. 
 * 
 * The longname code supports MAXNAME characters and _ as additional character.
 */
#ifdef HASLONGNAMES
	if (l>0 && l<=MAXNAME) {
		token=VARIABLE;
		zeroname(&name);
		while (((*bi >= '0' && *bi <= '9') || 
			    (*bi >= '@' && *bi <= 'Z') || 
				(*bi >= 'a' && *bi <= 'z') || 
			    (*bi == '_') ) && name.l < MAXNAME && *bi != 0) { 
			name.c[name.l]=*bi;
			bi++;
			name.l++;
		} 
		if (*bi == '$') {
			token=STRINGVAR;
			bi++;
		}
		whitespaces();
		if (token == VARIABLE && *bi == '(' ) { 
			token=ARRAYVAR;
		}	
/* the new code filling the name variable directly, will be used in the entire code soon */
		name.token = token;
		#if TRACE_PRINTF
		printf("next tokenA: %d\n", token);
		#endif
		if (DEBUG) debugtoken();
		return;
	}
#else
	if (l == 1 || l == 2) {
		token=VARIABLE;
		name.l=0;
		name.c[0]=*bi;
		name.c[1]=0;
		bi++;
		if ((*bi >= '0' && *bi <= '9') || (*bi >= 'A' && *bi <= 'Z') || *bi == '_' ) { 
			name.c[1]=*bi;
			bi++;
		} 
		if (*bi == '$') {
			token=STRINGVAR;
			bi++;
		}
		whitespaces();
		if (token == VARIABLE && *bi == '(' ) { 
			token=ARRAYVAR;
		}	
/* the new code filling the name variable directly, will be used in the entire code soon */
		name.token=token;			
		if (DEBUG) debugtoken();
		return;
	}
#endif

/* other single characters are parsed and stored */
	token=*bi;
	bi++;
	if (DEBUG) debugtoken();
	#if TRACE_PRINTF
	printf("next tokenB: %d\n", token);
	#endif
	return;
}

/* 
 * Layer 1 - program editor 
 *
 *	Editing the program, the structure of a line is 
 *	LINENUMBER linenumber(2 or more bytes) token(n bytes)
 *
 * store* stores something to memory 
 * get* retrieves information
 *
 *	No matter how long number_t is in the C implementation
 *	we pack into bytes, this is clumsy but portable
 *	the store and get commands all operate on here 
 *	and advance it
 *
 *	storetoken() operates on the variable top. 
 *	We always append at the end and then sort. 
 *
 *	gettoken() operate on the variable here 
 *	which will also be the key variable in run mode.
 *
 *	tokens are stored including their payload.
 *
 *	This group of functions changes global states and
 *	cannot be called at program runtime with saving
 *	the relevant global variable to the stack.
 */

/* 
 *  check if we still have memory left, on RAM only systems we make
 *  sure that we don't hit himem. On systems with EEPROM program storage we make
 *  sure that we stay within the EEPROM range.
 */
char nomemory(number_t b){
#ifndef EEPROMMEMINTERFACE
	if (top >= himem-b) return 1; else return 0;
#else
	if (top >= elength()-eheadersize-b) return 1; else return 0;
#endif
}

/* store a token - check free memory before changing anything */
static void storetoken() {
	int i;
	if (token == LINENUMBER ) {
		if (nomemory(addrsize+1)) return;
		memwrite2(top++, token);
		setaddress(top, memwrite2, ax);
		top+=addrsize;
		return;	
	}
	if (token == NUMBER ) {
		if (nomemory(numsize+1)) return;
		memwrite2(top++, token);
		setnumber(top, memwrite2, x);
		top+=numsize;
		return;
	}
	if (token == ARRAYVAR || token == VARIABLE || token == STRINGVAR ) {
		if (nomemory(sizeof(name_t))) return;
		memwrite2(top++, token);
		top = setname_pgm(top, &name);
		return;
	}
	if (token == STRING ) {
		i=sr.length;
		if (nomemory(i+2)) return;
		memwrite2(top++, token);
		memwrite2(top++, i);
		while (i > 0) { memwrite2(top++, *sr.ir++); i--; }	
		return;
	}
	if (token >= -127) { /* the good old code with just one byte token */
		if (nomemory(1)) return;
		memwrite2(top++, token);
		return;
	}
#ifdef HASLONGTOKENS
	if (nomemory(2)) return; /* this is the two byte token extension */
	memwrite2(top++, TEXT1);
	memwrite2(top++, token+255);
#endif
} 


/* 
 * wrappers around mem access in genereal
 *
 * memread is used only in the token stream, it reads from a stream
 * read only. If run is done from eeprom then bytes are taken from this 
 * stream, this would also be the place to implement direct run from 
 * another device like a file system or embedded programs on flash. 
 * Only the token stream uses memread. 
 *
 * memread2 and memwrite2 always go to ram. They are read/write. Variables and 
 * the program editor uses these functions. 
 *
 * currently only the SPIRAM and the EEPROM direct edit interface is implemented. 
 * This is handled in the runtime library.
 * 
 * USEMEMINTERFACE is the macro to control this.
 * 
 * The POSIX code has a test interface for SPIRAM as a dummy.
 *
 */
#ifndef USEMEMINTERFACE
static mem_t memread(address_t a) {
	if (st != SERUN) {
		return mem[a];
	} else {
		return eread(a + eheadersize);
	}
}

inline static mem_t memread2(address_t a) { return mem[a]; }

static void memwrite2(address_t a, mem_t c) { mem[a]=c; }
#else 
#if defined(SPIRAMINTERFACE) || defined(SPIRAMSIMULATOR)
mem_t memread(address_t a) {
	if (st != SERUN) {
		return spiram_robufferread(a);
	} else {
		return eread(a+eheadersize);
	}
}

mem_t memread2(address_t a) { return spiram_rwbufferread(a); }

void memwrite2(address_t a, mem_t c) { spiram_rwbufferwrite(a, c); }
#else
#ifdef EEPROMMEMINTERFACE
mem_t memread(address_t a) {
	if (a < elength()-eheadersize) return eread(a+eheadersize); else return mem[a-(elength()-eheadersize)];
}

mem_t memread2(address_t a) { return memread(a); }

void memwrite2(address_t a, mem_t c) { 
	if (a < elength()-eheadersize) eupdate(a+eheadersize, c); else mem[a-(elength()-eheadersize)]=c;
}
#endif
#endif
#endif


/* get a token from memory */
void gettoken() {
	stringlength_t i;
	/* if we have reached the end of the program, EOL is always returned
		we don't rely on mem having a trailing EOL */
	if (here >= top) {
		token = EOL;
		#if TRACE_PRINTF
		printf("[gettoken1] %d\n", token);
		#endif
		return;
	}

	/* if we have no data type we are done reading just one byte */
	token = memread(here++); 
	name.token = token;

	/* if there are multibyte tokens, get the next byte and construct a token value <-127 */
#ifdef HASLONGTOKENS
	if (token == TEXT1) {
		token = memread(here++) - 255;
	}
#endif
 
 /* otherwise we check for the argument */
	if ( token == LINENUMBER ) {
		ax = getaddress(here, memread);
		here += addrsize;
		#if TRACE_PRINTF
		printf("[gettoken2] %d\n", token);
		#endif
		return;
	}
	if ( token == NUMBER ) {
		x = getnumber(here, memread);
		here += numsize;
		#if TRACE_PRINTF
		printf("[gettoken3] %d\n", token);
		#endif
		return;
	}
	if ( token == ARRAYVAR || token == VARIABLE || token == STRINGVAR ) {
		here = getname(here, &name, memread);
		name.token = token;
		#if TRACE_PRINTF
		printf("[gettoken4] %d\n", token);
		#endif
		return;
	}
	if ( token == STRING ) {
		sr.length = (unsigned char)memread(here++);	
	/* 
 	 * 	if we run from EEPROM, the input buffer is used to get string constants. 
 	 *  if we run on a system with real memory, we produce a mem pointer
 	 *  otherwise the caller has to handle strings through the address (SPIRAM systems)
 	 */
		if (st == SERUN) { 					
			for (i = 0; i < sr.length; ++i)
				ibuffer[i] = memread(here+i);	
			sr.ir = ibuffer;
		} else {
#ifndef USEMEMINTERFACE
			sr.ir = (char*)&mem[here]; 
#else 
			sr.ir = 0;
#endif
		}
		sr.address = here;
		here += sr.length;	
	}
	#if TRACE_PRINTF
	printf("[gettoken4] %d\n", token);
	#endif
}

/* goto the first line of a program 
void firstline() {
	if (top == 0) {
		ax=0;
		return;
	}
	here=0;
	gettoken();
	#if TRACE_PRINTF
	printf("[firstline] %d\n", token);
	#endif
}
*/

/* goto the next line, search forward */
static void nextline() {
	while (here < top) {
		gettoken();
		#if TRACE_PRINTF
		printf("[nextline] %d\n", token);
		#endif
		if (token == LINENUMBER) return;
		if (here >= top) {
			here=top;
			ax=0;
			return;
		}
	}
}

/* 
 * the line cache mechanism, useful for large codes.
 * addlinecache does not test if the line already exist because it 
 * assumes that findline calls it only if a new line is to be stored
 * the LINECACHE size depends on the architecture. 
 */
#if defined(LINECACHESIZE) && LINECACHESIZE > 0
const unsigned char linecachedepth = LINECACHESIZE;
typedef struct {address_t l; address_t h;} linecacheentry;
linecacheentry linecache[LINECACHESIZE];
unsigned char linecachehere = 0;

static void clrlinecache() {
	unsigned char i;

	for(i=0; i<linecachedepth; i++) linecache[i].l=linecache[i].h=0;
	linecachehere=0;
}

static void addlinecache(address_t l, address_t h) {
	linecache[linecachehere].l=l; 
	linecache[linecachehere].h=h;
	linecachehere=(linecachehere+1)%linecachedepth;
}

static address_t findinlinecache(address_t l){
	unsigned char i;

	for(i=0; i<linecachedepth && linecache[i].l != 0; i++) {
		if (linecache[i].l == l) return linecache[i].h;
	}
	return 0;
}
#else
void clrlinecache() {}
void addlinecache(address_t l, address_t h) {}
address_t findinlinecache(address_t l){ return 0; }
#endif


/* find a line, look in cache then search from the beginning 
 * x is used as the valid line number once a line is found
 * hence x must be global 
 * (this is the logic of the gettoken mechanism)
 */
static void findline(address_t l) {
	address_t a;
	#if TRACE_PRINTF
	printf("[findline] %d [%d]\n", token, l);
	#endif
	/* we know it already, here to advance */
	if ((a = findinlinecache(l))) { 
		here = a; 
		token = LINENUMBER;
		ax = l;
		#if TRACE_PRINTF
		printf("[findline] %d [%d] found in cache\n", token, l);
		#endif
		return;
	}

/* we need to search */
	here = 0;
	while (here < top) {
		gettoken();
		#if TRACE_PRINTF
		printf("[findline] %d here (%d) < top (%d)\n", token, here, top);
		#endif
		if (token == LINENUMBER && ax == l ) {
			/* now that we know we cache */
			addlinecache(l, here);
			return;
		}
	}
	#if TRACE_PRINTF
	printf("[findline] %d here (%d) not found\n", token, l);
	#endif
	error(ELINE);
}

/* finds the line of a location */
static address_t myline(address_t h) {
	address_t l = 0; 
	address_t l1 = 0;
	address_t here2;

	here2 = here;
	here = 0;
	gettoken();
	#if TRACE_PRINTF
	printf("[myline] %d\n", token);
	#endif
	while (here < top) {
		if (token == LINENUMBER) {
			l1 = l;
			l = ax;
		}
		if (here >= h) break;
		gettoken();
		#if TRACE_PRINTF
		printf("[myline2] %d\n", token);
		#endif
	}
	here = here2;
	if (token == LINENUMBER)
		return l1;
	else 
		return l;
}

/*
 *	Move a block of storage beginng at b ending at e
 *	to destination d. No error handling here!!
 */
static void moveblock(address_t b, address_t l, address_t d) {
	address_t i;

	if (d+l > himem) {
		error(EOUTOFMEMORY);
		return;
	}	
	if (l<1) return;

	if (b < d) for (i=l; i>0; i--) memwrite2(d+i-1, memread2(b+i-1));
	else for (i=0; i<l; i++) memwrite2(d+i, memread2(b+i));
}

/* zero a block of memory */
inline static void zeroblock(address_t b, address_t l){
	address_t i;

	if (b+l > himem) { error(EOUTOFMEMORY); return; }
	if (l<1) return;

	for (i=0; i<l+1; i++) memwrite2(b+i, 0);
}

/*
 *	Line editor: 
 *
 *	stage 1: no matter what the line number is - store at the top
 *   	remember the location in here.
 *	stage 2: see if it is only an empty line - try to delete this line
 *	stage 3: calculate lengthes and free memory and make room at the 
 *		appropriate place
 *	stage 4: copy to the right place 
 *
 *	Very fragile code, con't change if you don't have to
 *
 *	zeroblock statements commented out after EOL code was fixed
 */
#if DEBUG
/* diagnosis function */
static void diag(){
	outsc("top, here, y and x\n");
	outnumber(top); outspc();
	outnumber(here); outspc();		
	outcr();
}
#endif

static void storeline() {
	const index_t lnlength = addrsize + 1;
	index_t linelength;
	number_t newline; 
	address_t here2, here3; 
	address_t t1, t2;
	address_t y;
/* the data pointers becomes invalid once the code has been changed */
	clrdata();

/* line cache is invalid on line storage */
	clrlinecache();

	if (DEBUG) { outsc("storeline "); outnumber(ax); outsc(" : "); outsc(ibuffer); outcr(); }

/*
 *	stage 1: append the line at the end of the memory,
 *	remember the line number on the stack and the old top in here
 */
	t1=ax;			
	here=top;		
	newline=here;	 
	token=LINENUMBER;
	do {
		storetoken();
		if (er != 0 ) {
			top=newline;
			here=0;
			return;
		}
		nexttoken();
	} while (token != EOL);

	ax=t1;									/* recall the line number */
	linelength=top-here;	/* calculate the number of stored bytes */

/* 
 *	stage 2: check if only a linenumber stored - then delete this line
 */
	if (linelength == (lnlength)) {  		
		top -= (lnlength);
		#if TRACE_PRINTF
		printf("[storeline]\n");
		#endif
		findline(ax);
		if (er)
			return;	
		y = here - lnlength;							
		nextline();			
		here -= lnlength;
		if (ax != 0) {						
			moveblock(here, top-here, y);	
			top=top-(here-y);
		} else {
			top=y;
		}
		return;
	}

/* 
 *	stage 3, a nontrivial line with linenumber x is to be stored
 *	try to find it first by walking through all lines 
 */
	else {	
		y=ax;
		here2=here;
		here=lnlength;
		nextline();
/* there is no nextline after the first line, we are done */		
		if (ax == 0) return;
/* go back to the beginning */
		here=0; 
		here2=0;
		while (here < top) {
			here3=here2;
			here2=here;
			nextline();
			if (ax > y) break;
		}

/* 
 *	at this point y contains the number of the line to be inserted
 *	x contains the number of the first line with a higher line number
 *	or 0 if the line is to be inserted at the end
 *	here points to the following line and here2 points to the previous line
 */
		if (ax == 0) { 
			here = here3 - lnlength;
			gettoken();
			#if TRACE_PRINTF
			printf("[storeline] %d\n", token);
			#endif
			if (token == LINENUMBER && ax == y) { // we have a double line at the end
				here2-=lnlength;
				here-=lnlength;
				moveblock(here2, linelength, here);
				top=here+linelength;
			}
			return;
		}
		here-=lnlength;
		t1=here;
		here=here2-lnlength;
		t2=here;
		gettoken();
		#if TRACE_PRINTF
		printf("[storeline2] %d\n", token);
		#endif
		if (ax == y) {		/* the line already exists and has to be replaced */
			here2=t2;  		/* this is the line we are dealing with */
			here=t1;   		/* this is the next line */
			y=here-here2;	/* the length of the line as it is  */
			if (linelength == y) {     /* no change in line length */
				moveblock(top-linelength, linelength, here2);
				top=top-linelength;
			} else if (linelength > y) { /* the new line is longer than the old one */
				moveblock(here, top-here, here+linelength-y);
				here=here+linelength-y;
				top=top+linelength-y;
				moveblock(top-linelength, linelength, here2);
				top=top-linelength;
			} else {					/* the new line is short than the old one */
				moveblock(top-linelength, linelength, here2);
				top=top-linelength;
				moveblock(here, top-here, here2+linelength);
				top=top-y+linelength;
			}
		} else {         /* the line has to be inserted in between */
			here=t1;
			moveblock(here, top-here, here+linelength);
			moveblock(top, linelength, here);
		}
	}
}

/* 
 * Layer 1 - the code in this section calculates an expression
 * with a recursive descent algorithm 
 *
 * all function use the stack to pass values back. We use the 
 * Backus-Naur form of basic from here https://rosettacode.org/wiki/BNF_Grammar
 * implementing a C style logical expression model
 */

/* the terminal symbol it ends a statement list - ELSE is one too as it ends a statement list */
static char termsymbol() {
	return (token == LINENUMBER || token == ':' || token == EOL || token == TELSE);
}

/* a little helpers - one token expect */ 
static char expect(token_t t, mem_t e) {
	nexttoken();
	if (token != t) {error(e); return 0; } else return 1;
}

/* a little helpers - expression expect */
static char expectexpr() {
	nexttoken();
	expression();
	if (er != 0) return 0; else return 1;
}

/* parses a list of expression, this may be recursive! */
static void parsearguments() {
	short argsl;

/* begin counting */
	argsl=0; 

/* having 0 args at the end of a command is legal */
	if (!termsymbol()) {

/* list of expressions separated by commas */
		do {
			expression();
			if (er != 0) break;
			argsl++;
			if (token == ',') nexttoken(); else break;
		} while (1);
	}

/* because of the recursion ... */
	args=argsl;
}


/* expect exactly n arguments */
static void parsenarguments(char n) {
	parsearguments();
	if (args != n) error(EARGS);
}

/* counts and parses the number of arguments given in brakets, this function 
	should not advance to the next token because it is called in factor */
static void parsesubscripts() {
	blocation_t l;

	args=0;

	if (DEBUG) {
		outsc("** in parsesubscripts "); outcr();
		bdebug("token "); 
	}

/* remember where we where */
	pushlocation(&l);

/* parsesubscripts is called directly after the object in question, it does 
	nexttoken() itself now */
	nexttoken(); 

/* if we have no bracket here, we return with zero */
	if (token != '(') { 
		poplocation(&l);
		return;
	} 
	nexttoken();

/* if () we return also with -1 */
#ifdef HASMULTIDIM
	if (token == ')') {
		args=-1;
		return; 
	}
#endif

/* now we are ready to parse a set of arguments */
	parsearguments();
	if (!USELONGJUMP && er) return; 
	
	if (token != ')') { error(EARGS); return; } /* we return with ) as a last token on success */

/* we end with the ) still as active token */
}

/* parse a function argument ae is the number of 
	expected expressions in the argument list, parsesubscripts
	should not avance precisely because it is used in factor */

static void parsefunction(void (*f)(), short ae){
	parsesubscripts();
	if (!USELONGJUMP && er) return;
	if (args == ae) f(); else error(EARGS);
}

/* helper function in the recursive decent parser */
static void parseoperator(void (*f)()) {
	mem_t u=1;

	nexttoken();
/* unary minuses in front of an operator are consumed once! */
	if (token == '-') {
		u=-1;
		nexttoken();
	} 
/* the operator */
	f();
	if (er !=0 ) return;
	y=pop();
	if (u == -1) y=-y;
	x=pop();
}

/* 
 * ABS absolute value 
 */
static void xabs() {
	number_t x;

	if ((x=pop())<0) { x=-x; }
	push(x);
}

/*
 * SGN evaluates the sign
 */
static void xsgn() {
	number_t x;

	x=pop();
	if (x>0) x=1;
	if (x<0) x=-1;
	push(x);
}

/*
 * PEEK on an arduino, negative values of peek address 
 * the EEPROM range -1 .. -1024 on an UNO
 */
static void xpeek(){
	number_t a;

/* get the argument from the stack because this is a function only */
	a=pop();

/* the memory and EEPROM range */
	if (a >= 0 && a<=memsize)
		push(memread2(a));
	else if (a < 0 && -a <= elength())
		push(eread(-a-1));
	else {
		error(EORANGE);
		return;
	}
}

/*
 * MAP Arduino map function, we always cast to long, this 
 * makes it potable for various integer sizes and the float.
 */ 
static void xmap() {
	long v, in_min, in_max, out_min, out_max;

	out_max=pop();
	out_min=pop();
	in_max=pop();
	in_min=pop();
	v=pop();
	push((v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

/*
 * RND very basic random number generator with constant seed in 16 bit
 * for float systems, use glibc parameters https://en.wikipedia.org/wiki/Linear_congruential_generator
 */

static void xrnd() {
	number_t r;
	mem_t base = randombase; 

/* the argument of the RND() function */
	r=pop();

/* this is the microsoft mode, argument <0 resets the sequence, 0 always the same number, > 1 a number between 0 and 1 */
	if (randombase < 0) {
		base=0;
		if (r < 0) {
			rd=-r;
			r=1; 
		} else if (r == 0) {
			r=1;
			goto pushresult;
		} else {
			r=1;
		}
	}

/* this is the congruence */
#ifndef HASFLOAT
/* the original 16 bit congruence, the & is needed to make it work for all kinds of ints */
	rd = (31421*rd + 6927) & 0xffff;
#else 
/* glibc parameters */
	rd = (110351245*rd + 12345) & 0x7fffffff; 
#endif

pushresult: 

/* the result is calculated with the right modulus */
#ifndef HASFLOAT
	if (r>=0) 
		push((unsigned long)rd*r/0x10000+base);
	else 
		push((unsigned long)rd*r/0x10000+1-base);
#else
	if (r>=0) 
		push(rd*r/0x80000000+base);
	else 
		push(rd*r/0x80000000+1-base);
#endif
}


#ifndef HASFLOAT
/*
 * SQR - a very simple approximate square root formula 
 *	for integers, for floats we use the library
 */
void sqr(){
	number_t t,r;
	number_t l=0;

	r=pop();
	t=r;
	while (t > 0) {
		t>>=1;
		l++;
	}
	l=l/2;
	t=1;
	t<<=l;
	do {
		l=t;
		t=(t+r/t)/2;
	} while (abs(t-l)>1);
	push(t);
}
#else
static void sqr(){
	push(sqrt(pop()));
}
#endif

/*
 * POW(X, N) evaluates powers
 */ 

/* this function is called by POW(a, c) in BASIC */
static void xpow(){
	number_t n;
	number_t a;

	n=pop();
	a=pop();
	push(bpow(a, n));
}

/* while this is needed by ^*/
number_t bpow(number_t x, number_t y) {
#ifdef HASFLOAT
	return pow(x, y);
#else
	number_t r;
	address_t i;

	r=1;
	if (y>=0) for(i=0; i<y; i++) r*=x; 
	else r=0;
	return r;
#endif
}

/* 
 * the reimplementation of parsestringvar, this code uses a lhsobject 
 * to store the data in, which is somehow more natural with the new code.
 * 
 * The need to handle substring and no substring situations makes thing 
 * complex. 
 * 
 * A plain string has the format A$.
 * A substringed string has the format A$(i) or A$(i,j).
 * A string array has the format A$()(i2), A$(i)(i2), A$(i,j)(i2).
 * In the Microsoft world with substringmode==0 we only have
 * 	A$, A$(i2). 
 * 
 * All this is distiguished here. 
 * 
 */

static void parsestringvar(bstring_t* strp, lhsobject_t* lhs) {
#ifdef HASAPPLE1
	blocation_t l;
	address_t temp;

/* remember the variable name and prep the indices */
	lhs->name=name;
	lhs->i=1; /* we start at 1 */
	lhs->j=arraylimit; /* we assume a string array of length 1, all simple strings are like this */ 
	lhs->i2=0; /* we want the full string length */
	lhs->ps=1; /* we deal with a pure string */

/* the array index default, can vary */
	/* array_index=arraylimit; this is i2 */

/* remember the location */
	pushlocation(&l);

/* and inspect the (first) brackets */
	parsesubscripts();
	if (!USELONGJUMP && er) return; 

	if (DEBUG) { outsc("** in parsestringvar "); outnumber(args); outsc(" arguments \n"); }

/* do we deal with a pure unindexed string or something more complicated */
	if (args == 0) {
/* pure string, we rewind and are done here */
		poplocation(&l); 
	} else if (!substringmode) {
/* we have no substring interpretation hence the brackets can only be one array index */
		if (args == 1) lhs->j=pop(); else { error(EORANGE); return; }
	} else {
/* not a pure string */
		lhs->ps=0;
/* we are in the substring world here */
		if (args == 2) { lhs->i2=popaddress(); args--; } /* A$(i,j) */
		if (!USELONGJUMP && er) return; 
		if (args == 1) { lhs->i=popaddress(); } /* A$(i) */
		if (!USELONGJUMP && er) return; 
		if (args == -1) {}	/* A$(), ignore */
/* here we have parsed a full substring and remember where we are and look forward*/
		pushlocation(&l); 
		nexttoken(); 
		if (token == '(') {
/* a second pair of braces is coming, we parse an array index */
			nexttoken();
			expression(); 
			if (!USELONGJUMP && er) return;
			if (token != ')') { error(EUNKNOWN); return; }
			lhs->j=popaddress();
			if (!USELONGJUMP && er) return;	
		} else 
			poplocation(&l);			
	}

/* in pure parse mode we end here. This is used for the lefthandside code */
	if (!strp) return; 

/* try to get the string */
	getstring(strp, &lhs->name, lhs->i, lhs->j);
	if (!USELONGJUMP && er) return;

/* look what we do with the upper index */
	if (!lhs->i2) lhs->i2=strp->length;

	if (DEBUG) {
		outsc("** in parsestringvar lower is "); outnumber(lhs->i); outcr();
		outsc("** in parsestringvar upper is "); outnumber(lhs->i2); outcr();
		outsc("** in parsestringvar array_index is "); outnumber(lhs->j); outcr();
	}

/* find the length */
	if (lhs->i2-lhs->i+1 > 0) strp->length=lhs->i2-lhs->i+1; else strp->length=0;

/* done */
	if (DEBUG) { 
		outsc("** in parsestringvar, length "); 
		outnumber(strp->length); 
		outsc(" from "); outnumber(lhs->i); outspc(); outnumber(lhs->i2); 
		outcr();
	}

/* restore the name */	
	name=lhs->name;
#else
	return;
#endif
}


/*
 * stringvalue(bstring_t*) evaluates a string value, return 0 if there is no string,
 * 	1 if there is a string. The pointer contains all the data needed to process the string.
 * 
 */

static char stringvalue(bstring_t* strp) {
	address_t k, l;
	address_t i;
	token_t t;
	mem_t args=1;

	lhsobject_t lhs;

	if (DEBUG) outsc("** entering stringvalue \n");

/* make sure everything is nice and clean */
	strp->address=0;
	strp->arraydim=0;
	strp->length=0;
	strp->strdim=0;
	strp->ir=0;

	if ( token == STRING ) {
/* sr has the string information from gettoken and nexttoken*/
		strp->ir=sr.ir;
		strp->length=sr.length;
		strp->address=sr.address;
	}
#ifdef HASAPPLE1
	else if ( token == STRINGVAR ) {
		parsestringvar(strp, &lhs);
	}
	else if ( token == TSTR ) {
		nexttoken();
		if (token == '$') nexttoken();
		if (token != '(') { error(EARGS); return 0; }
		nexttoken();
		expression();
		if (er != 0) return 0;
#ifdef HASFLOAT
		strp->length=writenumber2(sbuffer, pop());
#else
		strp->length=writenumber(sbuffer, pop());
#endif
		strp->ir=sbuffer;
		if (er != 0) return 0;
		if (token != ')') {error(EARGS); return 0; }
	}
#ifdef HASMSSTRINGS
	else if ( token == TCHR ) {
		nexttoken();
		if (token == '$') nexttoken();
		if (token != '(') { error(EARGS); return 0; }
		nexttoken();
		expression();
		if (er != 0) return 0;
		*sbuffer=pop();
		strp->ir=sbuffer;
		strp->length=1;
		if (token != ')') {error(EARGS); return 0; }
	}
	else if ( token == TRIGHT || token == TMID || token == TLEFT ) {
		t=token;
		nexttoken();
		if (token == '$') nexttoken();
		if (token != '(') { error(EARGS); return 0; }
		nexttoken();
		if (token != STRINGVAR) { error(EARGS); return 0; }
		parsestringvar(strp, &lhs);
		if (er != 0) return 0;
		k=strp->length; /* the length of the original string variable */
		nexttoken();
		if (token != ',') { error(EARGS); return 0; }
		nexttoken();
		expression();
		if (er != 0) return 0;
/* all the rest depends on the function */
		if ( t == TRIGHT ) {
			l=popaddress();
			if (k < l) l=k; 
			if (strp->address) strp->address=strp->address+(k-l);
			if (strp->ir) strp->ir=strp->ir+(k-l);
		}
		else if ( t == TLEFT ) {
			l=popaddress();
			if (k < l) l=k; 
		}
		else if ( t == TMID ) {
			if (token == ',') { 
				nexttoken();
				expression();
				if (er != 0) return 0;
				args++;
			}
			if (args == 1) {
				i=popaddress();
				l=0; 
				if (i <= k) l=k-i+1;
			} else {
				l=popaddress();
				if (er != 0) return 0;
				i=popaddress(); 
			}
			if (k < i+l) l=k-i+1;
			if (l < 0) l=0; 
			if (strp->address != 0) strp->address=strp->address+i-1;
			if (strp->ir) strp->ir=strp->ir+i-1;;
		}
		strp->length=l;

		if (token != ')') {error(EARGS); return 0; }
	} 
#endif
#endif
	else {
		return 0;
	}
	return 1;
}


/*
 * (numerical) evaluation of a string expression, used for 
 * comparison and for string rightvalues as numbers
 * the token rewind here is needed as streval is called in 
 * factor - no factor function should nexttoken
 */

static void streval(){
	token_t t;
	address_t k;
	bstring_t s1, s2;
	char* ir;
	address_t a;
	blocation_t l;

/* is the right side of the expression a string */
	if (!stringvalue(&s1)) { error(EUNKNOWN); return; } 
	if (!USELONGJUMP && er) return;

	if (DEBUG) { outsc("** in streval first string"); outcr(); }

/* get ready for rewind to this location */
	pushlocation(&l);
	t=token;

/* is the next token a operator, hence we need to compare two strings? */
	nexttoken();
	if (token != '=' && token != NOTEQUAL) {

/* if not, rewind one token and evaluate the string as a boolean */
		poplocation(&l);
		token=t;

/* a zero length string evaluates to zero else to the first character */
		if (s1.length == 0) push(0); else {
			if (s1.ir) push(s1.ir[0]); 
			else if (s1.address) push(memread2(s1.address)); 
			else error(EGENERAL);
		}

		return; 
	}

/* remember which operator we use */
	t=token;

/* questionable !! */
	nexttoken(); 

	if (DEBUG) { 
		outsc("** in streval second string"); outcr(); 
		debugtoken(); outcr();
	}

/* get the second string */
	if (!stringvalue(&s2)){ error(EUNKNOWN); return; } 
	if (!USELONGJUMP && er) return;

	if (DEBUG) { outsc("** in streval result: "); outnumber(x); outcr(); }

/* different length means unequal */	
	if (s2.length != s1.length) goto neq;

/* and a different character somewhere is also unequal */
#ifdef USEMEMINTERFACE
	if (s1.ir && s2.ir) 
		for (k=0; k < s1.length; k++) { if (s1.ir[k] != s2.ir[k]) goto neq; }
	else if (s1.address && s2.address) 
		for (k=0; k < s1.length; k++) { if (memread2(s1.address+k) != memread2(s2.address+k)) goto neq; }
	else {
		if (s1.address) { a=s1.address; ir=s2.ir; } else { a=s2.address; ir=s1.ir; }
		for (k=0; k < s1.length; k++) { if (memread2(a+k) != ir[k] ) goto neq; }
	}
#else 
	for (k=0; k < s1.length; k++) if (s1.ir[k] != s2.ir[k]) goto neq;
#endif

/* which operator did we use */
	if (t == '=') push(booleanmode); else push(0);
	return;
neq:
	if (t == '=') push(0); else push(booleanmode);
	return;
}


#ifdef HASFLOAT
/* 
 * floating point arithmetic 
 * SIN, COS, TAN, ATAN, LOG, EXP, INT
 * INT is always there and is nop in integer BASICs
 * no handling of floating point errors yet.
 */
void xsin() { push(sin(pop())); }
void xcos() { push(cos(pop())); }
void xtan() { push(tan(pop())); }
void xatan() { push(atan(pop())); }
void xlog() { push(log(pop())); }
void xexp() { push(exp(pop())); }
void xint() { push(floor(pop())); }
#else 
void xint() {}
#endif

/*
 * Recursive expression parser functions 
 *
 * factor(); term(); addexpression(); compexpression();
 * notexpression(); andexpression(); expression() 
 *
 * doing
 *
 * functions, numbers; *, /, %; +, -; =, <>, =>, <=;
 * NOT; AND; OR   
 *
 */

/*
 *	factor() - contrary to all other function
 *	nothing here should end with a new token - this is handled 
 *	in factors calling function
 * 
 * evaluates constants, variables and all functions
 */

/* helpers of factor - array access */
static void factorarray() {
	lhsobject_t object;
	number_t v;

/* remember the variable, because parsesubscript changes this */	
	object.name=name;

/* parse the arguments */
	parsesubscripts();
	if (er != 0 ) return;

	if ( args == 1 ) {
		object.i=popaddress();
		if (!USELONGJUMP && er) return;
		object.j=arraylimit;
	}
#ifdef HASMULTIDIM
	else if ( args == 2 ) {
		object.j=popaddress();
		object.i=popaddress();
		if (!USELONGJUMP && er) return;
	}
#endif
	else {
		error(EARGS); 
		return;
	}
	array(&object, 'g', &v);
	push(v); 
}

/* helpers of factor - string length */
static void factorlen() {
#ifdef HASAPPLE1
	address_t a;
	bstring_t s;
	name_t n;
	lhsobject_t lhs;

	nexttoken();
	if ( token != '(') { error(EARGS); return; }

	nexttoken();
	if (token == STRING ) {
		push(sr.length);
		nexttoken();
	}
	else if (token == STRINGVAR ) {
		parsestringvar(&s, &lhs);
		push(s.length);
		nexttoken();
	}
	else if (
#ifdef HASMSSTRINGS
	token == TRIGHT ||
	token == TLEFT ||
	token == TMID ||
	token == TCHR ||
#endif			
	token == TSTR ) {
		error(EARGS);
		return;
	} else {
		expression();
		if (!USELONGJUMP && er) return;
		n.token=TBUFFER;
		a=pop();
		n.c[0]=a%256;
		n.c[1]=a/256;
		push(blength(&n));
	}

	if (!USELONGJUMP && er) return;

	if (token != ')') { error(EARGS); return; }
#else
	push(0);
#endif
}

/* helpers of factor - the VAL command */
static void factorval() {
	index_t y;
	number_t x;
	bstring_t s;
	address_t a;
	char *ir;
 
	nexttoken();
	if (token != '(') { error(EARGS); return; }

	nexttoken();
	if (!stringvalue(&s)) { error(EUNKNOWN); return; }
	if (!USELONGJUMP && er) return;

/* the length of the strings consumed */
	vlength=0;

/* get the string if it is in serial memory */
#ifdef USEMEMINTERFACE
	if (!s.ir) getstringtobuffer(&s, spistrbuf1, SPIRAMSBSIZE);
#endif	

/* generate a 0 terminated string for use with parsenumber */
	stringtobuffer(sbuffer, &s);
	ir=sbuffer;

/* remove whitespaces */
	while(*ir==' ' || *ir=='\t') { ir++; vlength++; }

/* find a sign */
	if(*ir=='-') { y=-1; ir++; vlength++; } else y=1;
    
	x=0;
#ifdef HASFLOAT
	if ((a=parsenumber2(ir, &x)) > 0) {vlength+=a; ert=0; } else {vlength=0; ert=1;};
#else 
	if ((a=parsenumber(ir, &x)) > 0) {vlength+=a; ert=0; } else {vlength=0; ert=1;};
#endif
	push(x*y);
    
 	nexttoken();
 	if (token != ')') { error(EARGS); return; }
}

/* helpers of factor - the INSTR command */
#ifndef HASFULLINSTR
/* this is instring in a single character version, usefull to split strings */
void factorinstr() {
	char ch;
	address_t a;
	bstring_t s;
			
	nexttoken();
	if (token != '(') { error(EARGS); return; }
	
	nexttoken();		
	if (!stringvalue(&s)) { error(EUNKNOWN); return; }
	if (!USELONGJUMP && er) return;
	nexttoken();
		
	if (token != ',') { error(EARGS); return; }
	
	nexttoken();
	expression();
	if (!USELONGJUMP && er) return;

	ch=pop();
	if (s.address) {
		for (a=1; a<=s.length; a++) {if (memread2(s.address+a-1) == ch) break; }
	} else {
		for (a=1; a<=s.length; a++) {if (s.ir[a-1] == ch) break; }
	}
	if (a > s.length) a=0; 
	push(a);
	//nexttoken();
	if (token != ')') { error(EARGS); return;	}
}
#else
/* the full instr command which can compare two strings  */
static void factorinstr() {
	char ch;
	address_t a=1; 
	address_t i=1;
	bstring_t search;
	bstring_t s;
			
	nexttoken();
	if (token != '(') { error(EARGS); return; }
	nexttoken();

/* the search string */			
	if(!stringvalue(&s)) { error(EUNKNOWN); return; }
	if (!USELONGJUMP && er) return;
	nexttoken();

	if (token != ',') { error(EARGS); return; }
	nexttoken();

/* the string to be searched */			
	if (!stringvalue(&search)) { error(EUNKNOWN); return; }
	if (!USELONGJUMP && er) return;
	nexttoken();

/* potentially the start value */
	if (token == ',') {
		nexttoken();
		expression();
		if (!USELONGJUMP && er) return;
		
		a=popaddress();
		if (!USELONGJUMP && er) return;
	}	

	if (token != ')') { 
		error(EUNKNOWN); 
		return; 
	}

/* health check */
	if (search.length == 0 || search.length+a > s.length || a == 0) {
		push(0);
		return;
	}

/* go through the search string */
	while(i<=search.length) {

/* get one character from the string */
		if (search.address) {
			ch=memread2(search.address+i-1);
		} else {
			ch=search.ir[i-1];
		}

/* search the character */
		if (s.address) {
			for (; a<=s.length; a++) {if (memread2(s.address+a-1) == ch) break; }
		} else {
			for (; a<=s.length; a++) {if ( s.ir[a-1] == ch ) break; }
		}

/* we haven't found the character until the end of the string */
		if (a > s.length) {
			a=0;
			break;
		}

/* next character */
		i+=1;		 

	}

/* how did the search go? a rewind because we were at the end of the search part already */
	if (i <= search.length) {
		a=0;
	} else {
		a=a-search.length+1;
	}
	

	push(a);
}
#endif

/* helpers of factor - the NETSTAT command */
static void factornetstat() {
	address_t x=0;

	if (netconnected()) x=1;
	if (mqttstate() == 0) x+=2;
	push(x);
}

/* helpers of factor - the ASC command, really not needed but for completeness */
static void factorasc() {
#ifdef HASAPPLE1
	bstring_t s;
	lhsobject_t lhs;

	nexttoken();
	if ( token != '(') { error(EARGS); return; }

	nexttoken();
	if ( token == STRING ) {
		if (sr.ir) push(sr.ir[0]); else push(memread2(sr.address));
		nexttoken();
	}
	else if ( token == STRINGVAR ) {
		parsestringvar(&s, &lhs);
		if (s.length > 0) {
			if (s.ir) push(s.ir[0]); else push(memread2(s.address));
		} else 
			push(0);
		nexttoken();
	} else {
		error(EARGS);
		return;
	}

	if (!USELONGJUMP && er) return;

	if (token != ')') { error(EARGS); return; }
#else 
	push(0);
#endif
}

static void factor(){
	if (DEBUG) bdebug("factor\n");
	if ( token == NUMBER ) {
		push(x);
		return;
	}
	if ( token == VARIABLE ) {
		push(getvar(&name));
		return;
	}
	if ( token == ARRAYVAR ) {
		factorarray();
		return;
	}
	if ( token == '(' ) {
		nexttoken();
		expression();
		if (er != 0 ) return;
		if (token != ')') { error(EARGS); return; } 
		return;
	}
/* Palo Alto BASIC functions */
	if ( token == TABS ) {
		parsefunction(xabs, 1);
		return;
	}
	if ( token == TRND ) {
		parsefunction(xrnd, 1);
		return;
	}
	if ( token == TSIZE ) {
		push(himem-top);
		return;
	}
/* Apple 1 BASIC functions */
#ifdef HASAPPLE1
	if ( token == TSGN ) {
		parsefunction(xsgn, 1);
		return;
	}
	if ( token == TPEEK ) {
		parsefunction(xpeek, 1);
		return;
	}
	if ( token == TLEN ) {
		factorlen();
		return;
	}
#ifdef HASIOT
	if ( token == TAVAIL ) {
		parsefunction(xavail, 1);
		return;
	}
	if ( token == TOPEN ) {
		parsefunction(xfopen, 1);
		return;
	}
	if ( token == TSENSOR ) {
		parsefunction(xfsensor, 2);
		return;
	}
	if ( token == TVAL ) {
		factorval();
		return;
	}
	if ( token == TINSTR ) {
		factorinstr();
		return;
	}
	if ( token == TWIRE ) {
		parsefunction(xfwire, 1);
		return;
	}
#endif
#ifdef HASERRORHANDLING
	if ( token == TERROR ) {
		push(erh);
		return;
	}
#endif
	if ( token == THIMEM ) {
		push(himem);
		return;
	}
/* Apple 1 string compare code */
	if ( token == STRING || 
#ifdef HASIOT
		token == TSTR ||
#endif
#ifdef HASMSSTRINGS
		token == TLEFT ||
		token == TRIGHT ||
		token == TMID ||
		token == TCHR ||
#endif
		token == STRINGVAR ) {
		streval();
		return;
	}
#endif
/*  Stefan's tinybasic additions */
#ifdef HASSTEFANSEXT
	if ( token == TSQR ) {
		parsefunction(sqr, 1);
		return;
	}
	if ( token == TMAP ) {
		parsefunction(xmap, 5);
		return;
	}
	if ( token == TUSR ) {
		parsefunction(xusr, 2);
		return;
	}
	if ( token == TPOW ) {
		parsefunction(xpow, 2);
		return;
	}
#endif
/* Arduino I/O */
#ifdef HASARDUINOIO
	if ( token == TAREAD ) {
		parsefunction(xaread, 1);
		return;
	}
	if ( token == TDREAD ) {
		parsefunction(xdread, 1);
		return;
	}
	if ( token == TMILLIS ) {
		parsefunction(bmillis, 1);
		return;
	}
#ifdef HASPULSE
	if ( token == TPULSE ) {
		parsefunction(bpulsein, 3);
		return;
	}
#endif
	if ( token == TAZERO ) {
#if defined(ARDUINO) && defined(A0)
		push(A0);
#else 
		push(0);
#endif			
		return;
	}
	if ( token == TLED ) {
#ifdef LED_BUILTIN
		push(LED_BUILTIN);
#else
		push(0);
#endif
		return;
	}
#endif
/* mathematical functions in case we have float */
#ifdef HASFLOAT
	if ( token == TSIN ) {
		parsefunction(xsin, 1);
		return;
	}
	if ( token == TCOS ) {
		parsefunction(xcos, 1);
		return;
	}
	if ( token == TTAN ) {
		parsefunction(xtan, 1);
		return;
	}
	if ( token == TATAN ) {
		parsefunction(xatan, 1);
		return;
	}
	if ( token == TLOG ) {
		parsefunction(xlog, 1);
		return;
	}
	if ( token == TEXP ) {
		parsefunction(xexp, 1);
		return;
	}
#endif
/* int is always present to make programs compatible */
	if ( token == TINT ) {
		parsefunction(xint, 1);
		return;
	}
#ifdef HASDARTMOUTH
	if ( token == TFN ) {
		xfn(0);
		return;
	}
/* an arcane feature, DATA evaluates to the data record number */
	if ( token == TDATA ) {
		push(datarc);
		return;
	}
#endif
#ifdef HASDARKARTS
	if ( token == TMALLOC ) {
		parsefunction(xmalloc, 2);
		return;
	}
	if ( token == TFIND ) {
		xfind();
		return;
	}
#endif
#ifdef HASIOT
	if ( token == TNETSTAT ) {
		factornetstat();
		return;
	}
#endif
#ifdef HASMSSTRINGS
	if ( token == TASC ) {
		factorasc();
		return;
	}
#endif
/* unknown function */
	error(EUNKNOWN);
	return;
}

/* this is how the power operator ^ is handled */
#ifdef POWERRIGHTTOLEFT
/* the recursive version */
void power() { 
	if (DEBUG) bdebug("power\n"); 
	factor();
	if (!USELONGJUMP && er) return;

	nexttoken(); 
	if (DEBUG) bdebug("in power\n");
	if (token == '^'){
		parseoperator(power);
		if (!USELONGJUMP && er) return;
		push(bpow(x,y));
	} 
	if (DEBUG) bdebug("leaving power\n");
}
#else 
/* the left associative version */
static void power() {
	if (DEBUG) bdebug("power\n"); 
	factor();
	if (!USELONGJUMP && er) return;

nextpower:
	nexttoken(); 
	if (DEBUG) bdebug("in power\n");
	if (token == '^'){
		parseoperator(factor);
		push(bpow(x,y));
		goto nextpower;
	} 
	if (DEBUG) bdebug("leaving power\n");
}
#endif

/*
 *	term() evaluates multiplication, division and mod
 */
static void term(){
	if (DEBUG) bdebug("term\n"); 
	power();
	if (!USELONGJUMP && er) return;

nextfactor:
	if (DEBUG) bdebug("in term\n");
	if (token == '*'){
		parseoperator(power);
		if (!USELONGJUMP && er) return;
		push(x*y);
		goto nextfactor;
	} else if (token == '/'){
		parseoperator(power);
		if (!USELONGJUMP && er) return;
		if (y != 0)
#ifndef HASFLOAT
			push(x/y);
#else
			if (forceint) push((int)x/(int)y); else push(x/y);
#endif
		else {
			error(EDIVIDE);
			return;	
		}
		goto nextfactor;
	} else if (token == '%') {
		parseoperator(power);
		if (!USELONGJUMP && er) return;
		if (y != 0)
#ifndef HASFLOAT
			push(x%y);
#else 
			push((int)x%(int)y);
#endif
		else {
			error(EDIVIDE);
			return;	
		}
		goto nextfactor;
	}
	if (DEBUG) bdebug("leaving term\n");
}

/* add and subtract */
static void addexpression(){
	if (DEBUG) bdebug("addexp\n");
	if (token != '+' && token != '-') {
		term();
		if (!USELONGJUMP && er) return;
	} else {
		push(0);
	}

nextterm:
	if (token == '+' ) { 
		parseoperator(term);
		if (!USELONGJUMP && er) return;
		push(x+y);
		goto nextterm;
	} else if (token == '-'){
		parseoperator(term);
		if (!USELONGJUMP && er) return;
		push(x-y);
		goto nextterm;
	}
}

/* comparisions */
void compexpression() {
	if (DEBUG) bdebug("compexp\n"); 
	addexpression();
	if (!USELONGJUMP && er) return;
	if ( token == '=' ) {
		parseoperator(compexpression);
		if (!USELONGJUMP && er) return;
#ifndef HASFLOAT
		push(x == y ? booleanmode : 0);
#else 
		if (fabs(x-y) <= epsilon) push(booleanmode); else push(0);
#endif
		return;
	}
	if ( token == NOTEQUAL ) {
		parseoperator(compexpression);
		if (!USELONGJUMP && er) return;
#ifndef HASFLOAT
		push(x != y ? booleanmode : 0);
#else 
		if (fabs(x-y) > epsilon) push(booleanmode); else push(0);
#endif
		return;
	}
	if ( token == '>' ) {
		parseoperator(compexpression);
		if (!USELONGJUMP && er) return;
		push(x > y ? booleanmode : 0);
		return;
	}
	if ( token == '<' ) {
		parseoperator(compexpression);
		if (!USELONGJUMP && er) return;
		push(x < y ? booleanmode : 0);
		return;
	}
	if ( token == LESSEREQUAL ) {
		parseoperator(compexpression);
		if (!USELONGJUMP && er) return;
		push(x <= y ? booleanmode : 0);
		return;
	}
	if ( token == GREATEREQUAL ) {
		parseoperator(compexpression);
		if (!USELONGJUMP && er) return;
		push(x >= y ? booleanmode : 0);
		return;
	}
}

#ifdef HASAPPLE1
/* boolean NOT */
static void notexpression() {
	if (DEBUG) bdebug("notexp\n");
	if (token == TNOT) {
		nexttoken();
		expression();
		if (!USELONGJUMP && er) return;
		if (booleanmode == -1) push(~(short)pop());
		else if (pop() == 0) push(1); else push(0);
	} else 
		compexpression();
}

/* boolean AND and at the same time bitwise */
static void andexpression() {
	if (DEBUG) bdebug("andexp\n");
	notexpression();
	if (!USELONGJUMP && er) return;
	if (token == TAND) {
		parseoperator(expression);
		if (!USELONGJUMP && er) return;
		push((short)x & (short)y);
	} 
}

/* expression function and boolean OR at the same time bitwise !*/
static void expression(){
	if (DEBUG) bdebug("exp\n"); 
	andexpression();
	if (!USELONGJUMP && er) return;
	if (token == TOR) {
		parseoperator(expression);
		if (!USELONGJUMP && er) return;
		push((short)x | (short)y); 
	}  
}
#else 

/* expression function simplified */
void expression(){
	if (DEBUG) bdebug("exp\n"); 
	compexpression();
	if (!USELONGJUMP && er) return;
	if (token == TOR) {
		parseoperator(expression);
		if (!USELONGJUMP && er) return;
		push((short)x | (short)y);
	}  
}
#endif

/* 
 * Layer 2 - The commands and their helpers
 *    
 * Palo Alto BASIC languge set - PRINT, LET, INPUT, GOTO, GOSUB, RETURN,
 *    	IF, FOR, TO, NEXT, STEP, BREAK, STOP, END, LIST, NEW, RUN, REM
 *    	BREAK is not Palo ALto but fits here, eEND is identical to STOP.
 */

/*
 *	PRINT command, extended by many features like file, wire, mqtt and radio i/o.
 *  TAB added as part of the PRINT statement with C64 compatibility.
 */
void xprint(){
	char semicolon = 0;
	char oldod;
	char modifier = 0;
	bstring_t s;
	stringlength_t i;
	
	form=0;
	oldod=od;
	nexttoken();

processsymbol:
/* at the end of a print statement, do we need a newline, restore the defaults */
	if (termsymbol()) {
		if (!semicolon) outcr();
		od=oldod;
		form=0;
		return;
	}
	semicolon=0;

/* output a string if we found it */
	if (stringvalue(&s)) {
		if (!USELONGJUMP && er) return;

/* buffer must be used here for machine code to work */
#ifdef USEMEMINTERFACE
		if (!s.ir) getstringtobuffer(&s, spistrbuf1, SPIRAMSBSIZE);
#endif
 		outs(s.ir, s.length);

 		nexttoken();
		goto separators;
	}

/* the tab command as part of print */
#ifdef HASMSSTRINGS
	if (token == TTAB || token == TSPC) {
		xtab();
		goto separators;
	}
#endif

/* modifiers of the print statement */
	if (token == '#' || token == '&') {
		modifier=token;
		nexttoken();
		expression();
		if (!USELONGJUMP && er) return;

		if ( modifier == '#' ) {
			form=pop();
		} else if ( modifier == '&' ) {
			od=pop();
		}
		goto separators;
	}

	if (token != ',' && token != ';') {
		expression();
		if (!USELONGJUMP && er) return;
		outnumber(pop());
	}

/* commas and semicolons, all other symbols are accepted and no error is thrown */
separators:
	if (termsymbol()) goto processsymbol;

	if ( token == ',' ) {
		if (!modifier) outspc();
		goto cont;
	}
	if ( token == ';') {
cont:
		semicolon=1;
		nexttoken();
	}
	modifier=0;

	goto processsymbol;
}

/* 
 *	LET assigment code for various lefthand and righthand side. 
 *
 *
 *	lefthandside is a helper function for reuse in other 
 *	commands. It determines the address the value is to be 
 *	assigned to and whether the assignment target is a 
 *	"pure" i.e. subscriptless string expression
 *
 *	assignnumber assigns a number to a given lefthandside
 *
 * In lefthandside the type of the object is determined and 
 * possible subscripts are parsed.
 * 
 * Variables have no subscripts. The arguments are unchanged.
 * Arrays may have two subscript which will go to i, j
 * Strings may have a string subscript, going to i and an array subscript 
 * going to j
 * String arrays use i2 as the array index. 
 * Note that the role of the variables differs for string array and normal
 * arrays. 
 * Strings without a subscript i.e. pure strings, set the ps flag
 * 
 */

/* the new lefthandsite code */
static void lefthandside(lhsobject_t* lhs) {

/* just to provide it for parsestringvar to reuse the righthandside code */
	address_t temp;

	if (DEBUG) {
		outsc("assigning to variable "); 
		outname(&lhs->name); outspc(); 
		outsc(" type "); outnumber(lhs->name.token); 
		outcr();
	}

/* prep it */
	lhs->i=1;
	lhs->i2=0;
	lhs->j=arraylimit;
	lhs->ps=1;

/* look at the variables and continue parsing */
	register token_t t = lhs->name.token;
	if ( t == VARIABLE ) {
		nexttoken();
	}
	else if ( t == ARRAYVAR ) {
		parsesubscripts();
		if (!USELONGJUMP && er) return;
		if ( args == 1 ) {
			lhs->i=popaddress();
			if (!USELONGJUMP && er) return;
			lhs->j=arraylimit;
		}
		else if ( args == 2 ) {
			lhs->j=popaddress();
			if (!USELONGJUMP && er) return;
			lhs->i=popaddress();
			if (!USELONGJUMP && er) return;
		}
		else {
			error(EARGS);
			return;
		}
		nexttoken();
	}
#ifdef HASAPPLE1
	else if ( t == STRINGVAR ) {
		parsestringvar(0, lhs);
		nexttoken();
	}
#else /* HASAPPLE1 */
/* here we could implement a string thing for a true Tinybasic */
#endif
	else {
		error(EUNKNOWN);
		return;
	}

	if (DEBUG) {
		outsc("** in assignment lefthandside with (i,j,ps,i2) ");
		outnumber(lhs->i); outspc();
		outnumber(lhs->j); outspc();
		outnumber(lhs->ps); outspc();
		outnumber(lhs->i2); outcr();
		outsc("   token is "); outputtoken(); 
		outsc("   at "); outnumber(here); outcr();
	}
}

/* assign a number to a left hand side we have parsed */
static void assignnumber2(lhsobject_t* lhs, number_t x) {
	bstring_t sr;

/* depending on the variable type, assign the value */
	register token_t t = lhs->name.token;
	if ( t == VARIABLE ) {
		setvar(&lhs->name, x);
	}
	else if ( t == ARRAYVAR ) {
		array(lhs, 's', &x);
	}
#ifdef HASAPPLE1
	else if ( t == STRINGVAR ) {
/* find the string variable */
		getstring(&sr, &lhs->name, lhs->i, lhs->j);
		if (!USELONGJUMP && er) return;
/* the first character of the string is set to the number */
		if (sr.ir) sr.ir[0]=x; else if (sr.address) memwrite2(ax, x); else error(EUNKNOWN);
/* set the length */
		if (lhs->ps)
			setstringlength(&lhs->name, 1, lhs->j);
		else 
			if (sr.length < lhs->i && lhs->i <= sr.strdim) 
				setstringlength(&lhs->name, lhs->i, lhs->j);
	}
#endif
}

/*
 *	LET - the core assigment function, this is different from other BASICs
 */
static void assignment() {
	address_t newlength, copybytes;
	mem_t s;
	index_t k;
	char tmpchar; /* for number conversion only */
	bstring_t sr, sl; /* the right and left hand side strings */

/* the lefthandside identifier */
	lhsobject_t lhs;

/* this code evaluates the left hand side, we remember the object information first */
	lhs.name=name;

	lefthandside(&lhs);
	if (!USELONGJUMP && er) return;

/* the assignment part */
	if (token != '=') { error(EUNKNOWN); return; }
	nexttoken();

/* here comes the code for the right hand side, the evaluation depends on the left hand side type */
	register token_t t = lhs.name.token;
/* the lefthandside is a scalar, evaluate the righthandside as a number, even if it is a string */
	if ( t == VARIABLE || t == ARRAYVAR ) {
		expression();
		if (!USELONGJUMP && er) return;
		assignnumber2(&lhs, pop());
	}
#ifdef HASAPPLE1
/* the lefthandside is a string variable, try evaluate the righthandside as a stringvalue */
	else if ( t == STRINGVAR ) {
nextstring:
/* do we deal with a string as righthand side */
		s=stringvalue(&sr);
		if (!USELONGJUMP && er) return;

/* and then as an expression if it is no string, any number appearing in a string expression terminates the addition loop */
		if (!s) {
			expression();
			if (!USELONGJUMP && er) return;
			tmpchar=pop();
			sr.length=1;
			sr.ir=&tmpchar;
		} else 
			nexttoken(); /* we do this here because expression also advances, this way we avoid double advance */

		if (DEBUG) { outsc("* assigment stringcode at "); outnumber(here); outcr();	}

/* at this point we  have a stringvalue with ir2 pointing to the payload and the stack the length 
	this is either coming from stringvalue or from the expression code */

/* we now process the source string */		

/* getstring of the destination */         
		getstring(&sl, &lhs.name, lhs.i, lhs.j);
		if (!USELONGJUMP && er) return;

/* this debug messes up sbuffer hence all functions that use it in stringvalue produce wrong results */
		if (DEBUG) {
			outsc("* assigment stringcode "); outname(&lhs.name); outcr();
			outsc("** assignment source string length "); outnumber(sr.length); outcr();
			outsc("** assignment dest string length "); outnumber(sl.length); outcr();
			outsc("** assignment dest string dimension "); outnumber(sl.strdim); outcr();
		}

/* does the source string fit into the destination if we have no destination second index*/
		if ((lhs.i2 == 0) && ((lhs.i+sr.length-1)>sl.strdim)) { error(EORANGE); return; };

/* if we have a second index, is it in range */
		if ((lhs.i2 != 0) && lhs.i2>sl.strdim) { error(EORANGE); return; };

/* calculate the number of bytes we truely want to copy */
		if (lhs.i2 > 0) copybytes=((lhs.i2-lhs.i+1) > sr.length) ? sr.length : (lhs.i2-lhs.i+1); 
		else copybytes=sr.length;

		if (DEBUG) { outsc("** assignment copybytes "); outnumber(copybytes); outcr(); }

/* now do the heavy lifting in a seperate function to encasulate buffering */
		assignstring(&sl, &sr, copybytes);

/* 
 * classical Apple 1 behaviour is string truncation in substring logic, with
 * two index destination string we follow another route. We extend the string 
 * for the number of copied bytes
 */
		if (lhs.i2 == 0) {
			newlength = lhs.i+sr.length-1;	
		} else {
			if (lhs.i+copybytes > sl.length) newlength=lhs.i+copybytes-1; 
			else newlength=sl.length;
		} 

		setstringlength(&lhs.name, newlength, lhs.j);
/* 
 * we have processed one string and copied it fully to the destination 
 * see if there is more to come. For inplace strings this is odd because 
 * one term can change during adding (A$ = B$ + A$).
 */
addstring:
		if (token == '+') {
			lhs.i=lhs.i+copybytes;
			nexttoken();
			goto nextstring;
		}
	}
#endif /* HASAPPLE1 */
}


/* 
 * Try to copy one string to the other, assumes that getstring did its work
 * and that copybyte is correct.
 * BASICs in place strings make this a non trivial exercise as we need to 
 * avoid overwrites. 
 * Another complication is the mixed situation of BASIC memory strings 
 * and C memory strings.
 */
static void assignstring(bstring_t* sl, bstring_t* sr, stringlength_t copybytes) {
	stringlength_t k;

/* if we have a memory model that needs the mem interface, go through the addresses by default 
	else use just the pointers */

#ifdef USEMEMINTERFACE
/* for a regular string variable as left hand side we know the address */
	if (sl->address) {

/* for a regular string variable as a source we need to take care of order */

		if (sr->address) {
			if (sr->address > sl->address) 
				for (k=0; k<copybytes; k++) memwrite2(sl->address+k, memread2(sr->address+k));
			else 
				for (k=1; k<=copybytes; k++) memwrite2(sl->address+copybytes-k, memread2(sr->address+copybytes-k));
		} else {

/* if the right hand side is a special string or a constant things are much simpler */

			for (k=0; k<copybytes; k++) memwrite2(sl->address+k, sr->ir[k]);

		}
	} else {

/* non regular string variables like @U$ and @T$ are never assignable */
		error(EUNKNOWN); 
	}
#else

/* we just go through the C memory here */

	if (sr->ir && sl->ir) {
		if (sr->ir > sl->ir) 
			for (k=0; k<copybytes; k++) sl->ir[k]=sr->ir[k];
		else 
			for (k=1; k<=copybytes; k++) sl->ir[copybytes-k]=sr->ir[copybytes-k];
	} else { 
		error(EUNKNOWN); 
	}

#endif
}

/*
 * INPUT ["string",] variable [,["string",] variable]
 *
 * The original version of input only processes simple variables one at a time 
 * and does not support arrays. The code is redudant to assignment and read. 
 * It also does not support comma separated lists of values to be input.
 */
static void showprompt() {
	outsc("? ");
}

/* 
 * Reimplementation of input using the same pattern as read and print . 
 */
void xinput() {

	mem_t oldid = id; /* remember the stream on modify */
	mem_t prompt = 1; /* determine if we show the prompt */
	number_t xv; /* for number conversion with innumber */

/* the identifier of the lefthandside */
	lhsobject_t lhs;

	address_t maxlen, newlength; /* the maximum length of the string to be read */
	int k=0; /* the result of the number conversion */
	bstring_t s;
	char* buffer; /* the buffer we use for input */
	address_t bufsize; /* the size of the buffer */

/* depending on the RUN state we use either the input buffer or the string buffer */
/* this ways we can process long inputs in RUN and don't need a lot of memory */
	if (st == SRUN || st == SERUN) {
		buffer=ibuffer; 
		bufsize=BUFSIZE;
	} else {
		buffer=sbuffer;
		bufsize=SBUFSIZE;
	}

/* get the next token and check what we are dealing with */
	nexttoken();

/* modifiers of the input statement (stream) */
	if (token == '&') {
		if(!expectexpr()) return;
		oldid=id;
		id=pop();
		if (id != ISERIAL || id !=IKEYBOARD) prompt=0;
		if (token != ',') {
			error(EUNKNOWN);
			return;
		} else 
			nexttoken();
	}

/* unlike print, form can appear only once in input after the
		stream, it controls character counts in wire */
	if (token == '#') {
		if(!expectexpr()) return;
		form=pop();
		if (token != ',') {
			error(EUNKNOWN);
			return;
		} else 
			nexttoken();
	}

/* we have a string to be printed to prompt the user */
nextstring:
	if (token == STRING && id != IFILE) {
		prompt=0;
#ifdef USEMEMINTERFACE
		if (!sr.ir) getstringtobuffer(&sr, spistrbuf1, SPIRAMSBSIZE);
#endif
		outs(sr.ir, sr.length);
		nexttoken();
	}

/* now we check for a variable and parse it */
nextvariable:
	if (token == VARIABLE || token == ARRAYVAR || token == STRINGVAR) {  

/* check for a valid lefthandside expression */ 
		lhs.name=name;

		lefthandside(&lhs);
		if (!USELONGJUMP && er) return;

/* which data type do we input */
		register token_t t = lhs.name.token;
		if ( t == VARIABLE || t == ARRAYVAR ) {
again:
/* if we have no buffer or are at the end, read it and set cursor k to the beginning */
			if (k == 0 || (address_t) buffer[0] < k) { 
				if (prompt) showprompt();
				(void) ins(buffer, bufsize); 
				k=1;
			}

/* read a number from the buffer and return it, advance the cursor k */
			k=innumber(&xv, buffer, k);

/* if we break, end it here */
			if (k == -1) {
				st=SINT;
				token=EOL;
				goto resetinput;
			}

/* if we have no valid number, ask again */
			if (k == 0) {
				if (id == ISERIAL || id == IKEYBOARD) {
					printmessage(ENUMBER); 
					outspc(); 
					printmessage(EGENERAL);
					outcr();
					xv=0;
					k=0;
					goto again; 
				} else { 
					ert=1; 
					xv=0; 
					goto resetinput;
				}
			}

/* now assign the number */
			assignnumber2(&lhs, xv);
			
/* look if there is a comma coming in the buffer and keep it */
			while (k < (address_t) buffer[0] && buffer[k] != 0) {
				if (buffer[k] == ',') {
					k++;
					break;
				}
				k++;
			}
		}
#ifdef HASAPPLE1
		else if ( t == STRINGVAR ) {
/* the destination address of the lefthandside, on the fly create included */
			getstring(&s, &lhs.name, lhs.i, lhs.j);
			if (!USELONGJUMP && er) return;

/* the length of the lefthandside string */
			if (lhs.i2 == 0) {
				maxlen=s.strdim-lhs.i+1;
			} else {
				maxlen=lhs.i2-lhs.i+1;
				if (maxlen > s.strdim) maxlen=s.strdim-lhs.i+1;
			}

/* the number of bytes we want to read the form parameter in WIRE can be used 
	to set the expected number of bytes */
			if (form != 0 && form < maxlen) maxlen=form;

/* what is going on */
			if (DEBUG) {
				outsc("** input stringcode at "); outnumber(here); outcr();
				outsc("** input stringcode "); outname(&lhs.name); outcr();
				outsc("** input stringcode maximum length "); outnumber(maxlen); outcr();
			}

/* now read the string inplace */
			if (prompt) showprompt();
#ifndef USEMEMINTERFACE	
			newlength=ins(s.ir-1, maxlen);
#else
			newlength=ins(spistrbuf1, maxlen);

/* if we have a string variable, we need to copy the buffer to the string */
			if (newlength > 0) {
				if (s.ir) {
					for (k=0; k<newlength; k++) s.ir[k]=spistrbuf1[k+1];
				} else {
					for (k=0; k<newlength; k++) memwrite2(s.address+k, spistrbuf1[k+1]);
				}
			}
#endif

/* if we have a string variable, we need to copy the buffer to the string */


/* set the right string length */
/* classical Apple 1 behaviour is string truncation in substring logic */
			newlength = lhs.i+newlength-1;	
			setstringlength(&lhs.name, newlength, lhs.j);
		}
#endif		
	}

/* seperators and termsymbols */
	if (token == ',' || token == ';') {
		nexttoken();
		goto nextstring;
	}

/* no further data */
	if (!termsymbol()) {
		error(EUNKNOWN);
	}

resetinput:
	id=oldid;
	form=0;
}

/*
 *	GOTO, GOSUB, RETURN and their helpers
 *
 *	GOTO and GOSUB function for a simple one statement goto
 */
static void xgoto() {
	token_t t=token;
	number_t x;

	if (!expectexpr()) return;
	if (t == TGOSUB) pushgosubstack(0);
	if (!USELONGJUMP && er) return;

	x=pop();

	if (DEBUG) { outsc("** goto/gosub evaluated line number "); outnumber(x); outcr(); }
	#if TRACE_PRINTF
	printf("[xgoto]\n");
	#endif
	findline((address_t) x);
	if (!USELONGJUMP && er) return;
	if (DEBUG) { outsc("** goto/gosub branches to "); outnumber(here); outcr(); }

/* goto in interactive mode switched to RUN mode
		no clearing of variables and stacks */
	if (st == SINT) st=SRUN;
}

/*
 *	RETURN retrieves here from the gosub stack
 */
static void xreturn(){ 
	popgosubstack();
	if (DEBUG) { outsc("** restored location "); outnumber(here); outcr(); }
	if (!USELONGJUMP && er) return;
	nexttoken();
#ifdef HASEVENTS 
/* we return from an interrupt and reenable them */
  if (gosubarg[gosubsp] == TEVENT) events_enabled=1;
#endif
}

/* 
 *	IF statement together with THEN 
 */
static void xif() {
	mem_t nl=0;
	
	if (!expectexpr()) return;
	x=pop();
	if (DEBUG) { outsc("** in if, condition "); outnumber(x); outcr(); } 

/* if can have a new line after the expression in this BASIC */
	if (token == LINENUMBER) nexttoken();	

/* we only check false which is 0 */
	if (x == 0)  {
#ifndef HASSTRUCT
/* on condition false skip the entire line and all : until a potential ELSE */
		while(token != LINENUMBER && token != EOL && token != TELSE) nexttoken();
#else 
/* in the structured language set, we need to look for a DO  close to the IF and skip it*/
/* a THEN or not and then a line number expects a block */
		if (token == TTHEN) nexttoken();
		if (token == LINENUMBER) { nexttoken(); nl=1; }

/* skip the block */
		if (token == TDO) { 
			nexttoken();
			findbraket(TDO, TDEND); 
			nexttoken();
			goto processelse;
		} 

/* skip the line */
		if (!nl) while(token != LINENUMBER && token != EOL && token != TELSE) nexttoken();

processelse:		
#endif

/* if we have ELSE at this point we want to execute this part of the line as the condition 
		was false, isolated ELSE is GOTO, otherwise just execute the code */

#ifdef HASSTEFANSEXT
/* look if ELSE is at the next line */
		if (token == LINENUMBER) nexttoken();

/* now process ELSE */
		if (token == TELSE) {
			nexttoken();
			if (token == NUMBER) {
				#if TRACE_PRINTF
				printf("[xif else]\n");
				#endif
				findline((address_t) x);
				return;	
			}
		}
#endif		
	}	

/* a THEN is interpreted as simple one statement goto	if it is followed by a line number*/
#ifdef HASAPPLE1
/* then can be on a new line */
	if (token == TTHEN) {
		nexttoken();
		if (token == NUMBER) {
			#if TRACE_PRINTF
			printf("[xif]\n");
			#endif
			findline((address_t) x);
		}
	} 
#endif
}

/* if else is encountered in the statement line, the rest of the code is skipped 
 		as else code execution is triggered in the xif function */
#ifdef HASSTEFANSEXT
static void xelse() {
	mem_t nl=0;

#ifndef HASSTRUCT
/* skip the entire line */
	while(token != LINENUMBER && token != EOL) nexttoken();
#else 
	nexttoken();
/* else in a single line */
	if (token == LINENUMBER) { 
		nexttoken(); 
		nl=1; 
	}

/* the block after the else on a new line or the current line */
	if (token == TDO) {
		nexttoken();
		findbraket(TDO, TDEND);
	}

/* single line else, skip the line */
	if (!nl) while(token != LINENUMBER && token != EOL) nexttoken();

#endif
}
#endif

/* 
 *	FOR, NEXT and the apocryphal BREAK
 *
 * find the NEXT token or the end of the program
 */ 

/* 
 * The generic block scanner, used for structured code and in FOR NEXT.
 * The closing symbol of a symbol is found. Symbol pairs are: 
 * 
 * 	FOR NEXT
 * 	WHILE WEND
 *  REPEAT UNTIL
 * 	DO DEND
 *  SWITCH SWEND
 *  
 */
static void findbraket(token_t bra, token_t ket){
	address_t fnc = 0;

	while (1) {
		
		if (DEBUG) { 
			outsc("** skpping braket "); 
			outputtoken(); outspc(); 
			outnumber(here); outspc(); 
			outnumber(fnc); outcr(); 
		}		

		if (token == ket) {
	    	if (fnc == 0) return; else fnc--;
		}

		if (token == bra) fnc++;

/* no closing symbol found */
		if (token == EOL) {
			error(bra);
	    	return;
		}
		nexttoken();
	}
}

/*
 *	FOR variable [= expression [to expression]] [STEP expression]
 *	for stores the variable, the increment and the boudary on the 
 *	for stack. Changing steps and boundaries during the execution 
 *	of a loop has no effect.
 *
 *	This is different from many other BASICS as FOR can be used 
 *	as an open loop with no boundary
 */
static void xfor() {
	name_t variable;
	number_t begin=1;
	number_t to=maxnum;
	number_t step=1;
	
/* we need at least a name */
	if (!expect(VARIABLE, EUNKNOWN)) return;
	variable=name;

/* 
 * This is not standard BASIC.
 * All combinations of FOR TO STEP are allowed.
 * FOR X : NEXT is an infinite loop.
 * FOR X=1 : NEXT is a loop with X=1 and an infinite loop.
 * FOR X=1 TO 10 : NEXT is a loop with X=1 to 10.
 * FOR X=1 TO 10 STEP 2 : NEXT is a loop with X=1 to 10 in steps of 2.
 * A variable must always be supplyed to indentify the loop.
 * FOR : NEXT is illegal.
 */
	nexttoken();
	if (token == '=') { 
		if (!expectexpr()) return;
		begin=pop();
		setvar(&variable, begin);
	}  

	if (token == TTO) {
		if (!expectexpr()) return;
		to=pop();
	}

	if (token == TSTEP) {
		if (!expectexpr()) return;
		step=pop();
	} 

	if (!termsymbol()) {
		error(EUNKNOWN);
		return;
	}

/* in interactive mode we reuse here to store the offset in the buffer */
	if (st == SINT) here=bi-ibuffer;

/*  here we know everything to set up the loop */

	if (DEBUG) { 
		outsc("** for loop with parameters var begin end step: ");
		outname(&variable); 
		outspc(); outnumber(begin); 
		outspc(); outnumber(to); 
		outspc(); outnumber(step); 
		outcr();
		outsc("** for loop target location "); outnumber(here); outcr();
	}

	pushloop(&variable, TFOR, here, to, step);
	if (!USELONGJUMP && er) return;

/*
 *	This tests the condition and stops if it is fulfilled already from start.
 *	There is another apocryphal feature here: STEP 0 is legal triggers an infinite loop.
 */
	if ((step > 0 && getvar(&variable) > to) || (step < 0 && getvar(&variable) < to)) { 
		droploop();
		findbraket(TFOR, TNEXT);
		nexttoken();
		if (token == VARIABLE) nexttoken(); /* This BASIC does not check. */
	}
}

/*
 *	BREAK - an apocryphal feature here is the BREAK command ending a loop
 */
#ifdef HASSTRUCT
static void xbreak() {
	bloop_t* loop;

	loop=activeloop();
	if (!USELONGJUMP && er) return;
	register token_t t = loop->var.token;
	if ( t == TWHILE ) {
		findbraket(TWHILE, TWEND);
		nexttoken();
	}
	else if ( t == TREPEAT ) {
		findbraket(TREPEAT, TUNTIL);
		while (!termsymbol()) nexttoken();
	}
	else { /* a FOR loop is the default */
		findbraket(TFOR, TNEXT);
		nexttoken();	
		if (token == VARIABLE) nexttoken(); /* we are at next and skip the variable check */
	}
	droploop();
}
#else
void xbreak(){
	droploop();
	if (!USELONGJUMP && er) return;
	findbraket(TFOR, TNEXT);
	nexttoken();	
	if (token == VARIABLE) nexttoken(); /* we are at next and skip the variable check */
}
#endif

/*
 * CONT as a loop control statement, as apocryphal as BREAK, simply 
 * advance to next and the continue to process
 */
#ifdef HASSTRUCT
static void xcont() {
	bloop_t* loop;

	loop = activeloop();
	if (!USELONGJUMP && er) return;
	register token_t t = loop->var.token;
	if ( t == TWHILE )	findbraket(TWHILE, TWEND);
	else if ( t == TREPEAT ) findbraket(TREPEAT, TUNTIL);
	else /* a FOR loop is the default */
		findbraket(TFOR, TNEXT);
}
#else
void xcont() {
	findbraket(TFOR, TNEXT);
}
#endif

/* 
 * NEXT variable statement.
 * 
 * This code uses the global name variable right now for processing of 
 * the variable in FOR. The variable name in next is stored in a local variable.
 */

/* reimplementation of xnext without change of the stack in a running loop */

static void xnext(){
	name_t variable; /* this is a potential variable argument of next */	
	number_t value; 
	bloop_t* loop;

/* check is we have the variable argument */
	nexttoken();

/* one variable is accepted as an argument, no list */
	if (token == VARIABLE) {
		if (DEBUG) { outsc("** variable argument "); outname(&name); outcr(); }
		variable=name;
		nexttoken();
		if (!termsymbol()) {
			error(EUNKNOWN);
			return;
		}
	} else {
		variable.c[0]=0;
	}

/* see whats going on */
	loop=activeloop();
	if (!USELONGJUMP && er) return;

/* check if this is really a FOR loop */
#ifdef HASSTRUCT
	if (loop->var.token == TWHILE || loop->var.token == TREPEAT) { error(ELOOP); return; }
#endif

/* a variable argument in next clears the for stack 
		down as BASIC programs can and do jump out to an outer next */
	if (variable.c[0] != 0) {
		while(!cmpname(&variable, &loop->var)) {
			droploop();
			if (!USELONGJUMP && er) return;
			loop=activeloop();
			if (!USELONGJUMP && er) return;
		}
	}

/* step=0 an infinite loop */
	value=getvar(&loop->var)+loop->step;
	setvar(&loop->var, value);

	if (DEBUG) { 
		outsc("** next loop variable "); outname(&loop->var); outspc(); 
		outsc(" value "); outnumber(value); outcr(); 
	}

/* do we need another iteration, STEP 0 always triggers an infinite loop */
	if ((loop->step == 0) || (loop->step > 0 && value <= loop->to) || (loop->step < 0 && value >= loop->to)) {
/* iterate in the loop */
		here=loop->here;
/* in interactive mode, jump to the right buffer location */
		if (st == SINT) bi=ibuffer+here;
	} else {
/* last iteration completed we stay here after the next, 
	no precaution for SINT needed as bi unchanged */
		droploop();
	}
	nexttoken();
	if (DEBUG) { outsc("** after next found token "); debugtoken(); }
}

/* 
 *	TOKEN output - this is also used in save.
 * list does a minimal formatting with a simple heuristic.
 * 
 */
static void outputtoken() {
	address_t i;

	if (token == EOL) return;

	if (token == LINENUMBER) outliteral=0;

	if (token == TREM) outliteral=1;

	if (spaceafterkeyword) {
		if (token != '(' && 
			token != LINENUMBER && 
			token !=':' && 
			token !='$') outspc();
		spaceafterkeyword=0;
	}

	if ( token == NUMBER ) outnumber(x);
	else if ( token == LINENUMBER ) {
		outnumber(ax);
		outspc();
	}
	else if ( token == ARRAYVAR || token == STRINGVAR || token == VARIABLE ) {
		if (lastouttoken == NUMBER) outspc(); 
		outname(&name);
		if (token == STRINGVAR) outch('$');
	}
	else if ( token == STRING) {
		outch('"'); 
#ifdef USEMEMINTERFACE
		if (!sr.ir) getstringtobuffer(&sr, spistrbuf1, SPIRAMSBSIZE);
#endif
		outs(sr.ir, sr.length);
		outch('"');
	}
	else {
		if ( (token < 32 && token >= BASEKEYWORD) || token < -127) {	
			if ((token == TTHEN || 
				token == TELSE ||
				token == TTO || 
				token == TSTEP || 
				token == TGOTO || 
				token == TGOSUB ||
				token == TOR ||
				token == TAND) && lastouttoken != LINENUMBER) outspc();
			else 
				if (lastouttoken == NUMBER || lastouttoken == VARIABLE) outspc(); 	

			for(i=0; gettokenvalue(i)!=0 && gettokenvalue(i)!=token; i++);
			outsc(getkeyword(i)); 
			if (token != GREATEREQUAL && 
				token != NOTEQUAL && 
				token != LESSEREQUAL && 
				token != TREM &&
				token != TFN) spaceafterkeyword=1;
			goto ex;
		}	
		if (token >= 32) {
			outch(token);
			if (token == ':' && !outliteral) outspc();
			goto ex;
		} 
		outch(token); outspc(); outnumber(token);
	}
ex:
	lastouttoken=token;
}

/*
 * LIST programs to an output device.
 * 
 * The output is formatted to fit the screen, the heuristic is simple.
 *
 */

static void listlines(address_t b, address_t e) {
	mem_t oflag=0;

/* global variables controlling outputtoken, reset to default */
	lastouttoken=0;
	spaceafterkeyword=0;

/* if there is a programm ... */
	if ( top != 0 ) {
		here=0;
		gettoken();
		#if TRACE_PRINTF
		printf("[listlines] %d\n", token);
		#endif
		while (here < top) {
			if (token == LINENUMBER && ax >= b) oflag=1;
			if (token == LINENUMBER && ax >  e) oflag=0;
			if (oflag) outputtoken();
			gettoken();
			#if TRACE_PRINTF
			printf("[listlines2] %d\n", token);
			#endif
			if (token == LINENUMBER && oflag) {
				outcr();
/* wait after every line on small displays
   removed if ( dspactive() && (dsp_rows < 10) ){ if ( inch() == 27 ) break;} */
				if (dspactive() && dspwaitonscroll() == 27) break;
			}
		}
		if (here == top && oflag) outputtoken();
    	if (e == maxaddr || b != e) outcr(); /* supress newlines in "list 50" - a little hack */
	}

}

static void xlist() {
	address_t b, e;

/* get the argument */
	nexttoken();
 	parsearguments();
	if (!USELONGJUMP && er) return;

/* parse the arguments */
	if ( args == 0 ) {
		b=0;
		e=maxaddr;
	}
	else if ( args == 1 ) {
		b=pop();
		e=b;
	}
	else if ( args == 2 ) {
		e=pop();
		b=pop();
	}
	else {
		error(EARGS);
		return;
	}

/* list the line from b to e in the default output device */
	listlines(b, e);

/* we are done */
	nexttoken();
 }

/* 
 * The progam line editor, first version. The code is 
 * not save for BUFSIZE greater than 127. A cast to 
 * unsigned char aka uint8_t is needed for the string 
 * length as some platforms have a signed char and some 
 * don't.
 */
static void xedit(){
	mem_t ood = od;
	address_t line;
	address_t l;
	int i, k, j;
	char ch;

/* we edit only in interactive mode */
	if (st != SINT) { error(EUNKNOWN); return; }

/* currently only one line number */
	if(!expectexpr()) { error(EARGS); return; }

/* this uses the input buffer now */
	line=pop();
undo: /* this is the undo point */
	ibuffer[0]=0;
	ibuffer[1]=0;
	od=0;
	listlines(line, line);
	if (ibuffer[0] == 0) {
		outnumber(line);
		outspc();
	}
	od=ood;

/* set the cursor to the first character */
	l=1;

/* editing loop for blocking and non blocking terminals */
	while (-1) {

/* show the line and indicate the cursor */
		for(i=1; i<=(unsigned char)ibuffer[0]; i++) outch(ibuffer[i]);
		outcr(); 
		for(i=0; i<l-1; i++) outspc();
		outch('^'); 
		outcr();

/* get a bunch of editing commands and process them*/
		i = ins(sbuffer, SBUFSIZE);
		for (k = 1; k <= i; ++k) {
			ch = sbuffer[k];
			if ( ch == 'q' ) { /* quit the editor*/
				goto done;
			}
			if ( ch == 'Q' ) { /* end the editor without saving */
				goto endnosave;
			}
			if ( ch == 'X' ) { /* delete from cursor until the end of the line */
				ibuffer[0] = (char)l;
				ibuffer[l] = 0;
			}
			else if ( ch == 'j' ) { /* vi style left */
				if (l > 1) l--;
			}
			else if ( ch == 'k' ) { /* vi style right */
				if (l < (unsigned char)ibuffer[0]) l++;
			}
			else if ( ch == 'x' ) { /* delete the cursor character */
				if ((unsigned char)ibuffer[0]>0) {
					for (j=l; j<(unsigned char)ibuffer[0]; j++) ibuffer[j]=ibuffer[j+1]; 
					ibuffer[j]=0;
					ibuffer[0]=(unsigned char)ibuffer[0]-1;
				}
				if ((unsigned char)ibuffer[0]<l) l=(unsigned char)ibuffer[0];
			}
			else if ( ch == 's' ) { /* substitute one character at the cursor position */
				if (k < i) {
					k++; 
					ibuffer[l] = sbuffer[k];
				}
			}
			else if ( ch == 'a' ) { /* append multiple characters at the end of the line */
				l = (unsigned char)ibuffer[0] + 1;
				goto insert;
			}
			else if ( ch == 'i' ) { /* insert multiple characters at the cursor position */
insert:
				if (i-k+(unsigned char)ibuffer[0] < BUFSIZ) {
					for (j=i-k+(unsigned char)ibuffer[0]; j>=l; j--) {
						ibuffer[j+i-k]=ibuffer[j];
						if (j<=l+i-k) ibuffer[j]=sbuffer[k+1+(j-l)];
					}
				}
				ibuffer[0]=(unsigned char)ibuffer[0]+i-k;
				k=i;
			}
			else if ( ch == '^' ) { /* vi style start of line */
				l = 1;
			}
			else if ( ch == '$' ) { /* vi style end of line */
				l = (unsigned char)ibuffer[0] + 1;
			}
			else if ( ch == 'h' ) { /* vi style backspace */
				if (l > 1) {
					for (j=l-1; j<(unsigned char)ibuffer[0]; j++) ibuffer[j]=ibuffer[j+1]; 
					ibuffer[j]=0;
					ibuffer[0]=(unsigned char)ibuffer[0]-1;
					l--;
				}
			}
			else if ( ch == 'u' ) { /* vi style undo */
				goto undo;
			}
			else if ( ch == ':' ) {  /* find the next colon : character*/
				if (l <= (unsigned char)ibuffer[0]) {
					while (l <= (unsigned char)ibuffer[0] && ibuffer[l] != ':') l++;
					if (l <= (unsigned char)ibuffer[0]) l++;
				}
			}
			/* do nothing if the character is not recogized */
		}
	}

/* try to store the line, may heaven help us */
done:
	bi=ibuffer;
	st=SINT;
	nexttoken();
	if (token == NUMBER) {
		ax=x;
		storeline();
	}

/* and we are done, restore the output device and clean the buffer  */
endnosave:
	ibuffer[0]=0;
	ibuffer[1]=0;
	bi=ibuffer+1;
	nexttoken();
}

/*
 *	RUN and CONTINUE are the same function
 */
static void xrun(){
	if (token == TCONT) {
		st = SRUN;
		nexttoken();
	} else {
		nexttoken();
		parsearguments();
		if (er != 0 ) return;
		if (args > 1) {
			error(EARGS); return;
		}
		if (args == 0) {
			here = 0;
		} else {
			#if TRACE_PRINTF
			printf("[xrun]\n");
			#endif
			findline(pop());
		}
		if (!USELONGJUMP && er) return;
		if (st == SINT) st=SRUN;

/* all reset on run */
		clrvars();
		clrgosubstack();
		clrforstack();
		clrdata();
		clrlinecache();
		ert=0;
		ioer=0;
		fncontext=0;
#ifdef HASEVENTS
		resettimer(&every_timer);
		resettimer(&after_timer);
    	events_enabled=1;
#endif

		nexttoken();
	}

/* once statement is called it stays into a loop until the token stream 
		is exhausted. Then we return to interactive mode. */
	statement();
	st = SINT;
/* flush the EEPROM when changing to interactive mode */
	eflush();

/* if called from command line with file arg - exit after run */
#ifdef HASARGS
    if (bnointafterrun) restartsystem();
#endif
}

/*
 * NEW the general cleanup function - new deletes everything
 * 
 * restbasicstate() is a helper, it keeps the memory intact
 * 	this is needed for EEPROM direct memory 
 */
static void resetbasicstate() {

 	if (DEBUG) { outsc("** BASIC state reset \n"); }
 
 /* all stacks are purged */
	clearst();
	clrgosubstack();
	clrforstack();
	clrdata();
	clrvars();
	clrlinecache();
  
/* error status reset */
	reseterror();

/* let here point to the beginning of the program */
	here=0;

/* function context back to zero */
	fncontext=0;

/* interactive mode */
	st=SINT;

/* switch off timers and interrupts */
#ifdef HASTIMER
	resettimer(&after_timer);
	resettimer(&every_timer);
#endif
  
}
 
static void xnew(){ 

/* reset the state of the interpreter */
	resetbasicstate();


	if (DEBUG) {
		 outsc("** clearing memory ");
		 outnumber(memsize); 
		 outsc(" bytes \n");
	}

/* program memory back to zero and variable heap cleared */
	himem=memsize;
	zeroblock(0, memsize);
	top=0;

/* on EEPROM systems also clear the stored state and top */
#ifdef EEPROMMEMINTERFACE
	if (DEBUG) outsc("** clearing EEPROM state \n ");
	eupdate(0, 0);
	setaddress(1, beupdate, top);
#endif
}

/* 
 *	REM - skip everything 
 */
void xrem() {
	if (debuglevel == -1) outsc("REM: ");
	while (token != LINENUMBER && token != EOL && here <= top) 
	{
		nexttoken();
		if (debuglevel == -1) {
			if (token != LINENUMBER) outputtoken(); else outcr();
		}
		#if TRACE_PRINTF
		printf("[REM] token: %d\n", token);
		#endif
	} 
}

/* 
 *	The Apple 1 BASIC additions
 * CLR, DIM, POKE, TAB
 */

/* 
 *	CLR - clearing variable space
 */
static void xclr() {

#ifdef HASDARKARTS
	name_t variable;
	nexttoken();
	if (termsymbol()) {
		clrvars();
		clrgosubstack();
		clrforstack();
		clrdata();
		clrlinecache();
		ert = 0;
    	ioer = 0;
	} else {
		variable = name;
		register token_t t = variable.token;
		if ( t == VARIABLE ) {
			if (variable.c[0] == '@') { return; }
		}
		else if ( t == ARRAYVAR ) {
			nexttoken();
			if (token != '(') { error(EVARIABLE); return; }
			nexttoken();
			if (token != ')') { error(EVARIABLE); return; }
		}
		else if ( t == STRINGVAR ) {
			if (variable.c[0] == '@') { error(EVARIABLE); return; }
		}
		else if ( t == TGOSUB ) {
			clrgosubstack();
			goto next;
		}
		else if ( t == TFOR ) {
			clrforstack();
			goto next;
		}
		else if ( t == TEVERY ) {
			resettimer(&every_timer);
			goto next;
		}
		else if ( t == TAFTER ) {
			resettimer(&after_timer);
			goto next;
		} else {
			expression();
			if (!USELONGJUMP && er) return;
			ax = pop();
			variable.c[0] = ax % 256;
			variable.c[1] = ax / 256;
			variable.token = TBUFFER;
		}

/* we have to clear an object, call free */
		ax = bfree(&variable);
		if (ax == 0) { 
			if (variable.token != TBUFFER) { error(EVARIABLE); return; }
			else ert = 1;
		}
	}
#else
	clrvars();
	clrgosubstack();
	clrforstack();
	clrdata();
	clrlinecache();
	ert=0;
	ioer=0;
#endif

next:
	nexttoken();
}

#ifdef HASAPPLE1
/* 
 *	DIM - the dimensioning of arrays and strings from Apple 1 BASIC
 */
static void xdim(){
	name_t variable;
	address_t x;
	address_t y=1; 

/* which object should be dimensioned or created */
	nexttoken();

nextvariable:
	if (token == ARRAYVAR || token == STRINGVAR ){

/* remember the object, direct assignment of struct for the moment */	
		variable=name;

		if (DEBUG)	{
			outsc("** in xdim "); outname(&variable); outspc(); outnumber(variable.token); 
			outspc(); outname(&name); outspc(); outnumber(name.token); outcr();
		}	

/* parse the arguments */
		parsesubscripts();
		if (!USELONGJUMP && er) return;

#ifndef HASMULTIDIM
		if (args != 1) {error(EARGS); return; }
		x=popaddress();
#else 
		if (args != 1 && args != 2) {error(EARGS); return; }
		if (args == 2) y=popaddress();
		x=popaddress();
#endif	

/* we create at least one element */ 
		if (x<1 || y<1) {error(EORANGE); return; }

/* various checks - do we have enough space in buffers and string indices */
		if (variable.token == STRINGVAR) {
			if ((x>255) && (strindexsize==1)) {error(EORANGE); return; }
#ifdef SPIRAMSBSIZE
			if (x>SPIRAMSBSIZE-1) {error(EORANGE); return; }
#endif
/* With the substringmode switched off, if only one argument is given 
	we interpret the argument as the string array dimension and not as 
	the length two arguments are allowed and work as always. This makes
	things more compatible to the Microsoft BASIC world. */
		if (!substringmode)
			if (args == 1) {
				y=x;
				x=defaultstrdim;
			}

			(void) createstring(&variable, x, y);
		} else {
			(void) createarray(&variable, x, y);
		}
		if (!USELONGJUMP && er) return;

	} else if (token == VARIABLE) {
		(void) bmalloc(&name, 0); /* this is a local variable, currently no safety net */
	} else {
		error(EUNKNOWN);
		return;
	}

	nexttoken();
	if (token == ',') {	
		nexttoken();
		goto nextvariable;
	}

	nexttoken();
}


/* 
 *	POKE - low level poke to the basic memory.
 *  on 16bit systems, the address is signed, so we can only go up to 32767.
 *  If the address is negative, we poke into the EEPROM. 
 */
static void xpoke(){
	number_t a, v; 

/* get the address and the value */
	nexttoken();
	parsenarguments(2);
	if (!USELONGJUMP && er) return;

	v=pop(); /* the value */
	a=pop(); /* the address */

/* catch memsize here because memwrite doesn't do it */
	if (a >= 0 && a<=memsize) 
		memwrite2(a, v);
	else if (a < 0 && a >= -elength())
		eupdate(-a-1, v);
	else {
		error(EORANGE);
	}
}

/*
 *	TAB - spaces command of Apple 1 BASIC 
 * 		charcount mechanism for relative tab if HASMSTAB is set
 */
static void xtab(){
	address_t a;
	token_t t = token;

/* get the number of spaces, we allow brackets here to use xtab also in PRINT */
	nexttoken();
	if (token == '(') nexttoken();
	parsenarguments(1);
	if (!USELONGJUMP && er) return;
	if (token == ')') nexttoken();

	a=popaddress();
	if (!USELONGJUMP && er) return; 

/* the runtime environment can do a true tab then ...  */  
#ifdef HASMSTAB
	if (t != TSPC && reltab && od <= OPRT && od > 0) {
		if (charcount[od-1] >= a) a=0; else a=a-charcount[od-1]-1;
	} 
#endif	
	while (a-- > 0) outspc();	
}
#endif

/* 
 * locate the curor on the screen 
 */

static void xlocate() {
	address_t cx, cy; 

	nexttoken();
	parsenarguments(2);
	if (!USELONGJUMP && er) return;

	cy=popaddress();
	cx=popaddress();
	if (!USELONGJUMP && er) return;
#if VT52
/* for locate we go through the VT52 interface for cursor positioning*/
	if (cx > 0 && cy > 0 && cx < 224 && cy < 224) {
		outch(27); outch('Y');
		outch(31+(unsigned int) cy); 
		outch(31+(unsigned int) cx);
	}
#else // M-OS
	int w = get_console_width();
	int h = get_console_height();
	int x = cx = (w >> 1);
	int y = cy = (h >> 1);
	if (x >= 0 && y >= 0 && x < w && y < h) {
		graphics_set_con_pos(x, y);
	}
#endif
/* set the charcount, this is half broken on escape sequences */
#ifdef HASMSTAB
	if (od > 0 && od <= OPRT) charcount[od-1]=cx;
#endif

}

/* 
 *	Stefan's additions to Palo Alto BASIC
 * DUMP, SAVE, LOAD, GET, PUT, SET
 *
 */

/*
 *	DUMP - memory dump program
 */
static void xdump() {
	address_t a, x;
	char eflag = 0;

	nexttoken();
	if (token == '!') { eflag=1; nexttoken(); }
	parsearguments();
	if (!USELONGJUMP && er) return;

	if ( args == 0 ) {
		x = 0;
		a = memsize;
	}
	else if ( args == 1 ) {
		x = pop();
		a = memsize;
	}
	else if ( args == 2 ) {
		a = pop();
		x = pop();
	}
	else {
		error(EARGS);
		return;
	}
	form = 6;
	if (a > x) dumpmem((a - x) / 8 + 1, x, eflag);
	form = 0;
}

/*
 * helper of DUMP, wrote the memory out
 */
static void dumpmem(address_t r, address_t b, char eflag) {
	address_t j, i;	
	address_t k;
	mem_t c;
	address_t end;

	k=b;
	i=r;
	if (eflag) end=elength(); else end=memsize;
	while (i>0) {
		outnumber(k); outspc();
		for (j=0; j<8; j++) {
			if (eflag) c=eread(k); else c=memread(k);
			k++;
			outnumber(c); outspc();
			if (k > end) break;
		}
		outcr();
		i--;
		if (k > end) break;
	}
	if (eflag) {
		outsc("elength: "); outnumber(elength()); outcr();
	} else {
		outsc("top: "); outnumber(top); outcr();
		outsc("himem: "); outnumber(himem); outcr();
	}
}

/*
 *	creates a C string from a BASIC string
 *	after reading a BASIC string 
 */
static void stringtobuffer(char *buffer, bstring_t* s) {
	index_t i = s->length;

	if (i >= SBUFSIZE) i=SBUFSIZE-1;
	buffer[i--]=0;
	while (i >= 0) { buffer[i]=s->ir[i]; i--; }
}

#ifdef USEMEMINTERFACE
/* helper for the memintercase code */
static void getstringtobuffer(bstring_t* strp, char *buffer, stringlength_t maxlen) {
	stringlength_t i;

	for (i=0; i<strp->length && i<maxlen; i++) buffer[i]=memread2(strp->address+i);
	strp->ir=buffer;
}
#endif

/* get a file argument */
static void getfilename(char *buffer, char d) {
	index_t s;
	char *sbuffer;
	bstring_t sr;

/* do we have a string argument? */
	s=stringvalue(&sr);
	if (!USELONGJUMP && er) return;
	if (DEBUG) {outsc("** in getfilename2 stringvalue delivered "); outnumber(s); outcr(); }

	if (s) {
		if (DEBUG) {outsc("** in getfilename2 copying string of length "); outnumber(x); outcr(); }
#ifdef USEMEMINTERFACE
		if (!sr.ir) getstringtobuffer(&sr, spistrbuf1, SPIRAMSBSIZE);
#endif
		stringtobuffer(buffer, &sr);
		if (DEBUG) {outsc("** in getfilename2 stringvalue string "); outsc(buffer); outcr(); }
		nexttoken();
	} else if (termsymbol()) {
		if (d) {
			sbuffer=getmessage(MFILE);
			s=0;
			while ((sbuffer[s] != 0) && (s < SBUFSIZE-1)) { buffer[s]=sbuffer[s]; s++; }
			buffer[s]=0;
			x=s;
		} else {
			buffer[0]=0;
			x=0;
		}
		nexttoken();
	} else {
		expression();
		if (!USELONGJUMP && er) return;
		buffer[0]=pop();
		buffer[1]=0;
	}
}

/* 
 * an alternative getfilename implementation that simply gives back a buffer 
 * with the filename in it. We avoid a string buffer in the calling commands 
 * like SAVE and LOAD. 
 */
static char* getfilename2(char d) {
	mem_t s;
	bstring_t sr;

/* we have no argument and use the default */
	if (termsymbol()) {
		if (d) return getmessage(MFILE);
		else return 0;
	}

/* we have a string argument or an expression */
	s=stringvalue(&sr);
	if (!USELONGJUMP && er) return 0;
	if (s) {
#ifdef USEMEMINTERFACE
		if (!sr.ir) getstringtobuffer(&sr, sbuffer, SBUFSIZE);	
#endif
		nexttoken(); /* undo the rewind of stringvalue, only then use the buffer */
		for (s=0; s<sr.length && s<SBUFSIZE-1; s++) sbuffer[s]=sr.ir[s];
		sbuffer[s]=0;
		return sbuffer;
	} else {
		expression();
		if (!USELONGJUMP && er) return 0;
		sbuffer[0]=pop();
		sbuffer[1]=0;
		return sbuffer;
	}
}

#if defined(FILESYSTEMDRIVER)
/*
 *	SAVE a file either to disk or to EEPROM
 */
static void xsave() {
	// char filename[SBUFSIZE];
	char *filename;
	address_t here2;
	token_t t;

	nexttoken();
	//getfilename(filename, 1);
	filename=getfilename2(1);
	if (!USELONGJUMP && er) return;
	t=token;

	if (filename[0] == '!') {
		esave();
	} else {		
		if (DEBUG) { outsc("** Opening the file "); outsc(filename); outcr(); };
	 	
		if (!ofileopen(filename, "w")) {
			error(EFILE);
			return;
		} 

/* save the output mode and then save */
		push(od);
		od=OFILE;
		
/* the core save - xlist() not used any more */
		ert=0; /* reset error to trap file problems */
		here2=here;
		here=0;
		gettoken();
		#if TRACE_PRINTF
		printf("[xsave] %d\n", token);
		#endif
		while (here < top) {
			outputtoken();
			if (ert) break;
			gettoken();
			#if TRACE_PRINTF
			printf("[xsave2] %d\n", token);
			#endif
			if (token == LINENUMBER) outcr();
		}
		if (here == top) outputtoken();
   		outcr(); 
   	
/* back to where we were */
   		here=here2;

/* restore the output mode */
		od=pop();

/* clean up */
		ofileclose();

/* did an accident happen */
    if (ert) { printmessage(EGENERAL); outcr();  ert=0; }
	}

/* and continue remembering, where we were */
	token=t;
}

/*
 * LOAD a file, LOAD can either be invoked with a filename argument
 * or without, in the latter case the filename is read from the token stream
 * with getfilename.
 */
static void xload(const char* f) {
	// char filename[SBUFSIZE];
	char* filename;
	char ch;
	address_t here2;
	mem_t chain = 0;

	if (f == 0) {
		nexttoken();
		filename=getfilename2(1);
		if (!USELONGJUMP && er) return;
	} else {
		filename=(char*)f;
	}

	if (filename[0] == '!') {
		eload();
	} else {

/*	if load is called during runtime it chains
 *	load the program as new but perserve the variables
 *	gosub and for stacks are cleared 
 */
		if (st == SRUN) { 
			chain=1; 
			st=SINT; 
			top=0;
			clrgosubstack();
			clrforstack();
			clrdata();
		}

		if (!f)
			if (!ifileopen(filename)) { error(EFILE); return; } 

		bi=ibuffer+1;
		while (fileavailable()) {
			ch = fileread();
			if (ch == '\n' || ch == '\r' || cheof(ch)) {
				*bi = 0;
				bi = ibuffer + 1;
				#if TRACE_PRINTF
				printf("[line] %s\n", bi);
				#endif
				if (*bi != '#') { /* lines starting with a # are skipped - Unix style shell startup */
					nexttoken();
					if (token == NUMBER) {
						ax = x;
						storeline();
					}
        			if (er != 0 ) break;
        			bi = ibuffer+1;
        		}
      		} else {
        		*bi++ = ch;
      		}

			if ((bi-ibuffer) > BUFSIZE) {
        		error(EOUTOFMEMORY);
        		break;
      		}
		}   	
		ifileclose();
/* after a successful load we save top to the EEPROM header */
#ifdef EEPROMMEMINTERFACE
		setaddress(1, beupdate, top);
#endif

/* go back to run mode and start from the first line */
		if (chain) {
			st=SRUN;
			here=0;
			nexttoken();
		}
	}
}
#else 
/*
 *	SAVE a file to EEPROM - minimal version for small Arduinos
 */
void xsave() {
	esave(); 
	nexttoken();
}
/*
 *	LOAD a file from EEPROM - minimal version for small Arduinos
 */
void xload(const char* f) {
	eload();
	nexttoken();
}
#endif

/*
 *	GET just one character from input 
 */
static void xget(){

/* identifier of the lefthandside */
	lhsobject_t lhs;

	mem_t oid=id;   /* remember the input stream */
	char ch;

	nexttoken();

/* modifiers of the get statement */
	if (token == '&') {

		if (!expectexpr()) return;
		id=pop();		
		if (token != ',') {
			error(EUNKNOWN);
			return;
		}
		nexttoken();
	}

/* this code evaluates the left hand side - remember type and name */
	lhs.name=name; 

	lefthandside(&lhs);
	if (!USELONGJUMP && er) return;

/* get the data, non blocking on Arduino */
	if (availch()) ch=inch(); else ch=0;

/* store the data element as a number expect for */
	assignnumber2(&lhs, ch);

/* but then, strings where we deliver a string with length 0 if there is no data */
#ifdef HASAPPLE1
	if (lhs.name.token == STRINGVAR && ch == 0 && lhs.ps) setstringlength(&lhs.name, 0, arraylimit);
#endif

/* restore the output device */
	id=oid;
}

/*
 *	PUT writes one character to an output stream
 */
static void xput(){
	mem_t ood=od;
	index_t i;

	nexttoken();

/* modifiers of the put statement */
	if (token == '&') {

		if(!expectexpr()) return;
		od=pop();
		if (token != ',') {
			error(EUNKNOWN);
			return;
		}
		nexttoken();
	}

	parsearguments();
	if (!USELONGJUMP && er) return;

	for (i=args-1; i>=0; i--) sbuffer[i]=pop();
	outs(sbuffer, args);

	od=ood;
}

/* setpersonality is a helper of xset */
static void setpersonality(index_t p) {
#ifdef HASAPPLE1
/* a Microsoft like BASIC have arrays starting at 0 with n+1 elements and no substrings, MS type RND */
	if ( p == 'm' || p == 'M' ) {
		msarraylimits = 1;
		arraylimit = 0;
		substringmode = 0;
		booleanmode = -1;
		randombase = -1;
		reltab = 1;
		return;
	}
/* an Apple 1 like BASIC have arrays starting at 1 with n elements and substrings */
	if ( p == 'a' || p == 'A' ) {
		msarraylimits = 0;
		arraylimit = 1;
		substringmode = 1;
		booleanmode = 1;
		randombase = 0;
		reltab = 0;
		return;
	}
/* PaloAlto BASIC is an integer basic with slightly different behaviour */
	if ( p == 'p' || p == 'P' ) {
		msarraylimits = 0;
		arraylimit = 0;
		substringmode = 1;
		booleanmode = 1;
		forceint = 1;
		randombase = 1;
		reltab = 0;
	}
#endif
}

/* 
 *	SET - the command itself is also apocryphal it is a low level
 *	control command setting certain properties
 *	syntax, currently it is only SET expression, expression
 */
static void xset(){
	address_t function;
	index_t argument;
	nexttoken();
	parsenarguments(2);
	if ( !USELONGJUMP && er ) return;
	argument = pop();
	function = pop();
/* runtime debug level */
	if ( function == 0 ) {
		debuglevel=argument;
	}
/* autorun/run flag of the EEPROM 255 for clear, 0 for prog, 1 for autorun */
	else if ( function == 1 ) eupdate(0, argument);
/* change the output device */			
	else if ( function == 2 ) {
		if ( argument == 0 ) od = OSERIAL;
		else if ( argument == 1 ) od = ODSP;
	}
/* change the default output device */			
	else if ( function == 3 ) {
		if ( argument == 0 ) odd = (od = OSERIAL);
		else if ( argument == 1 ) odd = (od = ODSP);
	}
 /* change the input device */			
	else if ( function == 4 ) {
		if ( argument == 0 ) id = ISERIAL;
		else if ( argument == 1 ) id = IKEYBOARD;
	}
/* change the default input device */					
	else if ( function == 5 ) {
		if ( argument == 0 ) idd = (id = ISERIAL);
		else if ( argument == 1 ) idd = (id = IKEYBOARD);
	}
#ifdef HASSERIAL1
/* set the cr behaviour */
	else if ( function == 6 ) sendcr=(char)argument;
/* set the blockmode behaviour */
	else if ( function == 7 ) blockmode=argument;
/* set the second serial ports baudrate */
	else if ( function == 8 ) prtset(argument);
#endif
/* set the power amplifier level of the radio module */
#ifdef HASRF24
	else if ( function == 9 ) radioset(argument);
#endif		
/* display update control for paged displays */
#ifdef DISPLAYDRIVER
	else if ( function == 10 ) dspsetupdatemode(argument);
#endif
/* change the output device to a true TAB */
#ifdef HASMSTAB
	else if ( function == 11 ) reltab = argument;
#endif
/* change the lower array limit */
#ifdef HASAPPLE1
	else if ( function == 12 ) {
		if ( argument >= 0) arraylimit = argument;
		else error(EORANGE); 
	}
#endif
/* the keyboard repeat frequency */
#ifdef HASKEYPAD
	else if ( function == 13 ) kbdrepeat = argument;
#endif
/* the units the pulse command is using */
#ifdef HASPULSE
	else if ( function == 14 ) bpulseunit = argument;
#endif
/* switch on the vt52 emulation an a POSIX system with an ANSI terminal */
#ifdef POSIXVT52TOANSI 
	else if ( function == 15 ) vt52active = argument;
#endif
/* change the default size of a string at autocreate */
#ifdef HASAPPLE1
	else if ( function == 16 ) {
		if (argument > 0) defaultstrdim = argument;
		else error(EORANGE);
	}
#endif
/* set the boolean mode */
	else if ( function == 17 ) {
		if (argument == -1 || argument == 1) booleanmode = argument;
		else error(EORANGE);
	}
/* set the integer mode */
	else if ( function == 18 ) forceint = (argument != 0);
/* set the random number behaviour */
	else if ( function == 19 ) randombase = argument;
/* the substring mode on and off */
#ifdef HASAPPLE1
	else if ( function == 20) substringmode = (argument != 0);
#endif
/* the MS array behaviour, creates n+1 elements when on */
#ifdef HASAPPLE1
	else if ( function == 21 ) msarraylimits = (argument != 0);
#endif
/* set many settings at once to change the entire personality of the interpreter */
	else if ( function == 22 ) setpersonality(argument);
#ifdef HASAPPLE1
	else if ( function == 23 ) lowercasenames = (argument != 0);
#endif		
}

/*
 *	NETSTAT - network status command, rudimentary
 */
static void xnetstat(){
#if defined(HASMQTT)

	nexttoken();
 	parsearguments();
	if (!USELONGJUMP && er) return;
	
	switch (args) {
	case 0: 
		if (netconnected()) outsc("Network connected \n"); else outsc("Network not connected \n");
		outsc("MQTT state "); outnumber(mqttstate()); outcr();
 		outsc("MQTT out topic "); outsc(mqtt_otopic); outcr();
		outsc("MQTT inp topic "); outsc(mqtt_itopic); outcr();
		outsc("MQTT name "); outsc(mqttname); outcr();
		break;
	case 1: 
		ax=pop();
		switch (ax) {
		case 0: 
			netstop();
			break;
		case 1:
			netbegin();
			break;
		case 2:
			if (!mqttreconnect()) ert=1;
			break;
		default:
			error(EARGS);
			return;
		}
		break;
	default:
		error(EARGS);
		return;
	}
#endif	
	nexttoken();		
}

/*
 * The arduino io functions.
 *
 */ 

/* 
 *  rtaread and rtdread are wrappers coming from runtime 
 *  This is done for portability for raspberry pi and other systems
 */

static void xaread(){ 
	push(aread(popaddress())); 
}

static void xdread(){ 
	push(dread(popaddress())); 
}

 /*
  *	DWRITE - digital write 
  */
static void xdwrite(){
 	address_t x,y;

	nexttoken();
	parsenarguments(2);
	if (!USELONGJUMP && er) return; 
	x=popaddress();
	y=popaddress();
	if (!USELONGJUMP && er) return;
	dwrite(y, x);	
}

/*
 * AWRITE - analog write 
 */
static void xawrite(){
	address_t x,y;

	nexttoken();
	parsenarguments(2);
	if (!USELONGJUMP && er) return; 
	x=popaddress();
	if (x > 255) error(EORANGE); 
	y=popaddress();
	if (!USELONGJUMP && er) return;
	awrite(y, x);
}

/*
 * PINM - pin mode
 */
static void xpinm(){
	address_t x,y;

	nexttoken();
	parsenarguments(2);
	if (!USELONGJUMP && er) return; 
	x=popaddress();
	if (x > 1) error(EORANGE);
	y=popaddress();
	if (!USELONGJUMP && er) return;
	pinm(y, x);	
}

/*
 * DELAY in milliseconds
 *
 * this must call bdelay() and not delay() as bdelay()
 * handles all the yielding and timing functions 
 * 
 */
static void xdelay(){
	nexttoken();
	parsenarguments(1);
	if (!USELONGJUMP && er) return;
	bdelay(pop());
}

/* tone if the platform has it -> BASIC command PLAY */
#ifdef HASTONE
/* play a tone */
static void xtone(){
	address_t d = 0;
	address_t v = 100;
	address_t f, p;

/* get minimum of 2 and maximum of 4 args */
	nexttoken();
	parsearguments();
	if (!USELONGJUMP && er) return;
	if (args > 4 || args < 2) { error(EARGS); return; }

/* a switch would be more elegant but needs more progspace ;-) */
	if (args == 4) v=popaddress(); 
	if (args >= 3) d=popaddress();
	f=popaddress();
	p=popaddress();
	if (!USELONGJUMP && er) return; 

	playtone(p, f, d, v);
}
#endif

/* pulse output - pin, duration, [value], [repetitions, delay] */
#ifdef HASPULSE
static void xpulse(){
	address_t pin, duration;
	address_t val = 1;
	address_t interval = 0;
	address_t repetition = 1;

/* do we have at least 2 and not more than 5 arguments */
	nexttoken();
	parsearguments();
	if (!USELONGJUMP && er) return;
	if (args>5 || args<2) { error(EARGS); return; }

/* get the data from stack */
	if (args == 5) { interval=popaddress(); repetition=popaddress(); }
	if (args == 4) { error(EARGS); return; }
	if (args > 2) val=popaddress();
	duration=popaddress();
	pin=popaddress();
	if (!USELONGJUMP && er) return;
  
/* low level run time function for the pulse */ 
	pulseout(bpulseunit, pin, duration, val, repetition, interval);	
}

/* read a pulse, units given by bpulseunit - default 10 microseconds */
static void bpulsein() { 
	address_t x,y;
	unsigned long t, pt;
  
	t=((unsigned long) popaddress())*1000;
	y=popaddress(); 
	x=popaddress();
	if (!USELONGJUMP && er) return;

	push(pulsein(x, y, t)/bpulseunit); 
}
#endif


#ifdef HASGRAPH
/*
 *	COLOR setting, accepting one or 3 arguments
 */
static void xcolor() {
	int r, g, b;
	nexttoken();
	parsearguments();
	if (!USELONGJUMP && er) return;
	if (args == 1) vgacolor(pop());
	else if (args == 3) {
		b=pop();
		g=pop();
		r=pop();
		rgbcolor(r, g, b);
	}
	else error(EARGS);
}

/*
 * PLOT a pixel on the screen
 */
static void xplot() {
	int x0, y0;

	nexttoken();
	parsenarguments(2);
	if (!USELONGJUMP && er) return; 
	y0=pop();
	x0=pop();
	plot(x0, y0);
}

/*
 * LINE draws a line 
 */
void xline() {
	int x0, y0, x1, y1;

	nexttoken();
	parsenarguments(4);
	if (!USELONGJUMP && er) return; 
	y1=pop(); 
	x1=pop();
	y0=pop();
	x0=pop();
	line(x0, y0, x1, y1);
}

void xrect() {
	int x0, y0, x1, y1;

	nexttoken();
	parsenarguments(4);
	if (!USELONGJUMP && er) return; 
	y1=pop(); 
	x1=pop();
	y0=pop();
	x0=pop();
	rect(x0, y0, x1, y1);
}

static void xcircle() {
	int x0, y0, r;

	nexttoken();
	parsenarguments(3);
	if (!USELONGJUMP && er) return;  
	r=pop();
	y0=pop();
	x0=pop();
	circle(x0, y0, r);
}

void xfrect() {
	int x0, y0, x1, y1;

	nexttoken();
	parsenarguments(4);
	if (!USELONGJUMP && er) return; 
	y1=pop(); 
	x1=pop();
	y0=pop();
	x0=pop();
	frect(x0, y0, x1, y1);
}

static void xfcircle() {
	int x0, y0, r;

	nexttoken();
	parsenarguments(3);
	if (!USELONGJUMP && er) return;  
	r=pop();
	y0=pop();
	x0=pop();
	fcircle(x0, y0, r);
}
#endif

#ifdef HASDARKARTS
/*
 * MALLOC allocates a chunk of memory 
 */
static void xmalloc() {
	address_t s;
 	address_t a;
	name_t name;

/* size and identifier */
	s=popaddress();
	a=popaddress();
	if (!USELONGJUMP && er) return;

/* create a name */
	name.token=TBUFFER;
	name.l=2;
	name.c[0]=a%256;
	name.c[1]=a/256;

/* allocate the memory */
	push(bmalloc(&name, s));
}

/*
 * FIND an object on the heap
 * xfind can find things in the variable name space and the buffer space
 */

static void xfind() {
	address_t a;
	address_t n;

/* is there a ( */
	if (!expect('(', EUNKNOWN)) return;

/* after that, try to find the object on the heap */
	nexttoken();
	if (token == TFN) {
		nexttoken();
		name.token=TFN;
	}
	a=bfind(&name);

/* depending on the object, interpret the result */
	if ( token == ARRAYVAR || token == TFN) {
		if (!expect('(', EUNKNOWN)) return;
		if (!expect(')', EUNKNOWN)) return;
		goto intern;
	}
	if (token == VARIABLE || token == STRINGVAR ) {
intern:
		nexttoken();
		goto cont;
	}
	expression(); /* do not use expectexpr here because of the token sequence */
	if (!USELONGJUMP && er) return;
	n=popaddress();
  	if (!USELONGJUMP && er) return;
	name.token=TBUFFER;
	name.l=2;
	name.c[0]=n%256;
	name.c[1]=n/256;
	a=bfind(&name);
cont:
/* closing braket, dont use expect here because of the token sequence */ 
	if (token != ')') { error(EUNKNOWN); return; }

	push(a);
}

/*
 *	EVAL can modify a program, there are serious side effects
 *	which are not caught (and cannot be). All FOR loops and RETURN 
 *	vectors break if EVAL inserts in their range
 */
static void xeval(){
	address_t i, l;
	address_t mline, line;
	bstring_t s;

/* get the line number to store */
	if (!expectexpr()) return;
	line=popaddress();
	if (!USELONGJUMP && er) return;

	if (token != ',') { error(EUNKNOWN); return; }

/* the line to be stored */
	nexttoken();
	if (!stringvalue(&s)) { error(EARGS); return; }

/* here we have the string to evaluate it to the ibuffer
		only one line allowed, BUFSIZE is the limit */
	l=s.length;
	if (!USELONGJUMP && er) return;
	
	if (l>BUFSIZE-1) {error(EORANGE); return; }

#ifdef USEMEMINTERFACE
	if (!s.ir) getstringtobuffer(&s, spistrbuf1, SPIRAMSBSIZE);
#endif

	for (i=0; i<l; i++) ibuffer[i+1]=s.ir[i];
	ibuffer[l+1]=0;
	if (DEBUG) {outsc("** Preparing to store line "); outnumber(line); outspc(); outsc(ibuffer+1); outcr(); }

/* we find the line we are currently at */
	if (st != SINT) {
		mline = myline(here);
		#if TRACE_PRINTF
		printf("[xeval] %d %d\n", token, mline);
		#endif
		if (DEBUG) {outsc("** myline is "); outnumber(mline); outcr(); }
	}

/* go to interactive mode and try to store the line */
	ax=line;							// the linennumber
	push(st); st=SINT;	// go to (fake) interactive mode
	bi=ibuffer;					// go to the beginning of the line
	storeline();				// try to store it
	st=pop();						// go back to run mode

/* find my line - side effects not checked here */
	if (st != SINT) {
		#if TRACE_PRINTF
		printf("[xeval]\n");
		#endif
		findline(mline);
		nextline();
	}
}
#endif


#ifdef HASIOT

/* 
 * AVAIL of a stream - are there characters in the stream
 */
static void xavail() {
	mem_t oid=id;

	id=popaddress();
	if (!USELONGJUMP && er) return;
	push(availch());
	id=oid;
}

/* 
 * IoT functions - sensor reader, experimentral
 */
static void xfsensor() {
	address_t s, a;

	a=popaddress();
	if (!USELONGJUMP && er) return; 
	s=popaddress();
	if (!USELONGJUMP && er) return; 
	push(sensorread(s, a));
}


/*
 * Going to sleep for battery saving - implemented for ESP8266 and ESP32 
 * in hardware-*.h
 */

static void xsleep() {
	nexttoken();
	parsenarguments(1);
	if (!USELONGJUMP && er) return; 
	activatesleep(pop());
}

/* 
 * single byte wire access - keep it simple 
 */

static void xwire() {
	short port, data1, data2;

	nexttoken();
#if defined(HASWIRE) || defined(HASSIMPLEWIRE)
	parsearguments();
	if (!USELONGJUMP && er) return; 

	if (args == 3) {
		data2=pop();
		data1=pop();
		port=pop();	
		wirewriteword(port, data1, data2);
	} else if (args == 2) {
		data1=pop();
		port=pop();	
		wirewritebyte(port, data1);
	} else {
		error(EARGS);
		return;
	}
#endif
}

static void xfwire() {
#if defined(HASWIRE) || defined(HASSIMPLEWIRE) 
	push(wirereadbyte(pop()));
#else 
#endif
}

#endif

/*
 * Error handling function.
 */
#ifdef HASERRORHANDLING
static void xerror() {
	berrorh.type = 0;
	erh = 0;
	nexttoken();
	register token_t t = token;
	if ( t == TGOTO ) {
		if (!expectexpr()) return;
		berrorh.type = TGOTO;
		berrorh.linenumber = pop();
	}
	else if ( t == TCONT ) berrorh.type = TCONT;
	else if ( t == TSTOP ) nexttoken();
	else error(EARGS);
}
#endif

/*
 * After and every trigger timing GOSUBS and GOTOS.
 */
#ifdef HASTIMER 
void resettimer(btimer_t* t) {
	t->enabled = 0;
	t->interval = 0;
	t->last = 0;
	t->type = 0;
	t->linenumber = 0;
}

static void xtimer() {
	token_t t;
	btimer_t* timer;
	/* do we deal with every or after */
	if ( token == TEVERY ) timer = &every_timer;
	else timer = &after_timer;
	/* one argument expected, the time intervall */
	if ( !expectexpr() ) return;
	/* after that, a command GOTO or GOSUB with a line number
			more commands thinkable */
	if ( token == TGOSUB || token == TGOTO ) {
		t=token;
		if (!expectexpr()) return;
		timer->last = millis();
		timer->type=t;
		timer->linenumber = pop();
		timer->interval = pop();
		timer->enabled = 1;
	} else {
		if (termsymbol()) {
			x=pop();
			if (x == 0) 
				timer->enabled=0;
			else {
				if (timer->linenumber) {
					timer->enabled = 1;
					timer->interval = x;
					timer->last = millis();	
				} else 
					error(EARGS);
			}
		} else 
			error(EUNKNOWN);
		return;
	}	
}
#endif

#ifdef HASEVENTS
/* 
 * The events API for Arduino with interrupt service routines
 * analogous to the timer API.
 * 
 * we use raw modes here 
 *
 * #define CHANGE 1
 * #define FALLING 2
 * #define RISING 3
 *
 * detach und attach are wrappers around the original Arduino functions.
 * Runtime needs to be compiled with #define ARDUINOINTERRUPTS for this to 
 * work.
 * 
 */

/* interrupts in BASIC fire once and then disable themselves, BASIC reenables them */
static void bintroutine0() {
	eventlist[0].active=1;
	detachinterrupt(eventlist[0].pin); 
}
static void bintroutine1() {
	eventlist[1].active=1;
	detachinterrupt(eventlist[1].pin); 
}
static void bintroutine2() {
	eventlist[2].active=1;
	detachinterrupt(eventlist[2].pin); 
}  
static void bintroutine3() {
	eventlist[3].active=1;
	detachinterrupt(eventlist[3].pin); 
}

static mem_t eventindex(mem_t pin) {
 	mem_t i;

	for(i=0; i<EVENTLISTSIZE; i++ ) if (eventlist[i].pin == pin) return i; 
	return -1;
}

static mem_t enableevent(mem_t pin){
	mem_t inter;
	mem_t i;
/* do we have the data */
	if ((i = eventindex(pin)) < 0) return 0;
/* can we use this pin? */  
	inter = pintointerrupt(eventlist[i].pin);
	if (inter < 0) return 0;
/* attach the interrupt function to this pin */
	if ( i == 0 ) attachinterrupt(inter, bintroutine0, eventlist[i].mode); 
	else if ( i == 1 ) attachinterrupt(inter, bintroutine1, eventlist[i].mode); 
	else if ( i == 2 ) attachinterrupt(inter, bintroutine2, eventlist[i].mode); 
	else if ( i == 3 ) attachinterrupt(inter, bintroutine3, eventlist[i].mode); 
	else return 0;
/* now set it enabled in BASIC */
	eventlist[i].enabled=1; 
	return 1;
}

inline static void disableevent(mem_t pin) {
	detachinterrupt(pin); 
}

/* the event BASIC commands */
static void initevents() {
	mem_t i;

	for(i=0; i<EVENTLISTSIZE; i++) eventlist[i].pin=-1;
}

static void xevent() {
	mem_t pin, mode;
	mem_t type=0;
	address_t line=0;
	
/* in this version two arguments are neded, one is the pin, the second the mode */
	nexttoken();
  
/* debug code, display the event list */
	if (termsymbol()) {
		for (ax=0; ax<EVENTLISTSIZE; ax++) {
			if (eventlist[ax].pin >= 0) {
				outnumber(eventlist[ax].pin); outspc();
				outnumber(eventlist[ax].mode); outspc();
				outnumber(eventlist[ax].type); outspc();
				outnumber(eventlist[ax].linenumber); outspc();
				outcr();
			}  
		}
		outnumber(nevents); outcr();
		nexttoken();
		return;
	}

/* control of events */
	if (token == TSTOP) {
		events_enabled=0;
		nexttoken();
		return; 
	}

	if (token == TCONT) {
		events_enabled=1;
		nexttoken();
		return; 
	}

/* argument parsing */  
	parsearguments();
	if (!USELONGJUMP && er) return;

	if ( args == 2 ) {
		mode = pop();
		if (mode > 3) {
			error(EARGS);
			return;
		}
	}
	else if ( args == 1 ) pin = pop();
	else error(EARGS);

/* followed by termsymbol, GOTO or GOSUB */
	if (token == TGOTO || token == TGOSUB) {
		type = token;

/* which line to go to */
		if (!expectexpr()) return;
		line = pop();
	} 

    
/* all done either set the interrupt up or delete it*/
	if (type) {
		if (!addevent(pin, mode, type, line)) {
			error(EARGS);
			return;
		}	
	} else {
		disableevent(pin);
		deleteevent(pin);
		return;
	}

/* enable the interrupt */
	if (!enableevent(pin)) {
		deleteevent(pin);
		error(EARGS);
		return;
	}
}

/* handling the event list */
static mem_t addevent(mem_t pin, mem_t mode, mem_t type, address_t linenumber) {
	int i;

/* is the event already there */
	for (i=0; i<EVENTLISTSIZE; i++) 
		if (pin == eventlist[i].pin) goto slotfound;

/* if not, look for a free slot */
	if (nevents >= EVENTLISTSIZE) return 0;
	for (i=0; i<EVENTLISTSIZE; i++) 
    	if (eventlist[i].pin == -1) goto slotfound;

/* no free event slot */
	return 0;

/* we have a slot */
slotfound:
	eventlist[i].enabled=0;
	eventlist[i].pin=pin;
	eventlist[i].mode=mode;
	eventlist[i].type=type;
	eventlist[i].linenumber=linenumber;
	eventlist[i].active=0;
	nevents++;
	return 1;
}

void deleteevent(mem_t pin) {
	int i;

/* do we have the event? */
	i=eventindex(pin);

	if (i >= 0) {
		eventlist[i].enabled=0;
		eventlist[i].pin=-1;
		eventlist[i].mode=0;
		eventlist[i].type=0;
		eventlist[i].linenumber=0;
		eventlist[i].active=0;
		nevents--;
	}
}
#endif

/*
 *	BASIC DOS - disk access programs, to control mass storage from BASIC
 */

/* string match helper in catalog */
static char streq(const char *s, char *m){
	short i=0;

	while (m[i]!=0 && s[i]!=0 && i < SBUFSIZE){
		if (s[i] != m[i]) return 0;
		i++;
	}	
	return 1;
}

/*
 *	CATALOG - basic directory function
 */
static void xcatalog() {
#if defined(FILESYSTEMDRIVER) 
	char filename[SBUFSIZE];
	const char *name;

	nexttoken();
	getfilename(filename, 0);
	if (!USELONGJUMP && er) return; 

	rootopen();
	while (rootnextfile()) {
		if (rootisfile()) {
			name=rootfilename();
			if (*name != '_' && *name !='.' && streq(name, filename)){
				outscf(name, 14); outspc();
				if (rootfilesize()>0) outnumber(rootfilesize()); 
				outcr();
				if (dspwaitonscroll() == 27) break;
			}
		}
		rootfileclose();
	}
	rootclose();
#else
	nexttoken();
#endif
}

/*
 *	DELETE a file
 */
static void xdelete() {
#if defined(FILESYSTEMDRIVER)
	char filename[SBUFSIZE];

	nexttoken();
	getfilename(filename, 0);
	if (!USELONGJUMP && er) return; 

	removefile(filename);
#else 
	nexttoken();
#endif
}

/*
 *	OPEN a file or I/O stream - very raw mix of different functions
 */
static void xopen() {
#if defined(FILESYSTEMDRIVER) || defined(HASRF24) || defined(HASMQTT) || defined(HASWIRE) || defined(HASSERIAL1)
	char stream = IFILE; /* default is file operation */
	char* filename;
	int mode;

/* which stream do we open? default is FILE */
	nexttoken();
	if (token == '&') {
		if (!expectexpr()) return;
		stream=pop();
		if (token != ',') {error(EUNKNOWN); return; }
		nexttoken();
	}
	
/* the filename and its length */
	// getfilename(filename, 0);
	filename = getfilename2(0);
	if (!USELONGJUMP && er) return; 

/* and the arguments */
	args=0;
	if (token == ',') { 
		nexttoken(); 
		parsearguments();
	}

/* getting an argument, no argument is read, i.e. mode 0 */
	if (args == 0 ) { 
		mode=0; 
	} else if (args == 1) {
		mode=pop();
	} else {
		error(EARGS);
		return;
	}

/* open the stream */
#ifdef HASSERIAL1
	if ( stream == ISERIAL1 ) {
		prtclose();
		if (mode == 0) mode=9600;
		if (prtopen(filename, mode)) ert=0; else ert=1;
		nexttoken();
		return;
	}
#endif
#ifdef FILESYSTEMDRIVER
	if ( stream == IFILE ) {
		if ( mode == 1 ) {
			ofileclose();
			if (ofileopen(filename, "w")) ert=0; else ert=1;
		} else if ( mode == 2 ) {
			ofileclose();
			if (ofileopen(filename, "a")) ert=0; else ert=1;
		} else {
			ifileclose();
			if (ifileopen(filename)) ert=0; else ert=1;		
		}
		nexttoken();
		return;
	}
#endif
#ifdef HASRF24
	if ( stream == IRADIO ) {
		if (mode == 0) {
			iradioopen(filename);
		} else if (mode == 1) {
			oradioopen(filename);
		}
		nexttoken();
		return;
	}
#endif
#if defined(HASWIRE)
	if ( stream == IWIRE ) {
		wireopen(filename[0], mode);
		nexttoken();
		return;
	}
#endif
#ifdef HASMQTT
	if ( stream == IMQTT ) {
		if (mode == 0) {
			mqttsubscribe(filename);
		} else if (mode == 1) {
			mqttsettopic(filename); 
		}
		nexttoken();
		return;
	}
#endif
	error(EORANGE);
	return;
#endif
}

/*
 *	OPEN as a function, currently only implemented for MQTT
 */
static void xfopen() {
	address_t stream = popaddress();
	if (stream == 9) push(mqttstate()); else push(0);
}

/*
 *	CLOSE a file or stream 
 */
static void xclose() {
#if defined(FILESYSTEMDRIVER) || defined(HASRF24) || defined(HASMQTT) || defined(HASWIRE)
	char stream = IFILE;
	char mode;

	nexttoken();
	if (token == '&') {
		if (!expectexpr()) return;
		stream=pop();
		if (token != ',' && ! termsymbol()) {error(EUNKNOWN); return; }
		nexttoken();
	}

	parsearguments();
	if (args == 0) { 
		mode=0; 
	} else if (args == 1) {
		mode=pop();
	} else {
		error(EARGS);
		return;
	}

/* currently only close of files is implemented, should be also implemented for Wire */
	if ( stream == IFILE ) {
		if (mode == 1 || mode == 2) ofileclose(); else if (mode == 0) ifileclose();
	}
#endif
	nexttoken();
}

/*
 * FDISK - format internal disk storages of RP2040, ESP and the like
 */
static void xfdisk() {
#if defined(FILESYSTEMDRIVER)
	nexttoken();
	parsearguments();
	if (!USELONGJUMP && er) return;
	if (args > 1) error(EORANGE);
	if (args == 0) push(0);
	outsc("Format disk (y/N)?");
	(void) consins(sbuffer, SBUFSIZE);
	if (sbuffer[1] == 'y') formatdisk(pop());
	if (fsstat(1) > 0) outsc("ok\n"); else outsc("fail\n");
#endif
	nexttoken();
}

#ifdef HASSTEFANSEXT
/*	
 *	USR low level function access of the interpreter
 *	for each group of functions there is a call vector
 *	and and argument.
 *
 * USR arguments from 0 to 31 are reserved for the 
 * interpreter status and the input output stream 
 * mechanisms. All other values are free and can be used
 * for individual functions - see case 32 for an example.
 */
static void xusr() {
	address_t fn;
	number_t v;
	int arg;
	address_t a;

	v = pop();
	arg = (int)v; /* a bit paranoid here */
	fn = pop();
/* USR(0,y) delivers all the internal constants and variables or the interpreter */		
	if ( fn == 0 ) {
		if ( arg == 0 )	push(bsystype); 
		else if ( arg == 1 ) { /* language set identifier, odd because USR is part of STEFANSEXT*/
			a = 0;
#ifdef HASAPPLE1
			a |= 1;	
#endif
#ifdef HASARDUINOIO
			a |= 2;
#endif
#ifdef HASFILEIO
			a |= 4;
#endif 
#ifdef HASDARTMOUTH
			a |= 8;
#endif
#ifdef HASGRAPH
			a |= 16;
#endif
#ifdef HASDARKARTS
			a |= 32;
#endif
#ifdef HASIOT
			a |= 64;
#endif
			push(a);
		}
		else if ( arg == 2 ) push(0); /* reserved for system speed identifier */	

#ifdef HASFLOAT
		else if ( arg == 3 ) push(-1);
#else 
		case 3: push(0); break;
#endif
		else if ( arg == 4 ) push(numsize);
		else if ( arg == 5 ) push(maxnum);
		else if ( arg == 6 ) push(addrsize);
		else if ( arg == 7 ) push(maxaddr);
		else if ( arg == 8 ) push(strindexsize);
		else if ( arg == 9 ) push(memsize + 1);
		else if ( arg == 10 ) push(elength());
		else if ( arg == 11 ) push(GOSUBDEPTH);
		else if ( arg == 12 ) push(FORDEPTH);
		else if ( arg == 13 ) push(STACKSIZE);
		else if ( arg == 14 ) push(BUFSIZE);
		else if ( arg == 15 ) push(SBUFSIZE);
		else if ( arg == 16 ) push(ARRAYSIZEDEF);
		else if ( arg == 17 ) push(defaultstrdim);
/* - 24 reserved, don't use */
		else if ( arg == 24 ) push(top);
		else if ( arg == 25 ) push(here);
		else if ( arg == 26 ) push(himem);
		else if ( arg == 27 ) push(0);
		else if ( arg == 28 ) push(freeRam());
		else if ( arg == 29 ) push(gosubsp);
		else if ( arg == 30 ) push(loopsp); 
		else if ( arg == 31 ) push(0);  /* fnc removed as interpreter variable */
		else if ( arg == 32 ) push(sp);
#ifdef HASDARTMOUTH
		else if ( arg == 33 ) push(data);
#else
		else if ( arg == 33 ) push(0);
#endif
		else if ( arg == 34 ) push(0);
#ifdef FASTTICKERPROFILE
		else if ( arg == 35 ) push(avgfasttick);
#endif
/* - 48 reserved, don't use */
		else if ( arg == 48 ) push(id);
		else if ( arg == 49 ) push(idd);
		else if ( arg == 50 ) push(od);
		else if ( arg == 51 ) push(odd);
		else push(0);
	}	
/* access to properties of stream 1 - serial	*/
	else if ( fn == 1 ) push(serialstat(arg));
/* access to properties of stream 2 - display and keyboard */			
	else if ( fn == 2 ) {
#if defined(DISPLAYDRIVER) || defined(GRAPHDISPLAYDRIVER)
		push(dspstat(arg));
#elif defined(ARDUINOVGA)
		push(vgastat(arg));
#else 
		push(0);
#endif
	}
/* access to properties of stream 4 - printer */
#ifdef HASSERIAL1		
	else if ( fn == 4 )	push(prtstat(arg));	
#endif	
/* access to properties of stream 7 - wire */
#if defined(HASWIRE)	
	else if ( fn == 7 )	push(wirestat(arg));	
#endif
/* access to properties of stream 8 - radio */
#ifdef HASRF24	
	else if ( fn == 8 )	push(radiostat(arg));	
#endif
/* access to properties of stream 9 - mqtt */
#ifdef HASMQTT	
	else if ( fn == 9 )	push(mqttstat(arg));	
#endif
/* access to properties of stream 16 - file */
#ifdef FILESYSTEMDRIVER	
	else if ( fn == 16 ) push(fsstat(arg));	
#endif
	else {
		if (fn>31) push(usrfunction(fn, v)); else push(0);
	}
}
#endif

/*
 * CALL currently only to exit the interpreter
 */
static void xcall() {
	int r;

	if (!expectexpr()) return;
	r = pop();
	if ( r == 0) {
/* flush the EEPROM dummy and the output file and then exit */
		eflush();  
		ofileclose();
#if defined(POSIXFRAMEBUFFER)
		vgaend();  /* clean up if you have played with the framebuffer */
#endif
		restartsystem();
	}
/* restart the filesystem - only test code */
	else if ( r == 1 ) fsbegin();
/* call values to 31 reserved! */
	else {
/* your custom code into usrcall() */
		if (r > 31) usrcall(r); else { error(EORANGE); return; }
		nexttoken();
	}
}

/* the dartmouth stuff */
#ifdef HASDARTMOUTH
/*
 * DATA is simply skipped when encountered as a command
 */
static void xdata() {
	while (!termsymbol()) nexttoken();
}

/* 
 * for READ find the next data record, helper of READ
 */
static void nextdatarecord() {
	address_t h;
	mem_t s=1;

/* save the location of the interpreter and the token we are processing */
	h=here;

/* data at zero means we need to init it, by searching the first data record */
	if (data == 0) {
		here = 0;
		while (here < top && token != TDATA) gettoken();
		#if TRACE_PRINTF
		printf("[nextdatarecord] %d\n", token);
		#endif
		data = here;
		datarc = 1;
	} 

processdata:
/*
 * data at top means we have exhausted all data, 
 * nothing more to be done here, however we simulate
 * a number value of 0 here and don't throw an error
 * this is not Dartmouth style, more consistent with
 * iterables
 */
	if (data == top) { 
		token=NUMBER;
		x=0;
		ert=1;
		here=h;
		return;
	}
	
/* we process the data record by setting the here pointer to data and search with gettoken */
	here=data;
	gettoken();
	#if TRACE_PRINTF
	printf("[nextdatarecord2] %d\n", token);
	#endif
	if (token == '-') {
		s = -1;
		gettoken();
		#if TRACE_PRINTF
		printf("[nextdatarecord3] %d\n", token);
		#endif
	}
	if (token == NUMBER || token == STRING) goto enddatarecord;
	if (token == ',') {
		gettoken();
		#if TRACE_PRINTF
		printf("[nextdatarecord4] %d\n", token);
		#endif
		if (token == '-') {
			s = -1;
			gettoken();
			#if TRACE_PRINTF
			printf("[nextdatarecord4] %d\n", token);
			#endif
		}
		if (token != NUMBER && token != STRING) {
			error(EUNKNOWN);  
			here = h;
			return;
		}
		goto enddatarecord;
	}

	if (termsymbol()) {
		while (here < top && token != TDATA)
			gettoken();
		#if TRACE_PRINTF
		printf("[nextdatarecord5] %d\n", token);
		#endif
		data=here;
		goto processdata;
	}

	if (DEBUG) { outsc("** error in nextdata after termsymbol "); outnumber(data); outcr(); }
	error(EUNKNOWN);

enddatarecord:
	if (token == NUMBER && s == -1) { x=-x; s=1; } /* this is needed because we tokenize only positive numbers */
	data=here;
	datarc++;
	here=h;

	if (DEBUG) { 
		outsc("** leaving nextdata with data and here "); 
		outnumber(data); outspc(); 
		outnumber(here); outcr(); 
	}
}

/*
 *	READ - find data records and insert them to variables
 */
static void xread(){
	token_t t0;	/* remember the left hand side token until the end of the statement, type of the lhs */

	lhsobject_t lhs;

	mem_t datat;	/* the type of the data element */
	address_t lendest, lensource, newlength;
	int k;
	bstring_t s;
	

nextdata: 
/* look for the variable */	
	nexttoken();

/* this code evaluates the left hand side - remember type and name */
	lhs.name=name;

	lefthandside(&lhs);
	if (!USELONGJUMP && er) return;


	if (DEBUG) {
		outsc("** read lefthandside "); 
		outname(&lhs.name); 
		outsc(" at here "); 
		outnumber(here); 
		outsc(" and data pointer "); 
		outnumber(data); 
		outcr();
	}
	

/* if the token after lhs is not a termsymbol or a comma, something is wrong */
	if (!termsymbol() && token != ',') { error(EUNKNOWN); return; }

/* remember the token we have draw from the stream */
	t0=token;

/* find the data and assign */ 
	nextdatarecord();
	if (!USELONGJUMP && er) return;

/* assign the value to the lhs - somewhat redundant code to assignment */
	if ( token == NUMBER ) {
/* a number is stored on the stack */
		assignnumber2(&lhs, x);
	}
	else if ( token == STRING ) {
		if (lhs.name.token != STRINGVAR) {
/* we read a string into a numerical variable */
			if (sr.address) assignnumber2(&lhs, memread2(sr.address));
			else assignnumber2(&lhs, *sr.ir);
		} else {
/* we have all we need in sr */
/* the destination address of the lefthandside, on the fly create included */
			getstring(&s, &lhs.name, lhs.i, lhs.j);
			if (!USELONGJUMP && er) return;

/* the length of the lefthandside string */
			lendest=s.length;

			if (DEBUG) {
				outsc("* read stringcode "); outname(&lhs.name); outcr();
				outsc("** read source string length "); outnumber(sr.length); outcr();
				outsc("** read dest string length "); outnumber(s.length); outcr();
				outsc("** read dest string dimension "); outnumber(s.strdim); outcr();
			}

/* does the source string fit into the destination */
			if ((lhs.i+sr.length-1) > s.strdim) { error(EORANGE); return; }

/* now write the string */
			assignstring(&s, &sr, sr.length);

/* classical Apple 1 behaviour is string truncation in substring logic */
			newlength = lhs.i+sr.length-1;	
			setstringlength(&lhs.name, newlength, lhs.j);

		}
	}
	else {
		error(EUNKNOWN);
		return;
	}

/* next list item */
	if (t0 == ',') goto nextdata;

/* no nexttoken here as we have already a termsymbol */
	if (DEBUG) {
		outsc("** leaving xread with "); outnumber(token); outcr();
		outsc("** at here "); outnumber(here); outcr();
		outsc("** and data pointer "); outnumber(data); outcr();
	}

/* restore the token for further processing */
	token=t0;
}

/*
 *	RESTORE sets the data pointer to zero right now 
 */
static void xrestore(){
	short rec;

	nexttoken();

/* a plain restore */
	if (termsymbol()) {
		data=0;
		datarc=1;
		return;
	}

/* something with an argument */
	expression();
	if (!USELONGJUMP && er) return;

/* we search a record */
	rec=pop();

/* if we need to search backward, back to the beginning */
	if (rec < datarc) {
		data=0;
		datarc=1;
	}

/* advance to the record or top */
	while (datarc < rec && data < top) nextdatarecord();

/* token is poisoned after nextdatarecord, need to cure this here */
	nexttoken();
}

/*
 * DEF a function, functions are tokenized as FN ARRAYVAR to make
 * name processing easy.
 */
static void xdef(){
	address_t a;

	name_t function; /* the name of the function */
	name_t variable; /* the name of the argument */

/*  do we define a function */
	if (!expect(TFN, EUNKNOWN)) return;

/* the name of the function, it is tokenized as an array */
	if (!expect(ARRAYVAR, EUNKNOWN)) return;

	function=name;
	function.token=TFN; /* set the right type here */

/* the argument variable */ 
	if (!expect('(', EUNKNOWN)) return;
	nexttoken(); 
	if (token == ')') { 
		zeroname(&variable);
	} else if (token == VARIABLE) {
		variable=name;
		nexttoken();
	} else {
		error(EUNKNOWN);
		return;
	}
	if (token != ')') { error(EUNKNOWN); return; }

/* which type of function do we store is found in token */
	nexttoken();

/* ready to store the function */
	if (DEBUG) {
		outsc("** DEF FN with function "); 
		outname(&function); 
		outsc(" and argument ");
		outname(&variable);
		outsc(" at here "); 
		outnumber(here);
		outsc(" and token is ");
		outnumber(token);
		outcr();
	}

/* find the function, we allow redefinition, currently only functions with 1 argument */
	if ((a=bfind(&function))==0) a=bmalloc(&function, 1);
	if (DEBUG) {outsc("** found function structure at "); outnumber(a); outcr(); }
	if (!USELONGJUMP && er) return;

/* no more memory */
	if (a == 0) { error(EVARIABLE); return; }

/* store the payload */

/* first the jump address */
	setaddress(a, memwrite2, here);
	a=a+addrsize;

/* the type of the return value - at the moment only numbers */
	if (token == '=') 
		memwrite2(a++, VARIABLE);
	else
		memwrite2(a++, 0);

/* store the number of variables */
	memwrite2(a++, 1);

/* store the type and the name of the variables */
	memwrite2(a++, variable.token);
	setname_pgm(a, &variable);
	a=a+sizeof(name_t)-1; /* useless now but needed later */

/* skip the function body during defintion */
	if (token == '=') {
		while (!termsymbol()) nexttoken();
	} else {
#if defined(HASMULTILINEFUNCTIONS)
		while (token != TFEND) {
			nexttoken();
			if (token == TDEF || token == EOL) { error(EFUN); return; }
		}
		nexttoken();
#else
		error(EFUN);
		return;	
#endif
	}
}

/*
 * FN function evaluation, this is a call from factor or directly from
 * statement, the variable m tells xfn which one it is. 0 is from 
 * factor and 1 is from statement. 
 * 
 * This mechanism is only needed in multiline functions. In this case, 
 * a new interpreter instance is started with statement(). The variable 
 * m decides whether the stack should contain a return value (call from factor)
 * or should be empty. 
 * 
 * The new function code has local variable capability of the new heap.
 */
static void xfn(mem_t m) {
	address_t a;
	address_t h1, h2;
	name_t variable;
	token_t type;

/* the name of the function and its address */
	if (!expect(ARRAYVAR, EUNKNOWN)) return;	
	name.token=TFN;
	a=bfind(&name);
	if (a == 0) {error(EUNKNOWN); return; }

	if (DEBUG) { outsc("** in xfn found function "); outname(&name); outsc(" at "); outnumber(a); outcr(); }

/* and the argument */
	if (!expect('(', EUNKNOWN)) return;
	nexttoken();
	
/* if there is no argument, set it to zero */
	if (token == ')') {
		push(0);
	} else {
		expression();
		if (!USELONGJUMP && er) return;	
	}
	if (token != ')') {error(EUNKNOWN); return; }

/* where is the function code */
	h1=getaddress(a, memread2);
	a=a+addrsize;

	if (DEBUG) { outsc("** found function address "); outnumber(h1); outcr(); }

/* which type of function do we have*/
	type=memread2(a++);

	if (DEBUG) { outsc("** found function type "); outnumber(type); outcr(); }

/* the number of variables is always one here */
	a++;

/* what is the name of the variable, direct read as getname also gets a token */
/* skip the type here as not needed*/
	variable.token=memread2(a++);
	(void) getname(a, &variable, memread2);
	a=a+sizeof(name_t)-1;

	if (DEBUG) { outsc("** found function variable "); outname(&variable); outcr(); }

/* create a local variable and store the value in it if there is a variable */
	if (variable.c[0]) {
		if (!bmalloc(&variable, 0)) { error(EVARIABLE); return; }
		setvar(&variable, pop());
	} else {
/* create a dummy variable to make sure local variables are cleaned up */
		variable.token=VARIABLE;
		variable.c[0]='_';
		variable.c[1]=0;
		variable.l=1;
		if (!bmalloc(&variable, 0)) { error(EVARIABLE); return; }
	}

/* store here and then evaluate the function */
	h2=here;
	here=h1;

/* for simple singleline function, we directly do experession evaluation */
	if (type == VARIABLE) {
		if (DEBUG) {outsc("** evaluating expression at "); outnumber(here); outcr(); }
		if (!expectexpr()) return;
	} else {
#ifdef HASMULTILINEFUNCTIONS

/* here comes the tricky part, we start a new interpreter instance */ 

		if (DEBUG) {outsc("** starting a new interpreter instance "); outcr();}

		nexttoken();
		fncontext++;
		if (fncontext > FNLIMIT) { error(EFUN); return; }
		statement();
		if (!USELONGJUMP && er) return;
		if (fncontext > 0) fncontext--; else error(EFUN);
#else
		error(EFUN);
		return;
#endif
	}

/* now that all the function stuff is done, return to here and set the variable right */
	here=h2;
	(void) bfree(&variable);


/* now, depending on how this was called, make things right, we remove
	the return value from the stack and call nexttoken */
	if (m == 1) {
		pop();
		nexttoken();
	}
}

/*
 *	ON is a bit like IF  
 */

static void xon(){
	number_t cr, tmp;
	int ci;
	token_t t;
	int line = 0;
	
/*  ON can do the ON ERROR and ON EVENT commands as well, in this BASIC 
		ERROR and EVENT can also be used without the ON */
	
	nexttoken();
#ifdef HASERRORHANDLING
	if ( token == TERROR ) {
		xerror();
		return;
	}
#endif
#ifdef HASEVENTS
	if ( token == TEVENT ) {
		xevent();
		return;
	}
#endif
	expression();
	if (!USELONGJUMP && er) return;

/* the result of the condition, can be any number even large */
	cr=pop();
	if (DEBUG) { outsc("** in on condition found "); outnumber(cr); outcr(); } 

/* is there a goto or gosub */
	if (token != TGOSUB && token != TGOTO)  { error(EUNKNOWN); return; }

/* remember if we do gosub or goto */
	t=token;

/* how many arguments have we got here */
	nexttoken();
	parsearguments();
	if (!USELONGJUMP && er) return;
	if (args == 0) { error(EARGS); return; }
	
/* do we have more arguments then the condition? */
	if (cr > args && cr <= 0) ci=0; else ci=(int)cr;

/* now find the line to jump to and clean the stack, reuse cr 
 * we need to clean the stack here completely, therefore complete the loop
 * ERROR handling is needed for the trapping mechanism. No using popaddress()
 * here, because of the needed stack cleanup. Unclear is this precaution is 
 * really needed.
 */
	while (args) {
		tmp=pop();
		if (args == ci) {
			if (tmp < 0) {
				er=ELINE;
			}
			line=tmp;
		}
		args--;
	}
	if (!USELONGJUMP && er) return;
	
	if (DEBUG) { outsc("** in on found line as target "); outnumber(line); outcr(); }

/* no line found to jump to */
	if (line == 0) {
		nexttoken();
		return;
	}

/* prepare for the jump	*/
	if (t == TGOSUB) pushgosubstack(0);
	if (!USELONGJUMP && er) return;

	findline(line);
	#if TRACE_PRINTF
	printf("[xon]\n");
	#endif
	if (!USELONGJUMP && er) return;

/* goto in interactive mode switched to RUN mode
		no clearing of variables and stacks */
	if (st == SINT) st=SRUN;

}
#endif

/* the structured BASIC extensions, WHILE, UNTIL, and SWITCH */

#ifdef HASSTRUCT
static void xwhile() {

/* what? */
	if (DEBUG) { outsc("** in while "); outnumber(here); outspc(); outnumber(token); outcr(); }

/* interactively we need to save the buffer location */
	if (st == SINT) here=bi-ibuffer;

/* save the current location and token type, here points to the condition, name is irrelevant */ 
	pushloop(0, TWHILE, here, 0, 0);

/* is there a valid condition */
	if (!expectexpr()) return;

/* if false, seek WEND and clear the stack*/
	if (!pop()) {
		droploop();
		if (st == SINT) bi=ibuffer+here;
		nexttoken();
		findbraket(TWHILE, TWEND);
		nexttoken();
	}
}

static void xwend() {
	blocation_t l;
	bloop_t* loop;

/* remember where we are */
	pushlocation(&l);

/* back to the condition */
	loop = activeloop();
	if (!USELONGJUMP && er) return;

/* is this a while loop */
	if (loop->var.token != TWHILE ) { error(TWEND); return; }

/* interactive run or program run */
	if (st == SINT) bi=ibuffer+loop->here; else here=loop->here;

/* is there a valid condition */
	if (!expectexpr()) return;

/* if false, seek WEND */
	if (!pop()) {
		droploop();
		poplocation(&l);
		nexttoken();
	} 
}

static void xrepeat() {
	/* what? */
	if (DEBUG) { outsc("** in repeat "); outnumber(here); outspc(); outnumber(token); outcr(); }

/* interactively we need to save the buffer location */
	if (st == SINT) here=bi-ibuffer;

/* save the current location and token type, here points statement after repeat */ 
	pushloop(0, TREPEAT, here, 0, 0);

/* we are done here */
	nexttoken();
}

static void xuntil() {
	blocation_t l;
	bloop_t* loop;

/* is there a valid condition */
	if (!expectexpr()) return;

/* remember the location */
	pushlocation(&l);

/* look on the stack */
	loop = activeloop();
	if (!USELONGJUMP && er) return;

/* if false, go back to the repeat */
	if (!pop()) {

/* the right loop type ? */
		if (loop->var.token != TREPEAT) {
			error(TUNTIL);
			return;
		}

/* correct for interactive */
		if (st == SINT) bi=ibuffer+loop->here; else here=loop->here;

	} else {

/* back to where we were */
		droploop();
		poplocation(&l);
	}

	nexttoken(); /* a bit of evil here, hobling over termsymbols */
}

static void xswitch() {
	number_t r;
	mem_t match = 0;
	mem_t swcount = 0;
	blocation_t l;

/* lets look at the condition */
	if (!expectexpr()) return;
	r=pop();

/* remember where we are */
	pushlocation(&l);

/* seek the first case to match the condition */
	while (token != EOL) {
		if (token == TSWEND) break;
		/* nested SWITCH - skip them all*/
		if (token == TSWITCH) {

			if (DEBUG) { outsc("** in SWITCH - nesting found "); outcr(); }

			nexttoken();
			findbraket(TSWITCH, TSWEND);

			if (DEBUG) { outsc("** in SWITCH SWEND found at "); outnumber(here); outcr(); }

			if (!USELONGJUMP && er) return;
		}
		/* a true case */
		if (token == TCASE) {

/* more sophisticated, case can have an argument list */
			nexttoken();
			parsearguments();

			if (DEBUG) { outsc("** in CASE found "); outnumber(args); outsc(" arguments"); outcr(); }

			if (!USELONGJUMP && er) return;
			if (args == 0) {
				error(TCASE);
				return;
			}
			while (args > 0) {
				if (pop() == r) match=1;
				args--;
			}

			if (match) {
				return;
			}
		}
		nexttoken();
	}

/* return to the original location and continue if no case is found */
	poplocation(&l); 	
}

/* a nacked case statement always seeks the end of the switch, 
	currently SWITCH statements cannot be nested.  */
static void xcase() {
	while (token != EOL) {
		nexttoken();
		if (token == TSWEND) break;
	}
}
#endif


/* 
 *	statement processes an entire basic statement until the end 
 *	of the line. 
 *
 *	The statement loop is a bit odd and requires some explanation.
 *	A statement function called in the central switch here must either
 *	call nexttoken as its last action to feed the loop with a new token 
 *	and then break or it must return which means that the rest of the 
 *	line is ignored. A function that doesn't call nexttoken and just 
 *	breaks causes an infinite loop.
 *
 *	statement is called once in interactive mode and terminates 
 *	at end of a line. 
 */

static void statement(){
	mem_t xc;
	if (DEBUG) bdebug("statement \n"); 
/* we can long jump out out any function now, making error handling easier */
/* if we return here with a long jump, only the error handler is triggered */
/* this mechanism always branches to the highest context */
#if USELONGJUMP == 1
	if (fncontext == 0) if (setjmp(sthook)) goto errorhandler;
#endif
/* the core loop processing commands */
	while ( token != EOL && !marked_to_exit ) {
#ifdef HASSTEFANSEXT
/* debug level 1 happens only in the statement loop */
		if (debuglevel == 1) { debugtoken(); outcr(); }
#endif
		register token_t t = token;
		#if TRACE_PRINTF
		printf("token: %d\n", t);
		#endif
		if ( t == ':' || t == LINENUMBER ) {
			nexttoken();
			#if TRACE_PRINTF
			printf("token2: %d\n", token);
			#endif
		}
/* Palo Alto BASIC language set + BREAK */
		else if ( t == TPRINT ) xprint();
		else if ( t == TLET ) {
			nexttoken();
			if ((token != ARRAYVAR) && (token != STRINGVAR) && (token != VARIABLE)) {
				error(EUNKNOWN);
				break;
			}
		}
		else if ( t == STRINGVAR || t == ARRAYVAR || t == VARIABLE ) assignment();
		else if ( t == TINPUT )	xinput();
		else if ( t == TRETURN ) {
#ifndef HASMULTILINEFUNCTIONS
			xreturn();
#else 
			if (fncontext > 0) {
				nexttoken();
				if (termsymbol()) { push(0); }
				else expression();
				return;
			} else 
				xreturn();
#endif
		}
#ifndef HASMULTILINEFUNCTIONS
		else if ( t == TGOSUB || t == TGOTO ) xgoto();	
#else 
		else if ( t == TGOSUB ) {
			if (fncontext > 0) { error(EFUN); return; }
			xgoto();
		}
		else if ( t == TGOTO ) xgoto();
#endif
		else if ( t == TIF ) xif();
		else if ( t == TFOR ) xfor();
		else if ( t == TNEXT ) xnext();
		else if ( t == TBREAK ) xbreak();
		else if ( t == TSTOP || t == TEND ) {		/* return here because new input is needed, end as a block end is handles elsewhere */
			*ibuffer=0;	/* clear ibuffer - this is a hack */
			st=SINT;		/* switch to interactive mode */
			eflush(); 	/* if there is an EEPROM dummy, flush it here (protects flash storage!) */
			ofileclose();
			nexttoken();
			if (token == TSTOP) {
				restartsystem();
			}
			*ibuffer=0;	/* clear ibuffer - this is a hack */
			st=SINT;		/* switch to interactive mode */
			return;
		}
		else if ( t == TLIST ) xlist();
		else if ( t == TNEW ) {		/* return here because new input is needed */
			xnew();
			return;
		}
		else if ( t == TCONT ) {	/* cont behaves differently in interactive and in run mode */
			if (st==SRUN || st==SERUN) {
				xcont();
				goto b;
			}		/* no break here, because interactively CONT=RUN minus CLR */
			xrun();
			return;	
		}
		else if ( t == TRUN ) {
			xrun();
			return;
		}	
		else if ( t == TREM ) xrem();
/* Apple 1 language set */
#ifdef HASAPPLE1
		else if ( t == TDIM ) xdim();
		else if ( t == TCLR ) xclr();
		else if ( t == TTAB || t == TSPC ) xtab();
		else if ( t == TPOKE ) xpoke();
#endif
/* Stefan's tinybasic additions */
		else if ( t == TSAVE ) xsave();
		else if ( t == TLOAD ) {
			xload(0);
			if (st == SINT) return; /* interactive load doesn't like break as the ibuffer is messed up; */
		}
#ifdef HASSTEFANSEXT
		else if ( t == TDUMP ) xdump();
		else if ( t == TGET ) xget();
		else if ( t == TPUT ) xput();
		else if ( t == TSET ) xset();
		else if ( t == TNETSTAT ) xnetstat();
		else if ( t == TCLS ) {
			ax = od;
/* if we have a display it is the default for CLS */
#if defined(DISPLAYDRIVER) || defined(GRAPHDISPLAYDRIVER)		
			od = ODSP;
#endif
#if VT52
			outch(12);
#else
			clrScr(0);
#endif
			od = ax;
			nexttoken();
		}
		else if ( t == TLOCATE ) xlocate();
/* low level functions as part of Stefan's extension */
		else if ( t == TCALL ) xcall();
#endif
/* Arduino IO */
#ifdef HASARDUINOIO
		else if ( t == TDWRITE ) xdwrite();
		else if ( t == TAWRITE ) xawrite();
		else if ( t == TPINM ) 	xpinm();
		else if ( t == TDELAY )	xdelay();
#ifdef HASTONE
		else if ( t == TTONE ) xtone();
#endif
#ifdef HASPULSE
		else if ( t == TPULSE )	xpulse();
#endif
#endif
/* BASIC DOS function */
#ifdef HASFILEIO
		else if ( t == TCATALOG ) xcatalog();
		else if ( t == TDELETE ) xdelete();
		else if ( t == TOPEN ) xopen();
		else if ( t == TCLOSE )	xclose();
		else if ( t == TFDISK )	xfdisk();
#endif
/* graphics */
#ifdef HASGRAPH
		else if ( t == TCOLOR )	xcolor();
		else if ( t == TPLOT ) xplot();
		else if ( t == TLINE ) xline();
		else if ( t == TRECT ) xrect();
		else if ( t == TCIRCLE ) xcircle();
		else if ( t == TFRECT ) xfrect();
		else if ( t == TFCIRCLE ) xfcircle();
#endif
#ifdef HASDARTMOUTH
		else if ( t == TDATA ) xdata();
		else if ( t == TREAD ) xread();
		else if ( t == TRESTORE ) xrestore();
		else if ( t == TDEF ) xdef();
		else if ( t == TON ) xon();
#ifdef HASMULTILINEFUNCTIONS
		else if ( t == TFN ) xfn(1);
		else if ( t == TFEND ) {
/* we leave the statement loop and return to the calling expression() */
/* if the function is ended with FEND we return 0 */ 
			if (fncontext == 0) { error(EFUN); return; } 
			else { push(0); return; } 
		}
#endif
#endif
#ifdef HASSTEFANSEXT
		else if ( t == TELSE ) xelse();
#endif
#ifdef HASDARKARTS
		else if ( t == TEVAL ) xeval();
#endif
#ifdef HASERRORHANDLING
		else if ( t == TERROR )	xerror();
#endif
#ifdef HASIOT
		else if ( t == TSLEEP )	xsleep();
		else if ( t == TWIRE ) xwire();
#endif
#ifdef HASTIMER
		else if ( t == TAFTER || t == TEVERY ) xtimer();
#endif
#ifdef HASEVENTS
		else if ( t == TEVENT )	xevent();
#endif
#ifdef HASSTRUCT
		else if ( t == TWHILE ) xwhile();
		else if ( t == TWEND ) xwend();
		else if ( t == TREPEAT ) xrepeat();
		else if ( t == TUNTIL )	xuntil();
		else if ( t == TSWITCH ) xswitch();
		else if ( t == TCASE ) xcase();
		else if ( t == TSWEND || t == TDO || t == TDEND ) nexttoken();
#endif
#ifdef HASEDITOR
		else if ( t == TEDIT ) xedit();
#endif
		else {
/*  strict syntax checking */
			error(EUNKNOWN);
			goto errorhandler;
		}
b:

/* 
 * after each statement we check on a break character 
 * on an Arduino entering "#" at runtime stops the program 
 * for POSIXNONBLOCKING we do this in the background loop
 * to avoid slowing down 
 */
#if defined(BREAKCHAR)
#ifndef POSIXNONBLOCKING
		if (checkch() == BREAKCHAR) {
			st=SINT; 
			if (od == 1) serialflush(); else xc=inch();
			return;
		}
#else 
		if (breakcondition) {
			breakcondition=0;
			st=SINT; 
			if (od == 1) serialflush(); else xc=inch();
			return;
		}
#endif
#endif

/* and after each statement, check the break pin */
#if defined(BREAKPIN) 
		if (getbreakpin() == 0) {
			st=SINT; 
			return;
		}; 
#endif	

/* and then there is also signal handling on some platforms */
#if defined(POSIXSIGNALS)
		if (breaksignal) {
			st=SINT; 
			breaksignal=0;
			serialflush();
			outcr();
			return;
		}
#endif

/* yield after each statement which is a 10-100 microsecond cycle 
		on Arduinos and the like, all background tasks are handled in byield */
		byield();

/* if error handling is compiled into the code, errors can be trapped here */
errorhandler:
#ifdef HASERRORHANDLING
		if (er) {
			if (st != SINT) {
				erh=er;
				er=0;
				register mem_t m = berrorh.type;
				if ( !m ) return;
				if ( m == TCONT ) while(!termsymbol()) nexttoken(); 
				else if ( m == TGOTO ) {
					#if TRACE_PRINTF
					printf("[statement] before timer to %d\n", after_timer.linenumber);
					#endif
					findline(berrorh.linenumber);
					berrorh.type=0;
					berrorh.linenumber=0;
					if (er) return;
				}
				else nexttoken();
			} else 
				return;
		}
#else
/* when an error is encountered the statement loop is ended */
		if (er) return;
#endif

/* 
 * if we run error free, interrupts and times can be processed 
 * 
 * We can savely interrupt and return only if here points either to 
 * a termsymbol : or LINENUMBER. NEXT is a special case. We need to 
 * catch this here because empty FOR loops never even have a termsymbol
 * a : is swallowed after FOR.
 * 
 * the interrupts are only triggered in fncontext 0, i.e. in the 
 * main loop. While in functions, all interrupts are disabled.
 *
 */		
#ifdef HASTIMER
		if ((token == LINENUMBER || token == ':' || token == TNEXT) && (st == SERUN || st == SRUN)) {
/* after is always processed before every */
			if (after_timer.enabled && fncontext == 0) {
				if (millis() > after_timer.last + after_timer.interval) {
					after_timer.enabled=0;
					if (after_timer.type == TGOSUB) {
						if (token == TNEXT || token == ':') here--;
						if (token == LINENUMBER) here-=(1+sizeof(address_t));	
						pushgosubstack(0);
					}
					#if TRACE_PRINTF
					printf("[statement] after timer to %d\n", after_timer.linenumber);
					#endif
					findline(after_timer.linenumber);
          			if (er) return; 
				}
			}
/* periodic events */
			if (every_timer.enabled && fncontext == 0) {
				if (millis() > every_timer.last + every_timer.interval) {
						every_timer.last=millis();
						if (every_timer.type == TGOSUB) {
							if (token == TNEXT || token == ':') here--;
							if (token == LINENUMBER) here-=(1+sizeof(address_t));	
							pushgosubstack(0);
							if (er) return;
						}
						#if TRACE_PRINTF
						printf("[statement] timer to %d\n", every_timer.linenumber);
						#endif
						findline(every_timer.linenumber);
						if (er) return; 
				}
			}
		}
#endif

/* the branch code for interrupts, we round robin through the event list */
#ifdef HASEVENTS
		if ((token == LINENUMBER || token == ':' || token == TNEXT) && (st == SERUN || st == SRUN)) {
/* interrupts */
			if (events_enabled && fncontext == 0) {
				for (ax = 0; ax < EVENTLISTSIZE; ++ax) {
					if (eventlist[ievent].pin && eventlist[ievent].enabled && eventlist[ievent].active) {
						if (eventlist[ievent].type == TGOSUB) {
							if (token == TNEXT || token == ':') --here;
							if (token == LINENUMBER) here -= (1 + sizeof(address_t));	
							pushgosubstack(TEVENT);
							if (er) return;
						}
						#if TRACE_PRINTF
						printf("[statement] lookup for %d\n", eventlist[ievent].linenumber);
						#endif
						findline(eventlist[ievent].linenumber);  
						if (er) return; 
						eventlist[ievent].active = 0;
						enableevent(eventlist[ievent].pin); /* events are disabled in the interrupt function, here they are activated again */
						events_enabled = 0; /* once we have jumped, we keep the events in BASIC off until reenabled by the program*/
						break;
					}
					ievent =(ievent + 1) % EVENTLISTSIZE;
				}
			}
		}
#endif
		#if TRACE_PRINTF
		printf("token3: %d\n", token);
		#endif
	}
}

/*
 *	the setup routine - Arduino style
 */
static void setup() {
/* start measureing time */
	timeinit();

/* initialize the event system */
#ifdef HASEVENTS
	initevents();
#endif 

/* init all io functions */
	ioinit();
#ifdef FILESYSTEMDRIVER
 	// if (fsstat(1) == 1 && fsstat(2) > 0) outsc("Filesystem started\n");
#endif

/* get the BASIC memory, either as memory array with
	ballocmem() or as an SPI serical memory */
#if (defined(SPIRAMINTERFACE) || defined(SPIRAMSIMULATOR)) && MEMSIZE == 0
	himem=memsize=spirambegin();
#else 
#if defined(EEPROMMEMINTERFACE)
/* 
 *  for an EEPROMMEM system, the memory consists of the 
 *  EEPROM from 0 to elength()-eheadersize and then the RAM.
 */
	himem=memsize=ballocmem()+(elength()-eheadersize);
#else
	himem=memsize=ballocmem();
#endif
#endif

#ifndef EEPROMMEMINTERFACE
	if (DEBUG) { outsc("** on startup, memsize is "); outnumber(memsize); outcr(); }

/* be ready for a new program if we run on RAM*/
 	xnew();	

	if (DEBUG) { outsc("** on startup, ran xnew "); outcr(); }
#else
/* if we run on an EEPROM system, more work is needed */
	if (eread(0) == 0 || eread(0) == 1) { /* have we stored a program and don't do new */
		top=getaddress(1, beread);
		resetbasicstate(); /* the little brother of new, reset the state but let the program memory be */
		for (address_t a=elength(); a<memsize; a++) memwrite2(a, 0); /* clear the heap i.e. the basic RAM*/
  	} else {
		eupdate(0, 0); /* now we have stored a program of length 0 */
		setaddress(1, beupdate, 0);
		xnew();
	}
#endif

/* check if there is something to autorun and prepare 
		the interpreter to got into autorun once loop is reached */
 	if (!autorun()) {
		printmessage(MGREET); outspc();
		printmessage(EOUTOFMEMORY); outspc(); 
		if (memsize < maxnum) outnumber(memsize+1); else { outnumber(memsize/1024+1); outch('k'); }
		outspc();
		outnumber(elength()); outcr();
 	}

/* activate the BREAKPIN */
	breakpinbegin();
}

/* 
 *	the loop routine for interactive input 
 */
static void loop() {

/*
 *	autorun state was found in setup, autorun now but only once
 *	autorun BASIC programs return to interactive after completion
 *	autorun code always must loop in itself
 */
	if (st == SERUN) {
		xrun();
 /* on an EEPROM system we don't set top to 0 here */
 #ifndef EEPROMMEMINTERFACE
		top=0;
 #endif
		st=SINT;
	} else if (st == SRUN) {
		here=0;
		xrun();
		st=SINT;
	}
	if (marked_to_exit) return;

/* always return to default io channels once interactive mode is reached */
	iodefaults();
	form=0;

/* the prompt and the input request */
	printmessage(MPROMPT);
	(void) ins(ibuffer, BUFSIZE-2);
        
/* tokenize first token from the input buffer */
	bi=ibuffer;
	nexttoken();

/* a number triggers the line storage, anything else is executed */
	if (token == NUMBER) {
		ax=x;
		storeline();	

/* on an EEPROM system we store top after each succesful line insert */
#ifdef EEPROMMEMINTERFACE
		setaddress(1, beupdate, top);
#endif
	} else {
		/* st=SINT; */
		statement();   
		st=SINT;
	}

/* here, at last, all errors need to be catched and back to interactive input*/
	if (er) reseterror();

}

/* if we are not on an Arduino, we need a main */
#ifndef ARDUINO
int main(void){
    cmd_ctx_t* ctx = get_cmd_ctx();
/* save the arguments if there are any */
#ifdef HASARGS
	bargc = ctx->argc;
	bargv = ctx->argv;
#endif
  
/* do what an Arduino would do, this loops for every interactive input */
	setup();
	while (!marked_to_exit) {
		loop();
	}
	#if TRACE_PRINTF
	printf("Done. Cleanup mem: [%ph]\n", mem);
	#endif
	free(mem);
    return 0;
}
#endif

int _init(void) {
  /* put your setup code here, to run once: */
	sp = 0; 
	memset(stack, 0, sizeof(accu_t) * STACKSIZE);
	ibuffer[0] = 0;
	loopsp = 0;
	gosubsp = 0;
	form = 0;
#ifdef MSARRAYLIMITS
	msarraylimits = 1;
	arraylimit = 0;
#else
	msarraylimits = 0;
	arraylimit = 1;
#endif
	booleanmode = BOOLEANMODE;
	forceint = 0;
	defaultstrdim = STRSIZEDEF;
	randombase = 0;
	#ifdef SUPPRESSSUBSTRINGS
	substringmode = 0;
	#else 
	substringmode = 1;
	#endif
	reltab = 0;
	lowercasenames = 0;
	debuglevel = DEFAULT_DEBUG_LVL;
#ifdef HASDARTMOUTH
	data = 0;
	datarc = 1;
#endif
#ifdef HASARGS
	bnointafterrun = 0;
#endif
	outliteral = 0;
	lexliteral = 0; 
#ifdef HASTIMER
	resettimer(&after_timer);
	resettimer(&every_timer);
#endif
#ifdef HASEVENTS
/* the event list */
	nevents = 0;
	ievent = 0;
	events_enabled = 1;
	memset(eventlist, 0, sizeof(bevent_t) * EVENTLISTSIZE);
#endif
#ifdef HASERRORHANDLING
	berrorh.linenumber = 0;
	berrorh.type = 0;
	erh = 0;
#endif
/* the string for real time clocks */
	rtcstring[0] = 0; 
/* the units pulse operates on, in microseconds*/
	bpulseunit = 10; 
/* only needed for POSIXNONBLOCKING */
	breakcondition = 0;
/* the FN context, how deep are we in a nested function call, negative values reserved */
	fncontext = 0; 
/* the accuracy of a equal or not equal statement on numbers */
#ifdef HASFLOAT
	epsilon = 0;
#endif
	linecachehere = 0;
	marked_to_exit = false;
	init_runtime();
}
