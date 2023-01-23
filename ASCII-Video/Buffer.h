#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <Windows.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

extern HANDLE outHandle;

class Buffer {
private:
	struct SharedFrame {
		bool ready;
		wstring frame;

		SharedFrame(bool state, const wstring& frame);
	};

	string filename;
	bool loading;
	int delay, section_x, section_y;
	size_t DIM_X, DIM_Y;
	vector<SharedFrame> buf;
	VideoCapture video;
	Mat frame;
	thread loader;

	char processSection(const int x, const int y);
	void frameLoader();

public:
	Buffer(const string &filename, int size = 16);
	~Buffer() {};

	bool write();
	void stop();

	int getDelay() const { return this->delay; }
	bool isOpen() const { return this->video.isOpened(); }
};
