#include "WritingTracker.h"

ByteWriter::ByteWriter(std::ofstream& output) : output(output) {

}

std::streamsize ByteWriter::write(const char* s, std::streamsize n) {
    // Call the base class write method
    output.write(s, n);

    // Update the total number of bytes written
    bytesWritten += n;

    return n;
}