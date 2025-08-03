.PHONY: all clean test

headers:=Casio/FZ-1.h Casio/FZ-1_API.h
3headers:=3/tinyxml2/tinyxml2.h 3/tinywav/tinywav.h
3files=3/tinyxml2/tinyxml2.cpp 3/tinywav/tinywav.c

target:=fzutility.exe
cppfiles:=main.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp

test_target:=fzutility_tests.exe
test_cppfiles:=tests.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp


all: $(target) $(test_target)

clean:
	rm -rf $(target) $(test_target)

test: all
	./$(test_target) $V

tags: makefile $(cppfiles) $(test_cppfiles) $(3files) $(headers) $(3headers)
	ctags -R .

$(target): makefile $(cppfiles) $(3files) $(headers) $(3headers)
	g++ -g -std=c++17 -I . $(filter %.cpp,$^) $(filter %.c,$^) -o $@

$(test_target): makefile $(test_cppfiles) $(3files) $(headers) $(3headers)
	g++ -g -std=c++17 -I . $(filter %.cpp,$^) $(filter %.c,$^) -o $@


