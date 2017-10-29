#
#  Device independent graphics package GLIB.
#  NULL graphic device version
#

OBJS   = glib.o
TARGET = glib.a

ROOT = ../../../../../../
include ../../../Makefile.config

$(TARGET): $(OBJS)
	ar crv $(TARGET) $(OBJS)

clean:
	rm -rf $(OBJS) $(TARGET)
