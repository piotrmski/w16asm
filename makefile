appName := w13asm
testAppName := w13asm-test
CFLAGS  := -std=c23

srcFiles := $(shell find src -name "*.c")
objects  := $(patsubst %.c, %.o, $(srcFiles))

testSrcFiles := $(shell find test -name "*.c")
testObjects  := $(patsubst %.c, %.o, $(testSrcFiles))

all: $(appName)

$(appName): $(objects)
	$(CC) $(CFLAGS) -o dist/$(appName) $(objects)
	cp COPYING dist/COPYING

$(testAppName): $(testObjects)
	$(CC) $(CFLAGS) -o dist/$(testAppName) $(testObjects)

clean:
	rm -f $(objects) $(testObjects)