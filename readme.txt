To use this device driver:

(1) Add a line device=<path>labhelp.sys to config.sys and reboot.

(2) Look at test.c and understand what is happening in main.  You can scan
    the subroutine, but it's operation is pretty much irrelevent.

(3) Run test as an example program only in a full screen OS/2 session - never
    in a window.  It will gain access to the video screen memory and overwrite
    everything with 0's.

(4) Be careful with where you access memory.  The length descriptor in the
    call has a granularity of 4096 bytes (on page) as far as I can tell.
    I've been able to write beyond my requested block (to that limit) with no
    problem.

To use the Microsoft C6.0A compiler, you must #define MSC60 in the file test.c,
and then compile with recommended options /AH /Za /W3 /FPi /G2.  Probably also
have to #define NO_DEVELOPERS_KIT in test.c unless you have the developer's
toolkit installed.
