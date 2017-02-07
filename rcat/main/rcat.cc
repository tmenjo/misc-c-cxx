#include <algorithm>	// count
#include <cstdlib>	// exit
#include <fstream>	// ifstream
#include <iostream>
#include <numeric>	// accmulate
#include <string>
#include <vector>

#include <unistd.h>	// getopt

namespace rcat {

static const char kRecordSeparator('\n');

static char gFieldSeparator('\t');

static inline bool IsSingleCharString(const char *s)
{
	return (s != NULL && s[0] != '\0' && s[1] == '\0');
}

static std::vector<std::string> ParseOption(int argc, char **argv)
{
	int c(-1);
	while ((c = ::getopt(argc, argv, "d:")) != -1) {
		switch (c) {
		case 'd': // field separator
			if (!IsSingleCharString(::optarg)) {
				std::exit(1);
			}
			gFieldSeparator = ::optarg[0];
			break;
		default:
			std::exit(1);
		}
	}

	return std::vector<std::string>(&argv[optind], &argv[argc]);
}

static inline bool ReachingBlankLineEof(
	std::istream &stream, std::string &line)
{
	std::getline(stream, line, kRecordSeparator);
	return (stream.eof() && line.empty());
}

static inline int CountFieldSeparator(const std::string &s)
{
	//XXX is this cast really safe?
	return static_cast<int>(
		std::count(s.begin(), s.end(), gFieldSeparator));
}

static inline bool AllEndOfFile(const std::vector<std::ifstream> &files)
{
	return std::all_of(files.begin(), files.end(),
		[](const std::ifstream &file) { return file.eof(); });
}

template <class InputIterator, class T,
	class BinaryOperation, class NullaryOperation>
static T Join(
	InputIterator first, InputIterator last, T init,
	BinaryOperation bin_op, NullaryOperation nul_op)
{
	if (first == last)
		return init;

	init = bin_op(init, *first);
	for (++first; first != last; ++first) {
		nul_op();
		init = bin_op(init, *first);
	}
	return init;
}

static bool RunHeader(
	std::ifstream &file,
	std::vector<int> &nr_seps, std::string &record)
{
	std::string line;
	if (ReachingBlankLineEof(file, line)) {
		nr_seps.push_back(-1);
		return false;
	}

	nr_seps.push_back(CountFieldSeparator(line));
	record.append(line);
	return true;
}

static bool AnyHeaderNotEof(
	std::vector<std::ifstream> &files,
	std::vector<int> &nr_seps, std::string &record)
{
	return Join(files.begin(), files.end(), false,
		[&nr_seps, &record](bool init, std::ifstream &file) {
			return RunHeader(file, nr_seps, record) || init;
		},
		[&record]() { record.append(1, gFieldSeparator); });
}

static bool RunBody(std::ifstream &file, int nr_seps, std::string &record)
{
	if (file.eof()) {
		record.append(nr_seps, gFieldSeparator);
		return false;
	}

	std::string line;
	if(ReachingBlankLineEof(file, line)) {
		record.append(nr_seps, gFieldSeparator);
		return false;
	}

	if (nr_seps != CountFieldSeparator(line))
		exit(1);

	record.append(line);
	return true;
}

static bool AnyBodyNotEof(
	std::vector<std::ifstream> &files,
	std::vector<int> &nr_seps, std::string &record)
{
	std::size_t i(0); //XXX soooooooooooooooooo ugry
	return Join(files.begin(), files.end(), false,
		[&nr_seps, &record, &i](bool init, std::ifstream &file) {
			return RunBody(file, nr_seps[i++], record) || init;
		},
		[&record]() { record.append(1, gFieldSeparator); });
}

static int Run(const std::vector<std::string> &args)
{
	const std::size_t length(args.size());
	if (length == 0)
		return 1;

	// 1) open files
	std::vector<std::ifstream> files;
	files.reserve(length);
	std::for_each(args.cbegin(), args.cend(),
		[&files](const std::string &arg) {
			files.emplace_back(arg);
			if (files.crbegin()->fail()) {
				std::cerr << "cannot open " << arg << std::endl;
				exit(1);
			}
		});

	// 2) read header
	std::vector<int> nr_seps;
	nr_seps.reserve(length);
	{
		std::string record;
		if (AnyHeaderNotEof(files, nr_seps, record))
			std::cout << record << kRecordSeparator;
	}

	// 3) read body
	while (!AllEndOfFile(files)) {
		std::string record;
		if (AnyBodyNotEof(files, nr_seps, record))
			std::cout << record << kRecordSeparator;
	}

	return 0;
}

} // namespace rcat

int main(int argc, char **argv)
{
	std::vector<std::string> args(rcat::ParseOption(argc, argv));
	return rcat::Run(args);
}
