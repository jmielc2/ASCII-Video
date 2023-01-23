#include <iostream>
#include "Buffer.h"

HANDLE outHandle;

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return 0;
	}

	string filename(argv[1]);
	outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	Buffer buffer(filename, 64);
	
	do {
		waitKey(buffer.getDelay() - 10);
	} while (buffer.isOpen() && buffer.write());
	buffer.stop();

	return 0;
}