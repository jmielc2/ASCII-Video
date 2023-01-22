#include <opencv2/opencv.hpp>
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

using namespace cv;

#define DEFAULT ' '
#define NONE ' '
#define STAR '.'
#define LOW '*'
#define MEDIUM '='
#define HIGH '#'

size_t DIM_X, DIM_Y;
int section_x, section_y;
std::wstring buffer;
std::vector<Mat> channels;
Mat frame;
HANDLE outHandle;

void printBuffer() {
	std::puts("\x1b[H");
	WriteConsole(outHandle, buffer.c_str(), (int)((DIM_X * DIM_Y) + DIM_Y), 0, 0);
}

char processSection(int x, int y) {
	Range xRange(x * section_x, (x * section_x) + section_x);
	Range yRange(y * section_y, (y * section_y) + section_y);
	Mat section(frame, yRange, xRange);

	char res = NONE;
	long sum = 0;
	
	// Calculate Average GrayScale Value for Section
	for (int i = 0; i < section_y; i++) {
		for (int j = 0; j < section_x; j++) {
			sum += (unsigned int) section.at<uchar>(i, j);
		}
	}
	double val = (sum / static_cast<double>(section_x * section_y)) / 255.0f;

	if (val > 0.6f) {
		res = 65 + ((val - 0.6f) * 62.5f);
	} else if (val >= 0.3f) {
		res = 97 + ((val - 0.3f) * 83.3f);
	} else {
		res = 32 + (val * 50);
	}
	return res;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return 0;
	}

	std::string filename(argv[1]);
	VideoCapture video(filename);
	if (!video.isOpened()) {
		std::cout << "Error: unable to open '" << filename << "'." << std::endl;
		return 0;
	}

	outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	double fps = video.get(CAP_PROP_FPS);
	int delay = static_cast<int>(1000.0f / fps);
	const size_t width = static_cast<size_t>(video.get(CAP_PROP_FRAME_WIDTH));
	const size_t height = static_cast<size_t>(video.get(CAP_PROP_FRAME_HEIGHT));

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(outHandle, &csbi);
	DIM_X = (size_t)csbi.srWindow.Right - csbi.srWindow.Left;
	DIM_Y = (size_t)csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
	section_x = (int)(width / DIM_X);
	section_y = (int)(height / DIM_Y);

	buffer = std::wstring((DIM_X * DIM_Y) + DIM_Y, DEFAULT);
	for (int i = 0; i < DIM_Y; i++) {
		buffer[(i * (DIM_X + 1)) + DIM_X] = '\n';
	}
	
	std::puts("\x1b[2J\033[2J\033[1;1H");
	video.read(frame);
	if (frame.type() != CV_8UC3) {
		std::cout << "Error: can't handle this pixel type." << std::endl;
		return 0;
	}
	do {
		for (int i = 0; i < DIM_Y; i++) {
			for (int j = 0; j < DIM_X; j++) {
				buffer[(i * (DIM_X + 1)) + j] = processSection(j, i);
			}
		}
		printBuffer();
		waitKey(delay - 10);
	} while (video.read(frame));

	return 0;
}