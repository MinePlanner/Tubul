//
// Opening files for reading, decompressing gzip files while they are read.
//
// Every file is read through zlib: a gzip-compressed file is decompressed,
// and any other file is read as it is (zlib's transparent mode). One path for
// both kinds of files, as fast as std::ifstream on plain files.

#include "tubul_file_utils.h"
#include <zlib.h>
#include <cerrno>
#include <cstring>
#include <istream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

namespace TU
{
namespace
{

//A stream buffer that reads a file through zlib, decompressing it while it's
//read if it is gzip-compressed. A read error (corrupt or truncated compressed
//data) throws, with zlib's message.
class ZlibStreamBuffer: public std::streambuf
{
public:
	//Opens the file; throws if it can't be opened.
	explicit ZlibStreamBuffer(const std::string& filename):
		filename_(filename),
		buffer_(kBufferSize)
	{
		errno = 0;
		file_ = gzopen(filename.c_str(), "rb");
		//zlib leaves the reason in errno when the file itself can't be opened.
		if (file_ == nullptr)
			throw std::runtime_error("couldn't open file '" + filename + "'" +
									 (errno != 0 ? std::string(": ") + std::strerror(errno) : std::string()));
		gzbuffer(file_, kBufferSize);
	}

	//Closes the file.
	~ZlibStreamBuffer() override
	{
		gzclose(file_);
	}

	//It owns the open file, so it can't be copied.
	ZlibStreamBuffer(const ZlibStreamBuffer&) = delete;
	ZlibStreamBuffer& operator=(const ZlibStreamBuffer&) = delete;

protected:
	//Called by the stream when it has read all the buffered characters: reads
	//(and decompresses) the next block of the file into the buffer. Returns the
	//next character, or eof at the end of the file.
	int_type underflow() override
	{
		if (gptr() < egptr())
			return traits_type::to_int_type(*gptr());

		const int numRead = gzread(file_, buffer_.data(), static_cast<unsigned>(buffer_.size()));
		int errnum = Z_OK;
		const char* message = gzerror(file_, &errnum);
		if (numRead < 0)
			throw std::runtime_error("error reading file '" + filename_ + "': " + message);
		//zlib doesn't fail a read that reaches the end of a file in the middle
		//of compressed data: it returns what it decompressed, with Z_BUF_ERROR
		//set until the next read. That is an error.
		if (errnum == Z_BUF_ERROR)
			throw std::runtime_error("error reading file '" + filename_ + "': unexpected end of file");
		if (numRead == 0)
			return traits_type::eof();

		setg(buffer_.data(), buffer_.data(), buffer_.data() + numRead);
		return traits_type::to_int_type(*gptr());
	}

private:
	//Bytes read from the file at a time.
	static constexpr unsigned kBufferSize = 256 * 1024;

	std::string filename_;
	std::vector<char> buffer_;
	gzFile file_ = nullptr;
};

//An input stream that owns its zlib stream buffer. Its badbit throws, so a
//read error reaches the caller with zlib's message, instead of looking like
//the end of the file.
class ZlibInputStream: public std::istream
{
public:
	//Opens the file; throws if it can't be opened.
	explicit ZlibInputStream(const std::string& filename):
		std::istream(nullptr),
		buffer_(filename)
	{
		rdbuf(&buffer_);
		exceptions(std::ios::badbit);
	}

private:
	ZlibStreamBuffer buffer_;
};

} // namespace

//Every file goes through zlib (see the top of this file).
std::unique_ptr<std::istream> openInputFile(const std::string& filename)
{
	return std::make_unique<ZlibInputStream>(filename);
}

} // namespace TU
