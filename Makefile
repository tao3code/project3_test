SRC := main.c serial.c
HEADER := serial.h robot.h
LDFLAG := -lpthread

project3.elf: $(SRC) $(HEADER) 
	gcc $(SRC) $(LDFLAG) -Wall -o project3.elf 
clean:
	rm -f *.elf 
