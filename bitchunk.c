#include <stdio.h>
#include "bit-chunk.h"


// Corresponding to case 1/2 in find_frame(), i.e.
//     +--------+
//  p  | frame  |
//     +--------+
int frame_case1(uint32_t *frame, Chunk *chunks, int num_chunks, int curr_chunk)
{
	int			i, lo;
	uint32_t	bits;
printf("frame: %x\n", *frame);
	// 1. set all low-order bits (if any) to 0 (to get the left border of the frame)
	lo = chunks[curr_chunk].seq * CHUNK_SIZE;
	if (lo > 0) 
		clear_bits(frame, lo - 1, 0); 
printf("frame: %x\n", *frame);

	// 2. set bits of the rest chunks (non-* bits)
	for (i = curr_chunk; i < num_chunks; i++) {
		lo = chunks[i].seq * CHUNK_SIZE;
		set_bits(frame, lo+CHUNK_SIZE-1, lo, chunks[i].val);
printf("frame: %x\n", *frame);
	}

	return 1;
}



// Corresponding to case 3 (or 4) in find_frame(), i.e.
// +--------+        +--------+
// | frame' |  p     | frame  |
// +--------+        +--------+
int frame_case2(uint32_t *frame, Chunk *chunks, int num_chunks, int curr_chunk, int width)
{
	int			i, lo, len, found = 0;
	uint32_t	bits;

	// 1. add the value of higher * bits by 1
	for (i = curr_chunk; i > 0; i--) {
		len = chunks[i-1].seq - chunks[i].seq; 
		if (len == 1)	// no * bits in between two chunk[i] & chunk[i-1]
			continue;
		len = (len - 1) * CHUNK_SIZE;
		lo = (chunks[i].seq + 1) * CHUNK_SIZE;
		bits = extract_bits(*frame, lo+len-1, lo);
		if (bits == ((1 << len) - 1)) {
			// adding frame's value leads to overflow at current *-bit chunk, set them to 0, and
			// continue to find the next *-bit chunk to finish the add operation
			clear_bits(frame, lo+len-1, lo);
		} else {
			// simply adding 1 to bits is done (get the frame on the right of point)
			set_bits(frame, lo+len-1, lo, bits+1);
			found = 1;
			break;
		}
	}

	// add 1 to * bits (if any) higher than the first chunk if this is not done at lower * chunks
	if (!found) {
		lo = (chunks[0].seq + 1) * CHUNK_SIZE;
		if ( lo == width)
			return 0;	// no frames on the right of point

		bits = extract_bits(*frame, width-1, lo);
		if (bits == ((1 << (width-lo)) - 1)) {
			// the * bits are all-1, adding 1 will lead to overflow, so no frame can be found
			return 0;
		} else {
			// simply adding 1 to * bits is done (get the frame on the right of point)
			set_bits(frame, width-1, lo, bits+1);
		}
	}
	
	// 2. set all low-order bits (if any) to 0 (to get the left border of the frame)
	lo = chunks[curr_chunk].seq * CHUNK_SIZE;
	if (lo > 0)
		clear_bits(frame, lo-1, 0); 

	// 3. set bits of lower chunks (non-* bits)
	for (i = curr_chunk; i < num_chunks; i++) {
		lo = chunks[i].seq * CHUNK_SIZE;
		set_bits(frame, lo+CHUNK_SIZE-1, lo, chunks[i].val);
	}

	return 1;
}



// Among the space frames encoded by chunks, return the one containing or immediately right
// to point. Note: chunks must be ordered, e.g.,
// with chunks:  0001-****-1010-**** encoding 16 4-bit space frames: 
// 1. for point: 0001-1100-1010-1000, the frame is 0001-1100-1010-****, which contains point
// 2. for point: 0001-1100-1000-1000, the frame is 0001-1100-1000-****, which is right to point
// 3. for point: 0001-1100-1011-1000, the frame is 0001-1101-1010-****, which is also right to
// point, but the frame 0001-1100-1010-**** with *-bit chunks setting to point's values is left to
// point, which is not the frame that we desire.
// in some cases, we may not find the frame right to point, e.g,
// 4. for point: 0001-1111-1011-1000, no frame is found no matter what values are set for * bits
int find_frame(uint32_t *frame, uint32_t point, Chunk *chunks, int num_chunks, int width)
{
	uint32_t	bits;
	int			i, lo, hi, found = 0;

	*frame = point;
	for (i = 0; i < num_chunks; i++) {
		lo = chunks[i].seq * CHUNK_SIZE;
		hi = lo + CHUNK_SIZE - 1;
		bits = extract_bits(point, hi, lo);
		if (bits < chunks[i].val) {
			// case 1: point on the left of current frame
printf("case 1: chunk:%d, hi=%d, lo=%d, %x < %x\n", chunks[i].seq, hi, lo, bits, chunks[i].val);
			found = frame_case1(frame, chunks, num_chunks, i);
			break;
		} else if (bits > chunks[i].val) {
printf("case 2\n");
			// case 2: point on the right of current frame
			found = frame_case2(frame, chunks, num_chunks, i, width);
			break;
		}
	}
	if (i == num_chunks) {
printf("case 3\n");
		// case 3: point inside current frame
		hi = chunks[num_chunks-1].seq * CHUNK_SIZE - 1;
		clear_bits(frame, hi, 0);
		found = 1;
	}
	return found;
}
