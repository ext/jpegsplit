.PHONY: clean

CONVERT_FLAGS:=-resize 300x300 -gravity center -pointsize 48

SRC_FILES=1.jpg 2.jpg 3.jpg 4.jpg 5.jpg 6.jpg text.txt
CONCAT_FILES:=simple.jpg jpg_jpg.jpg jpg_png_txt.jpg
CONCAT_MD5:=$(CONCAT_FILES:.jpg=.md5)

check: $(CONCAT_FILES) $(CONCAT_MD5)
	@./check.sh

clean:
	rm -f $(SRC_FILES) $(CONCAT_FILES) $(CONCAT_MD5)

simple.jpg: 1.jpg
	@ln -f $< $@

simple.md5: 1.jpg
	@md5sum $^ | sed 's/\([[:space:]]\)[a-z0-9]*\(\..*\)/\1tmp\2/g' > $(basename $@).md5

jpg_jpg.jpg: 1.jpg 2.jpg
	@cat $^ > $@

jpg_jpg.md5: 1.jpg 2.jpg
	@md5sum $^ | sed 's/\([[:space:]]\)[a-z0-9]*\(\..*\)/\1tmp\2/g' > $(basename $@).md5

jpg_png_txt.jpg: 1.jpg 6.png text.txt
	@cat $^ > $@

jpg_png_txt.md5: 1.jpg 6.png text.txt
	@md5sum $^ | sed 's/\([[:space:]]\)[a-z0-9]*\(\..*\)/\1tmp\2/g' > $(basename $@).md5

1.jpg: Makefile
	@convert xc:yellow $(CONVERT_FLAGS) -draw "text 0,0 'Image 1'" $@

2.jpg: Makefile
	@convert xc:blue $(CONVERT_FLAGS) -draw "text 0,0 'Image 2'" $@

3.jpg: Makefile
	@convert xc:cyan $(CONVERT_FLAGS) -draw "text 0,0 'Image 3'" $@

4.jpg: Makefile
	@convert xc:pink $(CONVERT_FLAGS) -draw "text 0,0 'Image 4'" $@

5.jpg: Makefile
	@convert xc:lime $(CONVERT_FLAGS) -draw "text 0,0 'Image 5'" $@

6.png: Makefile
	@convert xc:red $(CONVERT_FLAGS) -draw "text 0,0 'PNG Image'" $@

text.txt: Makefile
	@echo "Räksmörgås" > $@
