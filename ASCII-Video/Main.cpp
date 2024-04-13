#include <signal.h>
#include <chrono>
#include "Buffer.h"

bool stop = false;

void stp_handler(int sig) {
	stop = true;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return 0;
	}

	string filename(argv[1]);
	outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	signal(SIGINT, stp_handler);
	Buffer buffer(filename, 256);
	if (!buffer.isOpen()) {
		return 0;
	}

	do {
		auto start = chrono::high_resolution_clock::now();
		while ((chrono::high_resolution_clock::now() - start) / chrono::milliseconds(1) < buffer.getDelay()) {};
	} while (!stop && buffer.write());
	buffer.stop();

	return 0;
}