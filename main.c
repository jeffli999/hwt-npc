#include <stdio.h>
#include "common.h"
#include "bit-chunk.h"


void
test_range()
{
	Range	a, b;

	a.lo = 1; a.hi = 5;
	b.lo = 5; b.hi = 10;

	if (range_overlap(a, b))
		printf("overlapped\n");
	else
		printf("not overlapped\n");
}


void test_chunk()
{
	Chunk		chunks[3];
//	uint32_t	point = 0x1ca8;
//	uint32_t	point = 0x1c88;
	uint32_t	point = 0x12f4ff78;
//	uint32_t	point = 0x12f4fff8;
//	uint32_t	point = 0x1234eff8;
//	uint32_t	point = 0x1fb8;
	uint32_t	frame, found;

//	chunks[0].dim = 2; chunks[0].seq = 3, chunks[0].val = 0x1;
//	chunks[1].dim = 2; chunks[1].seq = 1, chunks[1].val = 0xa;

	chunks[0].dim = 2; chunks[0].seq = 7, chunks[0].val = 0x1;
	chunks[1].dim = 2; chunks[1].seq = 4, chunks[1].val = 0x4;
	chunks[2].dim = 2; chunks[2].seq = 1, chunks[2].val = 0x7;

	found = find_frame(&frame, point, chunks, 3, 32);
	if (found)
		printf("Found: %x\n", frame);
	else
		printf("Not found: %x\n", frame);
}



int main(int argc, char **argv)
{
	test_chunk();
}
