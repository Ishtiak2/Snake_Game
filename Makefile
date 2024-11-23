all:
	g++ -o main snake.cpp -F/Library/Frameworks/ -framework SDL2 -framework SDL2_ttf -framework SDL2_mixer -framework SDL2_image
	./main