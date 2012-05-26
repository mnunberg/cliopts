all: libcliopts.so test

CFLAGS = -Wall -std=c89 -Winit-self -pedantic

libcliopts.so: cliopts.c
	$(CC) $(CFLAGS) -shared -o $@ -fPIC $^

test: test.c libcliopts.so
	$(CC) $(CFLAGS) -o $@ $< -Wl,-rpath='$$ORIGIN' -L. -lcliopts

clean:
	rm -f *.o *.so test
