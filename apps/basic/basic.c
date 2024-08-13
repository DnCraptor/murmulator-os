
/* Global BASIC definitions */
#define PROGMEM
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

/* the zero terminated token dictonary needed for scalability
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

 */

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
