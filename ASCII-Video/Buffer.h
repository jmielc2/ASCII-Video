#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <thread>

using namespace std;
using namespace cv;

extern HANDLE outHandle;

class Buffer {
private:
	struct SharedFrame {
		wstring frame;
		bool final;
		bool ready;

		SharedFrame(const wstring& frame);
	};

	string filename;
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
};
