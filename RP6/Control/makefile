###############################################################################
#
#   S i m p l e  u s a g e  i n s t r u c t i o n s:
#
#   Run from command line:
#
#   make        :   creates hex target
#   make clean  :   cleans up by removing all build products.
#
###############################################################################
# Target file name (without extension).
# This is the name of your main C source file! Do NOT append the ".c"!
# Example: Let's assume your main source file is "RP6Base_MyProgram.c", then
# you would write: TARGET = RP6Base_MyProgram
TARGET = main

###############################################################################
# Specify relative path to RP6 library files here.
# This is "../../RP6Lib" or "../RP6Lib" usually.

RP6_LIB_PATH=../../../../rp6/RP6Examples_20120725f/RP6Lib
RP6_LIB_PATH_CONTROL = $(RP6_LIB_PATH)/RP6control
RP6_LIB_PATH_COMMON = $(RP6_LIB_PATH)/RP6common
RP6_LIB_PATH_I2C_MASTER= $(RP6_LIB_PATH)/RP6control_I2CMasterLib

RP6_LIB_PATH_OTHERS= $(RP6_LIB_PATH_CONTROL) $(RP6_LIB_PATH_COMMON) 

# List any extra directories to look for include files here.
EXTRAINCDIRS = $(RP6_LIB_PATH) $(RP6_LIB_PATH_OTHERS) $(RP6_LIB_PATH_I2C_MASTER)
###############################################################################

#------------------------------------------------
# Main Source file is _automatically_ added here:
SRC = $(TARGET).c
SRC += Drive.c
SRC += Crash.c
SRC += SerialReaderSender.c
SRC += InterpretSerial.c
SRC += Tools.c
SRC += ProtocolDefines.c

SRC += $(RP6_LIB_PATH_CONTROL)/RP6ControlLib.c
SRC += $(RP6_LIB_PATH)/RP6common/RP6uart.c
SRC += $(RP6_LIB_PATH_COMMON)/RP6I2CmasterTWI.c
SRC += $(RP6_LIB_PATH_I2C_MASTER)/RP6Control_I2CMasterLib.c


include $(RP6_LIB_PATH_CONTROL)/makefile.control