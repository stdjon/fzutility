.PHONY: all clean doc doc-clean test

headers:=Casio/FZ-1.h Casio/FZ-1_API.h
3headers:=3/tinyxml2/tinyxml2.h 3/tinywav/tinywav.h
3files=3/tinyxml2/tinyxml2.cpp 3/tinywav/tinywav.c

target:=fzutility.exe
cppfiles:=main.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp

test_target:=fzutility_tests.exe
test_cppfiles:=tests.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp

doc_targets:=doc/classes.png doc/fz-ml.html doc/fzutility.html

CPPFLAGS:=-g -std=c++17 -Werror -Wall -Wno-format -I .

all: $(target) $(test_target)

clean:
	rm -rf $(target) $(test_target)

test: all
	./$(test_target) $V

tags: $(cppfiles) $(test_cppfiles) $(3files) $(headers) $(3headers) makefile
	ctags -R .

doc: $(doc_targets)

doc-clean:
	rm -rf $(doc_targets)

$(target): $(cppfiles) $(3files) $(headers) $(3headers) makefile
	g++ $(CPPFLAGS) $(filter %.cpp,$^) $(filter %.c,$^) -o $@

$(test_target): $(test_cppfiles) $(3files) $(headers) $(3headers) makefile
	g++ $(CPPFLAGS) $(filter %.cpp,$^) $(filter %.c,$^) -o $@

doc/%.png: doc/%.dot makefile
	dot -Tpng $< -o$@

doc/%.html: doc/style.html doc/%.md makefile
	cat $(filter-out makefile,$^) | pandoc -f markdown_strict -t html - -o $@


