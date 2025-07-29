# MacOs Compile
macos:
	clang++ -lpthread -std=c++26 SnakeUpdated.cpp -o SnakeUpdated

# Linux Compile
linux:
	gcc SnakeUpdated.cpp -o SnakeUpdated -lpthread

# Run
run:
	./SnakeUpdated
	