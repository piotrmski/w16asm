appName := w16asm
testAppName := w16asm-test
CFLAGS  := -std=c23

srcFiles := $(shell find src -name "*.c")
objects  := $(patsubst %.c, %.o, $(srcFiles))

testSrcFiles := $(shell find test -name "*.c")
testObjects  := $(patsubst %.c, %.o, $(testSrcFiles))

all: $(appName) $(testAppName)

$(appName): $(objects)
	$(CC) $(CFLAGS) -o $(appName) $(objects)

$(testAppName): $(testObjects)
	$(CC) $(CFLAGS) -o $(testAppName) $(testObjects)

clean:
	rm -f $(objects) $(testObjects)