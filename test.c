/* ---------------------------------------------------------------------------
-- LabHelp subroutines to provide useful access to physical functions
-- of the OS/2 operating system and hardward.  These functions are normally
-- not allowed, but in lab situations, such access if often desirable.
--------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
-- void *LHGetPhysMem(unsigned long ulAddr, size_t cbSize);
--    Obtains pointer to specified physical memory locations
-- int LHGetTime(unsigned long *ulMs, unsigned long *ulNs);
--    Obtains timestamp to the nearest millisecond, ns since boot
--
-- Notes:
--
-- * Code tested with IBM CSET/2 compiler at CSD CS0022, and Microsoft C6.0A
--   compiler.  Default is the IBM CSET/2 compiler.
--
-- * #define NO_DEVELOPMENT_KIT if you don't have the OS/2 developers kit.
--
-- * #define MSC60 to use Microsoft C compiler instead of IBM CSET/2 compiler
--
-- * #include files are for POSIX standard implementation.  If not using
--   POSIX, replace <unistd.h> with <io.h>.
--
-- * Code depends on CSET/2 providing pointer conversion from _SEG16 to flat.
--   I hope IBM continues to make this work with future CSET/2 releases.
--
-- * Under Microsoft C, large model must be used if you do not have the OS/2
--   developer's kit (DosDevIOCtl call must be properly prototyped).
---------------------------------------------------------------------------- */

/* -------------------
-- Feature test macros
---------------------- */
#define _POSIX_SOURCE					/* Must always be defined for POSIX   */
/* #define MSC60 */						/* Uncomment if using Microsoft C 6.0 */
/* #define NO_DEVELOPERS_KIT */		/* Uncomment if no OS/2 develope kit  */
/* #define DEBUG */				  		/* Uncomment to get debug message	  */
#define TEST								/* Uncomment to enable test routines  */

/* ----------------------
-- Standard include files
------------------------- */
#ifndef NO_DEVELOPMENT_KIT				/* Include if it exists */
	#define INCL_DOSDEVICES
	#include <os2.h>
#elif defined MSC60						/* Otherwise, fake definitions */
	unsigned short _pascal _far
		DosDevIOCtl(void *,void *,unsigned short,unsigned short,unsigned short);
#else
	unsigned long _System
		DosDevIOCtl(unsigned long, unsigned long, unsigned long, void *,
		unsigned long, unsigned long *, void *, unsigned long, unsigned long *);
#endif

#include <stdio.h>
#include <unistd.h>						/* Replace with <io.h> if no POSIX */
#include <fcntl.h>

/* ----------------------------
-- Local macros and definitions
------------------------------- */
#ifdef MSC60								/* Create standard way to access IOCTL */
	#define IOCTL(h,ct,fc,pm,ln)	DosDevIOCtl(NULL,pm, fc,ct, h)
#else
	#define IOCTL(h,ct,fc,pm,ln)	DosDevIOCtl(h, ct,fc, pm,ln,&ln, NULL,0,NULL)
#endif

#define	LHCATEGORY	0x81			/* Category for Labhelp IOCtl calls */
#define	LHGETMEM		0x41			/* Function to allocate physical mem */
#define	LHGETTIME	0x42			/* Function to return time in ns */

/* -------------------
-- Function prototypes
---------------------- */
void *LHGetPhysMem(unsigned long ulAddr, size_t cbSize);


