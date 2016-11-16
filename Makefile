all: c-example cxx-example

CFLAGS = -Wall -Wextra \
		 -Wno-missing-field-initializers \
		 -Winit-self -pedantic -g

CXXFLAGS = -Wall -Wextra -g

c-example: c-example.c cliopts.o
	$(CC) $(CFLAGS) -o $@ $^

cxx-example: cxx-example.cpp cliopts.o
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f *.o *.so c-example cxx-example
	rm -fr *.dSYM
