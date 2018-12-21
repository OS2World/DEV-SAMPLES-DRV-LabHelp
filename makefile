#######################################################################
# Options for assembler (to remake labhelp.sys)
# Lines below when using IBM CSET/2
#
# For Microsoft C6.0, use options /AH /Za /W3 /FPi /G2
#######################################################################
ICC=
COPTS = -Q+ -N30 -W3 -Sm -Gn- -Kb+ -G4 -DCSET2
CC    = icc
AOPTS =
ASM   = masm

# Inference rules
.SUFFIXES:
.SUFFIXES: .asm .c .obj .exe

.c.obj :
   $(CC) $(COPTS) -C+ -Fo$@ $<

.asm.obj : 
   $(ASM) $(AOPTS) $*;

###########################################################################
# Make dependencies and rules
###########################################################################
EXES:  test.exe 

labhelp.sys: $*.obj $*.def
    link $*, $*.sys,  NUL , os2286, $*

# Alternate with full map for debugging
# labhelp.sys: $*.obj $*.def
#     link /MAP $*, $*.sys,, os2286, $*
#     mapsym $*

test.exe : test.obj
    $(CC) $(COPTS) -Fe$@ $**
