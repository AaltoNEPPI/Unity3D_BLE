#
# Unity3D_BLE plugin Makefile for Linux
#
# For Mac OS X, use XCode
#

DIR = Assets/Plugins

TARGET = $(DIR)/Unity3D_BLE.a

OBJS = Unity3D_BLENativeManager.o Unity3D_BLENativePeripheral.o

# XXX
CFLAGS = -U__APPLE__ -D__linux__

all:	$(TARGET)

$(TARGET): $(OBJS)
	ar -rcs $@ $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)

# XXX Silly, FIXME
install: $(TARGET)
	rm -f ../$(TARGET)
	cp $(TARGET) ../$(TARGET)