/* ===========================================================================
-- Description: Obtain (void *) pointer to physical memory address block.
--
-- Usage:   void *LHGetPhysMem(unsigned long ulAddr, size_t cbSize);
--
-- Inputs:	ulAddr - long (32 bit) physical address of desired block
--				cbSize - block length in bytes.  (see note below).
--
-- Outputs:	<none>
--
-- Returns: (void *) pointer to memory if successful.  NULL on failure.
--
-- Errors:  Returns NULL.  Possible reasons are driver not installed (message
--          printed) or some invalid call (ie. don't know).
--
-- Note:  * Physical memory is only accessible at Ring 0 from the device
--          driver LABHELP.SYS, which must be loaded in CONFIG.SYS.
--        * Memory under OS/2 2.0 is committed only in pages of 4096 bytes.
--          Requests for less will be satisfied with one full page.
--			 * Because of implementation, block size must be < 65535 bytes
--        * Implemented as IOCtl call to device LABHELP$ via category 0x81
--          and function 0x41.  LABHELP$ returns _SEG16 pointer to memory.
--        * Attempting to "free" pointer will likely result in a trap
--
-- WARNING: This routine blatantly bypasses OS/2's memory protection.  It goes
--          against everything OS/2 stands for, but that's why I run a single
--          user system.  This call gives access to any memory, and you can
--          trash it as you see fit.  This is now your problem, not mine.
=========================================================================== */
void *LHGetPhysMem(unsigned long ulAddr, size_t cbSize) {

	struct {
		unsigned long	PhysicalAddress;		/* 32 bit physical address		*/
		unsigned long	AccessLength;			/* Length access required		*/
#if defined MSC60
		void  *SegPtr;								/* Returned pointer				*/
#else
		void	* _Seg16 SegPtr;					/* Returned pointer				*/
#endif
	} Parms;

	int fh;											/* File handle						*/
	int rc;											/* Return code						*/
	unsigned long len=sizeof(Parms);			/* Random length variable		*/
	void	*pvPtr=NULL;							/* Normal (! _Seg16) pointer	*/

	Parms.PhysicalAddress = ulAddr;			/* Copy user values to structure */
	Parms.AccessLength    = cbSize;

	if ( (fh = open("LABHELP$", O_RDONLY)) < 0) {
		perror("LABHELP$ not found - is LABHELP.SYS installed?");
		return(NULL);
	} else {
		rc = IOCTL(fh, LHCATEGORY, LHGETMEM, &Parms, len);
		if (rc == 0) pvPtr = Parms.SegPtr;	/* Compiler handles conversion */
		close(fh);
#ifdef DEBUG
		printf("GETMEM: %i  (_Seg16 *): %p  (*): %p\n", rc, Parms.SegPtr, pvPtr);
#endif
	}
	return(pvPtr);
}

/* ===========================================================================
-- Description: Obtain timestamp in nanoseconds and milliseconds
--
-- Usage:   int LHGetTime(unsigned long *ulMs, unsigned long *ulNs);
--
-- Inputs:	<none>
--
-- Outputs:	*ulMs - If not NULL, get time in milliseconds since start of system
--          *ulNs - If not NULL, get time in nanoseconds portion of time
--                  0 < ulNs < 999999 since 1 mS is largest fraction
--
-- Returns:  0 - successful
--          !0 - unsuccessful
--
-- Errors:  No driver returns -1.  Otherwise, return code from DosDevIOCtl
--          returned.  Error printed if no driver found.
--
-- Note:  * Timer accessed from device driver LABHELP.SYS.
--        * Implemented as IOCtl call to device LABHELP$ via category 0x81
--          and function 0x42.  LABHELP$ returns private structure.
=========================================================================== */
int LHGetTime(unsigned long *ulMs, unsigned long *ulNs) {

	struct {
		unsigned long ns_per_tick;				/* ns / tick (normally 840)	*/
		unsigned long tick[2];					/* 64 bit tick count				*/
	} Parms;
	unsigned long ns, ms;
	unsigned long tick[4], remain;			/* 16 bit represent of 32 bit # */
	int i;

	int fh;											/* File handle						*/
	int rc;											/* Return code						*/
	unsigned long len=sizeof(Parms);			/* Random length variable		*/

	if ( (fh = open("LABHELP$", O_RDONLY)) < 0) {
		perror("LABHELP$ not found - is LABHELP.SYS installed?");
		return(-1);
	} else {
		rc = IOCTL(fh, LHCATEGORY, LHGETTIME, &Parms, len);
		close(fh);
#ifdef DEBUG
		printf("GETTIME: %i  Ticks: %lx%8.8lx\n", rc, Parms.tick[1], Parms.tick[0]);
#endif
		if (rc != 0) return(-1);
	}
/* -- Synthetic math - break into 4 16 bit quantities to give 65536 range */
	tick[0] = Parms.tick[0] & 0xFFFF;
	tick[1] = Parms.tick[0] >> 16;
	tick[2] = Parms.tick[1] & 0xFFFF;
	tick[3] = Parms.tick[1] >> 16;

/* -- Multiply by ns per tick to get total # of ns */
	for (i=0; i<4; i++) tick[i] *= Parms.ns_per_tick;
	for (i=0; i<3; i++) {							/* Handle carry */
		tick[i+1] += tick[i] >> 16;
		tick[i]   &= 0xFFFF;
	}

/* -- Divide limited to 2**16 = 65536, so do by 1000 twice */
	remain = 0;
	for (i=3; i>=0; i--) {								/* Divide by 1000 */
		tick[i] += remain << 16;						/* Add remainder */
		remain   = tick[i] % 1000u;					/* Next stage remainder */
		tick[i] /= 1000u;									/* And do division */
	}
	ns = remain;
	remain = 0;
	for (i=2; i>=0; i--) {
		tick[i] += (remain << 16);						/* Add remainder */
		remain   = (tick[i] % 1000u);					/* Next stage remainder */
		tick[i] /= 1000u;									/* And do division */
	}
	ns += 1000*remain;									/* uS left overs */

	ms = tick[0] + (tick[1] << 16);					/* Restore the 32 bit value */

	if (ulMs != NULL) *ulMs = (rc==0) ? ms : 0;
	if (ulNs != NULL) *ulNs = (rc==0) ? ns : 0;
	return(rc);
}


