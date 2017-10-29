#
#  CS348C  Radiosity     Device independent graphics package GLIB.
#                        SUN/Xview version makefile
#
#

TARGET = glibps.a
OBJS   = glibps.o

include ../../../Makefile.config

$(TARGET): $(OBJS)
	ar crv $(TARGET) $(OBJS)

clean:
	rm -rf *.o $(TARGET)
