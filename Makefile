#
# Unity3D_BLE plugin Makefile for Linux
#
# For Mac OS X, use XCode
#

DIR = Assets/Plugins

PLUGIN = $(DIR)/libUnity3D_BLE.so
TEST   = $(DIR)/test

TARGETS = $(PLUGIN) $(TEST)

OBJS = Unity3D_BLENativeManager.o Unity3D_BLENativePeripheral.o

# XXX
CFLAGS = -U__APPLE__ -D__linux__
CFLAGS += -fPIC
CFLAGS += -Wl,--no-undefined
CFLAGS += -g
LIBS = -lsystemd

all:	$(DIR) $(TARGETS)

$(PLUGIN): $(OBJS)
	$(CC) $(CFLAGS) -shared $(OBJS) -o $@ $(LIBS)

$(TEST): Plugin_Test.o $(PLUGIN)
	$(CC) $(CFLAGS) -o $@ Plugin_Test.o $(PLUGIN)

$(DIR):
	mkdir -p $(DIR)

test:	$(TEST)
	./$(TEST)

clean:
	rm -f $(TARGETS) $(OBJS)

# XXX Silly, FIXME
install: $(PLUGIN)
	rm -f ../$(PLUGIN)
	cp $(PLUGIN) ../$(PLUGIN)
