#include "Buffer.h"
#include <thread>

#define NONE ' '
#define STAR '.'
#define LOW '*'
#define MEDIUM '='
#define HIGH '#'

Buffer::SharedFrame::SharedFrame(const bool state, const wstring& frame) : ready(state), frame(frame) { return; }

Buffer::Buffer(const string& filename, int size) : filename(filename) {
	if (size > 128) {
		std::cout << "Error: buffer size " << size << " is too big." << std::endl;
		return;
	}
	this->video = VideoCapture(filename);
	if (!video.isOpened()) {
		std::cout << "Error: unable to open '" << filename << "'." << std::endl;
		return;
	}

	{  // Calculate Buffer Dimensions
		const size_t width = static_cast<size_t>(this->video.get(CAP_PROP_FRAME_WIDTH));
		const size_t height = static_cast<size_t>(this->video.get(CAP_PROP_FRAME_HEIGHT));
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(outHandle, &csbi);
		this->DIM_X = (size_t)csbi.srWindow.Right - csbi.srWindow.Left;
		this->DIM_Y = (size_t)csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
		this->section_x = (int)(width / this->DIM_X);
		this->section_y = (int)(height / this->DIM_Y);
	}

	{  // Calculate Frame Delay Time (ms)
		const double fps = video.get(CAP_PROP_FPS);
		this->delay = static_cast<int>(1000.0f / fps);
	}

	{  // Actually Create the Buffer and Strings
		size_t length = (this->DIM_X * this->DIM_Y) + this->DIM_Y;
		this->buf = vector<SharedFrame>(size, Buffer::SharedFrame(false, wstring(length, NONE)));
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < DIM_Y; j++) {
				buf[i].frame[(j * (DIM_X + 1)) + DIM_X] = '\n';
			}
		}
	}

	// Check Pixel Format
	puts("\x1b[2J\033[2J\033[1;1H");
	this->video.read(this->frame);
	if (this->frame.type() != CV_8UC3) {
		puts("Error: can't handle this pixel type.\n");
		this->video.release();
		return;
	}
	this->video.set(CAP_PROP_POS_FRAMES, 0);

	// Start Loading Frames
	this->loader = thread(&Buffer::frameLoader, this);
}

char Buffer::processSection(const int x, const int y) {
	Range xRange(x * section_x, (x * section_x) + section_x);
	Range yRange(y * section_y, (y * section_y) + section_y);
	Mat section(this->frame, yRange, xRange);

	char res = NONE;
	long sum = 0;

	// Calculate Average GrayScale Value for Section
	for (int i = 0; i < section_y; i++) {
		for (int j = 0; j < section_x; j++) {
			sum += (unsigned int)section.at<uchar>(i, j);
		}
	}
	double val = sum / static_cast<double>((size_t)section_x * section_y);

	if (val >= 191.25f) {
		res = HIGH;
	}
	else if (val >= 127.5f) {
		res = MEDIUM;
	}
	else if (val >= 63.75f) {
		res = LOW;
	}
	else if (val >= 20.0f) {
		res = STAR;
	}
	return res;
}

void Buffer::frameLoader() {
	int curFrame = 0;
	while (this->video.read(this->frame)) {
		while (this->video.isOpened() && this->buf[curFrame].ready) {}
		for (int i = 0; i < DIM_Y; i++) {
			for (int j = 0; j < DIM_X; j++) {
				this->buf[curFrame].frame[(i * (DIM_X + 1)) + j] = processSection(j, i);
			}
		}
		buf[curFrame].ready = true;
		curFrame = (curFrame + 1 == (int)this->buf.size()) ? 0 : curFrame + 1;
	}
	if (this->video.isOpened()) {
		this->video.release();
	}
}

bool Buffer::write() {
	static int bufNum = 0;
	while (!this->buf[bufNum].ready && this->video.isOpened()) {}
	if (this->video.isOpened()) {
		puts("\x1b[H");
		WriteConsole(outHandle, this->buf[bufNum].frame.c_str(), this->buf[bufNum].frame.size(), 0, 0);
		this->buf[bufNum].ready = false;
		bufNum = (bufNum + 1 == this->buf.size()) ? 0 : bufNum + 1;
		return true;
	}
	return false;
}

void Buffer::stop() {
	if (this->video.isOpened()) {
		this->video.release();
	}
	this->loader.join();
	return;
}