/* ******************************************************************
** TEST SECTION - ONLY NECESSARY IF YOU WANT TO TEST ALL FUNCTIONS **
****************************************************************** */

#ifdef TEST

/* --------------------------------------------
-- Additional includes/prototypes (CSET/2 only)
----------------------------------------------- */
APIRET APIENTRY DosSleep(ULONG msec);
#include <stdlib.h>
#include <string.h>

/* ===========================================================================
-- Usage: int TestGetMem(void);
--        int TestGetTime(void);
--
-- Description: Gets pointer to memory at 0xB8000, length 0x2000, video memory
--              in CGA full screen mode.  Write's 0's over screen for 1 sec.
--              Time test sleeps for 1 second and prints time of sleep.
--
-- Inputs:	<none>
--
-- Outputs: <none>
--
-- Returns: 0 (successful) or 1 (failed)
--
-- Notes:   Imagine the damage you can do with LHGetPhysMem!
=========================================================================== */
int TestGetMem(void) {

	int i;
	short *Video, *VideoHold;		
	
	if ( (Video = LHGetPhysMem(0xB8000, 0x1000)) == NULL) {
		printf("ERROR: Unable to obtain pointer to physical memory\n");
		return(1);
	}

	VideoHold = malloc(25*80*sizeof(*VideoHold));
	memcpy(VideoHold, Video, 25*80*sizeof(*VideoHold));
	for (i=0; i<25*80; i++) Video[i] = 0x0730;		/* Fill with 0's */
	DosSleep(1000L);											/* Sleep one second */
	memcpy(Video, VideoHold, 25*80*sizeof(*VideoHold));

	free(VideoHold);											/* Free memory blocks */
	return(0);
}

#define	NUMFAST	25
#define	NUMSLEEP	10

int TestGetTime(void) {

	unsigned long ms_start, ns_start, ms, ns;
	int i;
	unsigned long fast[NUMFAST];

	if (LHGetTime(&ms_start, &ns_start) != 0) return(1);

	for (i=0; i<NUMSLEEP; i++) {
		DosSleep(1000);
		if (LHGetTime(&ms, &ns) != 0) return(1);
		ms = ms - ms_start;									/* Delta ms */
		ns = (ns >= ns_start) ? ns-ns_start : (1000000+ns)-ns_start, ms--;
		printf("Total time: %lu.%6.6lu milliseconds\n", ms, ns);
	}

	for (i=0; i<NUMFAST; i++) {
		LHGetTime(&ms_start, &ns_start);
		LHGetTime(&ms, &ns);
		ms = ms - ms_start;									/* Delta ms */
		ns = (ns >= ns_start) ? ns-ns_start : (1000000+ns)-ns_start, ms--;
		fast[i] = ms*1000000+ns;
	}
	for (i=0; i<NUMFAST; i++) printf("fast: %lu\n", fast[i]);
	return(0);
}

/* ===========================================================================
-- Usage: main routine to run through LabHelp calls
--
-- Description: Runs test of each LabHelp routine
--
-- Returns: 0 (successful) or # of failures
=========================================================================== */
int main(int argc, char *argv[]) {

	int rc=0;

	if (TestGetMem() != 0) {
		rc++;
		printf("Memory access function test failed\n");
	}

	if (TestGetTime() != 0) {
		rc++;
		printf("Timer access function test failed\n");
	}

	return(rc);
}

#endif	 /* TEST */
