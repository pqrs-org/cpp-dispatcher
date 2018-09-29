CXXFLAGS = -std=c++14 -g -I../include -Wall -Werror -O2

all: test.o
	c++ test.o
	./a.out

test.o: test.cpp \
	../include/dispatcher.hpp \
	../include/dispatcher/dispatcher.hpp \
	../include/dispatcher/dispatcher_client.hpp \
	../include/dispatcher/object_id.hpp \
	../include/dispatcher/time_source.hpp

clean:
	rm -f *.o a.out
