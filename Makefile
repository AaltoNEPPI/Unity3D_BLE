#
# Unity3D_BLE plugin Makefile for Linux
#
# For Mac OS X, use XCode
#

DIR = Assets/Plugins

TARGET = $(DIR)/libUnity3D_BLE.so

OBJS = Unity3D_BLENativeManager.o Unity3D_BLENativePeripheral.o

# XXX
CFLAGS = -U__APPLE__ -D__linux__
CFLAGS += -fPIC
CFLAGS += -Wl,--no-undefined
CFLAGS += -g

all:	$(DIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -shared $(OBJS) -o $@ -lsystemd

$(DIR):
	mkdir -p $(DIR)

clean:
	rm -f $(TARGET) $(OBJS)

# XXX Silly, FIXME
install: $(TARGET)
	rm -f ../$(TARGET)
	cp $(TARGET) ../$(TARGET)
