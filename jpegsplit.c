#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <endian.h>

enum {
	SOI = 0xFFD8,
	EOI = 0xFFD9,
	SOS = 0xFFDA,
	APP0 = 0xFFE0,
};
static unsigned char soi_marker[2] = {0xff, 0xd8};

static void show_usage(){
	printf(
		"usage: jpegsplit SRC [DST]\n"
		"Detect and extract additional data efter end of JPEG-images.\n"
		"If no destination is set it returns 0 if extra data was found or 1 if pure jpeg.\n"
	);
}

static void error(const char* func){
	fprintf(stderr, "jpegsplit: %s failed: %s\n", func, strerror(errno));
}

int main(int argc, const char* argv[]){
	if ( strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 ){
		show_usage();
		return 0;
	}

	if ( argc < 2 ){
		show_usage();
		return 1;
	}

	/* open src */
	int src = open(argv[1], O_RDONLY);
	if (src == -1){
		error("open");
		return 1;
	}

	/* obtain file size */
	struct stat sb;
	if (fstat(src, &sb) == -1){
		error("fstat");
		return 1;
	}

	/* memory-map file */
	const unsigned char* data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, src, 0);
	if ( data == MAP_FAILED ){
		error("mmap");
		return 1;
	}

	/* validate jpeg */
	const unsigned char* ptr = data;
	const unsigned char* end = data + sb.st_size;
	if ( memcmp(ptr, soi_marker, 2) != 0 ){
		fprintf(stderr, "jpegsplit: unrecognized jpeg %s\n", argv[1]);
		return 1;
	}
	ptr += 2;

	do {
		/* safety check */
		if ( ptr > end ){
			fprintf(stderr, "jpegsplit: pointer moved past end-of-file, truncated file?\n");
			return 1;
		}

		/* read marker */
		const uint16_t marker = be16toh(*((uint16_t*)ptr));
		ptr += 2;
		if ( marker == EOI ) break;

		/* validate marker */
		if ( (marker & 0xff00) != 0xff00 ){
			fprintf(stderr, "jpegsplit: unrecognized marked %x\n", marker);
			return 1;
		}

		/* size of marker */
		const uint16_t size = be16toh(*((uint16_t*)ptr));
		ptr += size;

		/* start-of-scan marker */
		if ( marker == SOS ){
			do {
				/* read data until a new frame marker arrives */
				while ( *ptr++ != 0xFF && ptr < end );
				if ( *ptr == 0x00 && ptr < end ) continue;
				ptr--;
				break;
			} while (1);
		}
	} while (1);

	/* detect presence of additional data. */
	const int extra_bytes = end - ptr;
	if ( extra_bytes == 0){
		return 1;
	}

	/* no output filename given, just return successful */
	if ( argc < 3 ){
		return 0;
	}

	/* write extra data to new file */
	FILE* dst = fopen(argv[2], "w");
	fwrite(ptr, extra_bytes, 1, dst);
	fclose(dst);

	return 0;
}
