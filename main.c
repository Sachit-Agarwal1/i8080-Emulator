#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
FILE *file;
extern uint8_t memory[65536];
int main() { 
	file = fopen("program1", "rb");
	fseek(file, 0, SEEK_END);
	long filelen = ftell(file);
	rewind(file);
	fread(memory, 1, filelen, file);
	tick();
	fclose(file);
	return 1;
}