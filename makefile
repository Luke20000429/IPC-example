
EXE			= master
OBJS		= master.o

CPPFLAGS		= -g -Wall -O3 -larmadillo 
LIBS		= -lm -lz -lpthread

CC = g++

default: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CPPFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@ $(LIBS)


cleangem:
	rm -rf *.o*
	rm -rf *.e*

clean:
	rm -rf *.o
	rm -rf $(EXE)
