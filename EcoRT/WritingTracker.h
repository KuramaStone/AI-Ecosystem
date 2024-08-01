#pragma once

#include <iostream>
#include <fstream>

class ByteWriter {
public:
	int bytesWritten = 0;
	std::ofstream& output;

	ByteWriter(std::ofstream& output);

	std::streamsize write(const char* s, std::streamsize n);
};