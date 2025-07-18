target:=fzutility.exe
cppfiles:=main.cpp Casio/FZ-1.cpp Casio/FZ-1_API.cpp

all: $(target)

clean:
	rm -rf $(target)

$(target): $(cppfiles)
	g++ -std=c++17 -I . $^ -o $@

$(cppfiles): Casio/FZ-1.h Casio/FZ-1_API.h

