CXXFLAGS = -std=c++14 -g -I../include -Wall -Werror -O2

all: main.o
	c++ main.o

main.o: main.cpp \
	../include/dispatcher.hpp \
	../include/dispatcher/dispatcher.hpp \
	../include/dispatcher/object_id.hpp \
	../include/dispatcher/time_source.hpp \
	../include/dispatcher/wait.hpp \
	../include/dispatcher/extra/dispatcher_client.hpp

clean:
	rm -f *.o a.out
