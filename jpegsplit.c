/**
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <endian.h>
#include <getopt.h>
#include <magic.h>

#define VERSION "1.4"

enum {
	SOI = 0xFFD8,
	EOI = 0xFFD9,
	SOS = 0xFFDA,
	APP0 = 0xFFE0,
};

struct format {
	unsigned char* signature;
	size_t signature_size;
	const unsigned char* (*func)(const unsigned char* ptr, const unsigned char* end);
};

static unsigned char jpg_signature[2] = {0xff, 0xd8};
static unsigned char png_signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
static magic_t magic = NULL;

static const unsigned char* do_jpg(const unsigned char* ptr, const unsigned char* end);
static const unsigned char* do_png(const unsigned char* ptr, const unsigned char* end);

struct format known_formats[] = {
	{jpg_signature, sizeof(jpg_signature), do_jpg},
	{png_signature, sizeof(png_signature), do_png},
	{0, 0, 0} /* sentinel */
};

static void error(const char* func){
	fprintf(stderr, "jpegsplit: %s failed: %s\n", func, strerror(errno));
}

static const unsigned char* do_jpg(const unsigned char* ptr, const unsigned char* end){
	do {
		/* safety check */
		if ( ptr > end ){
			fprintf(stderr, "jpegsplit: pointer moved past end-of-file, truncated file?\n");
			exit(1);
		}

		/* read marker */
		const uint16_t marker = be16toh(*((const uint16_t*)ptr));
		ptr += 2;
		if ( marker == EOI ) break;

		/* validate marker */
		if ( (marker & 0xff00) != 0xff00 ){
			fprintf(stderr, "jpegsplit: unrecognized marked %x\n", marker);
			exit(1);
		}

		/* size of marker */
		const uint16_t size = be16toh(*((const uint16_t*)ptr));
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

	return ptr;
}

static const unsigned char* do_png(const unsigned char* ptr, const unsigned char* end){
	do {
		char type[4];

		/* read chunk, ignore data */
		const uint32_t len = be32toh(*(const uint32_t*)ptr); ptr += 4;
		memcpy(type, ptr, 4); ptr += 4;
		ptr += len + 4;

		if ( memcmp(type, "IEND", 4) == 0 ){
			break;
		}
	} while (1);
	return ptr;
}

static struct format* match_signature(const unsigned char* ptr){
	struct format* format = &known_formats[0];
	while ( format->signature ){
		if ( memcmp(ptr, format->signature, format->signature_size) == 0 ){
			return format;
		}
		format++;
	}
	return NULL;
}

static void output(const char* filename, const unsigned char* begin, const unsigned char* end, const unsigned char* ref){
	const size_t bytes = end - begin;
	const char* magic_desc = magic_buffer(magic, begin, bytes);

	fprintf(stderr, "jpegsplit: `%s' found at offset 0x%zx (%zd bytes)", magic_desc, begin-ref, bytes);
	if ( !filename ){
		fputc('\n', stderr);
		return;
	}

	fprintf(stderr, ", writing to %s\n", filename);
	FILE* dst = fopen(filename, "w");
	fwrite(begin, bytes, 1, dst);
	fclose(dst);
}

static const char* shortopts = "hv";
static const struct option longopts[] = {
	{"help",      no_argument,       0, 'h'},
	{"version",   no_argument,       0, 'v'},
	{0, 0, 0, 0}, /* sentinel */
};

static void show_usage(){
	printf(
		"Detect and extract additional data efter end of images.\n"
		"If no destination is set it returns 0 if extra data was found or 1 if the file was pure.\n"
		"\n"
		"usage: jpegsplit SRC [DST..]\n"
		"  -v, --version      Show version and exit.\n"
		"  -h, --help         This text.\n"
	);
}

static void show_version(){
	puts("jpegsplit-" VERSION);
}

int main(int argc, char* argv[]){
	int op, option_index = -1;
	while ( (op = getopt_long(argc, argv, shortopts, longopts, &option_index)) != -1 ){
		switch (op){
		case 0:   /* long opt */
		case '?': /* unknown opt */
			break;

		case 'h': /* --help */
			show_usage();
			return 0;

		case 'v': /* --version */
			show_version();
			return 0;
		}
	}

	/* initialize libmagic */
	magic = magic_open(MAGIC_MIME_TYPE);
	if ( !magic ) {
		fprintf(stderr, "jpegsplit: unable to initialize magic library\n");
	} else if ( magic_load(magic, NULL) != 0 ) {
		fprintf(stderr, "jpegsplit: cannot load magic database - %s\n", magic_error(magic));
		magic_close(magic);
		magic = NULL;
	}

	const size_t num_files = argc-optind;
	if ( num_files == 0 ){
		show_usage();
		return 1;
	}

	/* open src */
	const char* src_filename = argv[optind++];
	int src = open(src_filename, O_RDONLY);
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

	/* setup pointers */
	const unsigned char* ptr = data;
	const unsigned char* begin = NULL;
	const unsigned char* end = data + sb.st_size;

	/* try known formats */
	struct format* format = match_signature(ptr);
	if ( !format ){
		fprintf(stderr, "jpegsplit: unrecognized image %s\n", src_filename);
		return 1;
	}

	/* loop over the files until eof or unknown data */
	int num_found = 0;
	for ( ;; num_found++ ){
		begin = ptr;

		/* check if this is a known format */
		if ( !(format && format->func) ){
			break;
		}

		/* parse file and detect presence of additional data. */
		ptr = format->func(ptr + format->signature_size, end);
		const size_t bytes_left = end - ptr;
		if ( bytes_left == 0 ){
			break;
		}

		/* write current file (detected extra file is written in next pass) */
		const char* filename = (optind<=argc) ? argv[optind++] : NULL;
		output(filename, begin, ptr, data);

		/* guess next format */
		format = match_signature(ptr);
	}

	if ( num_found == 0 ){
		fprintf(stderr, "jpegsplit: no additional data found\n");
		return 1;
	}

	/* write last file */
	const char* filename = (optind<=argc) ? argv[optind++] : NULL;
	output(filename, begin, end, data);

	return 0;
}
