all:
	avr-gcc -DF_CPU=8000000 -mmcu=attiny85 -g -Os -Wall -Wextra -std=c99 -o program.out -fshort-enums -Wa,-adhlmns=program.lst ledstrip.c
	avr-size -d program.out
	avr-objcopy -j .text -j .data -O ihex program.out program.hex
	avrdude -cusbasp -pt85 -U flash:w:program.hex
