//
// Created by Daniel Espinoza on 01-11-26.
// Opening files for reading, decompressing gzip files while they are read.
//
// Every file is read through zlib: a gzip-compressed file is decompressed,
// and any other file is read as it is (zlib's transparent mode). One path for
// both kinds of files, as fast as std::ifstream on plain files.

#include <zlib.h>

#include <cerrno>
#include <cstring>
#include <format>
#include <istream>
#include <streambuf>
#include <string>
#include <vector>
#include <ios>

#include "tubul_file_utils.h"
#include "tubul_exception.h"

namespace TU
{
namespace
{

//A stream buffer that reads a file through zlib, decompressing it while it's
//read if it is gzip-compressed. A read error (corrupt or truncated compressed
//data) throws a TU::Exception, with zlib's message. That message doesn't name
//the file: the caller knows it, and the dataframe readers add it.
class ZlibStreamBuffer: public std::streambuf
{
public:
	//Opens the file; throws if it can't be opened.
	explicit ZlibStreamBuffer(std::string_view filename);

	//Closes the file.
	~ZlibStreamBuffer() override;

	//It owns the open file, so it can't be copied.
	ZlibStreamBuffer(const ZlibStreamBuffer&) = delete;
	ZlibStreamBuffer& operator=(const ZlibStreamBuffer&) = delete;

protected:
	//Called by the stream when it has read all the buffered characters: reads
	//(and decompresses) the next block of the file into the buffer. Returns the
	//next character, or eof at the end of the file.
	int_type underflow() override;

private:
	//Bytes read from the file at a time.
	static constexpr unsigned kBufferSize = 256 * 1024;

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
	explicit ZlibInputStream(std::string_view filename);

private:
	ZlibStreamBuffer buffer_;
};

} // namespace

//Every file goes through zlib (see the top of this file).
std::unique_ptr<std::istream> openInputFile(std::string_view filename)
{
	return std::make_unique<ZlibInputStream>(filename);
}

namespace
{

ZlibStreamBuffer::ZlibStreamBuffer(std::string_view filename):
	buffer_(kBufferSize)
{
	//zlib takes the name as a terminated string.
	const std::string name(filename);
	errno = 0;
	file_ = gzopen(name.c_str(), "rb");
	//zlib leaves the reason in errno when the file itself can't be opened.
	if (file_ == nullptr && errno != 0)
		throw TU::Exception(std::format("couldn't open file '{}': {}", filename, std::strerror(errno)));
	if (file_ == nullptr)
		throw TU::Exception(std::format("couldn't open file '{}'", filename));
	gzbuffer(file_, kBufferSize);
}

ZlibStreamBuffer::~ZlibStreamBuffer()
{
	gzclose(file_);
}

ZlibStreamBuffer::int_type ZlibStreamBuffer::underflow()
{
	if (gptr() < egptr())
		return traits_type::to_int_type(*gptr());

	const int numRead = gzread(file_, buffer_.data(), static_cast<unsigned>(buffer_.size()));
	int errnum = Z_OK;
	const char* message = gzerror(file_, &errnum);
	if (numRead < 0)
		throw TU::Exception(std::format("read error: {}", message));
	//zlib doesn't fail a read that reaches the end of a file in the middle
	//of compressed data: it returns what it decompressed, with Z_BUF_ERROR
	//set until the next read. That is an error.
	if (errnum == Z_BUF_ERROR)
		throw TU::Exception("read error: unexpected end of compressed data");
	if (numRead == 0)
		return traits_type::eof();

	setg(buffer_.data(), buffer_.data(), buffer_.data() + numRead);
	return traits_type::to_int_type(*gptr());
}

ZlibInputStream::ZlibInputStream(std::string_view filename):
	std::istream(nullptr),
	buffer_(filename)
{
	rdbuf(&buffer_);
	exceptions(std::ios::badbit);
}

} // namespace

} // namespace TU
