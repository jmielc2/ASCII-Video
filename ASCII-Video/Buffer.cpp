#include "Buffer.h"
#include <iostream>

#define NONE ' '
#define STAR '.'
#define LOW '*'
#define MEDIUM '='
#define HIGH '#'

HANDLE outHandle;

Buffer::SharedFrame::SharedFrame(const wstring& frame) : frame(frame), final(false), ready(false) {
	return;
}

Buffer::Buffer(const string& filename, int size) : filename(filename), curFrame(0), opened(false) {
	if (size > 256) {
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
		wstring temp(length, NONE);
		for (int j = 0; j < DIM_Y; j++) {
			temp[(j * (DIM_X + 1)) + DIM_X] = '\n';
		}
		for (int i = 0; i < size; i++) {
			this->buf.push_back(new Buffer::SharedFrame(temp));
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
	this->opened = true;
	this->video.set(CAP_PROP_POS_FRAMES, 0);

	// Start Loading Frames
	this->loader = thread(&Buffer::frameLoader, this);
}

Buffer::~Buffer() {
	for (Buffer::SharedFrame* frame : this->buf) {
		delete frame;
	}
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
		bool state = this->buf[curFrame]->ready.load();
		while (this->video.isOpened() && state) {
			this->buf[curFrame]->ready.wait(state);
			state = this->buf[curFrame]->ready.load();
		}
		if (!this->video.isOpened()) {
			return;
		}
		for (int i = 0; i < this->DIM_Y; i++) {
			for (int j = 0; j < this->DIM_X; j++) {
				this->buf[curFrame]->frame[(i * (this->DIM_X + 1)) + j] = processSection(j, i);
			}
		}
		this->buf[curFrame]->ready.store(true);
		this->buf[curFrame]->ready.notify_all();
		curFrame = (curFrame + 1 == (int)this->buf.size()) ? 0 : curFrame + 1;
	}
	curFrame = (curFrame == 0) ? (int)(this->buf.size() - 1) : curFrame - 1;
	this->buf[curFrame]->final.store(true);
}

bool Buffer::write() {
	bool state = this->buf[curFrame]->ready.load();
	while (!state) {
		this->buf[curFrame]->ready.wait(state);
		state = this->buf[curFrame]->ready.load();
	};
#ifndef _DEBUG
	puts("\x1b[H");
	WriteConsole(outHandle, this->buf[curFrame]->frame.c_str(), (DWORD)this->buf[curFrame]->frame.size(), 0, 0);
#endif
	if (this->buf[curFrame]->final.load()) {
		return false;
	}
	this->buf[curFrame]->ready.store(false);
	this->buf[curFrame]->ready.notify_all();
	curFrame = (curFrame + 1 == this->buf.size()) ? 0 : curFrame + 1;
	return true;
}

void Buffer::stop() {
	if (this->video.isOpened()) {
		this->video.release();
	}
	this->buf[curFrame]->ready.store(false);
	this->buf[curFrame]->ready.notify_all();
	this->loader.join();
	return;
}