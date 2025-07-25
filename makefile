headers:=Casio/FZ-1.h Casio/FZ-1_API.h

target:=fzutility.exe
cppfiles:=main.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp

test_target:=fzutility_tests.exe
test_cppfiles:=tests.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp


all: $(target) $(test_target)

clean:
	rm -rf $(target) $(test_target)

test: all
	./$(test_target)

$(target): $(cppfiles) $(headers)
	g++ -std=c++17 -I . $(filter %.cpp,$^) -o $@

$(test_target): $(test_cppfiles) $(headers)
	g++ -std=c++17 -I . $(filter %.cpp,$^) -o $@


