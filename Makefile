SRC := main.c serial.c

LDFLAG := -lpthread

project3.elf: $(SRC) firmware.h serial.h
	gcc $(SRC) $(LDFLAG) -Wall -o project3.elf 
clean:
	rm -f *.elf 
