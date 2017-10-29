TARGET = rltotiff
OBJS   = rltotiff.o tiff_rgba_io.o

include ../../../Makefile.config

LDFLAGS := $(LDFLAGS) -ltiff

clean:
	rm -rf $(OBJS) $(TARGET)
