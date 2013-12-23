# jpegsplit

Detect and extract additional data after the end of the image (e.g. after EOI-marker). This is sometimes used to hide other files and/or data in images.

    usage: jpegsplit SRC [DST]

Return 0 if data was detected and non-zero if nothing was found.

## Supported input formats

* jpg
* png
