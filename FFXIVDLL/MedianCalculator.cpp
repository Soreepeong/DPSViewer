#include "MedianCalculator.h"
#include <Windows.h>
#include <sstream>
#include <istream>


MedianCalculator::MedianCalculator() :
	count(0),
	median(0) {
}


MedianCalculator::~MedianCalculator() {
}

int MedianCalculator::calcMedian() {
	if (values.empty())
		return 0;

	const size_t n = values.size();
	int median = 0;

	auto iter = values.cbegin();
	std::advance(iter, n / 2);

	// Middle or average of two middle values
	if (n % 2 == 0) {
		const auto iter2 = iter--;
		median = (*iter + *iter2) / 2;    // data[n/2 - 1] AND data[n/2]
	} else {
		median = *iter;
	}

	return median;
}

void MedianCalculator::add(int val) {
	if (values.size() > 64) {
		if (abs(*values.begin() - val) > abs(*(--values.end()) - val))
			values.erase(values.begin());
		else
			values.erase(--values.end());
	}
	values.insert(val);
	median = calcMedian();
}


std::string MedianCalculator::save() {
	std::stringstream str;
	for (auto it = values.begin (); it != values.end (); ++it)
		str << *it << " ";
	return str.str ();
}
void MedianCalculator::load(char *inp) {
	std::istringstream in_stream(inp);
	int n;
	while (in_stream >> n) {
		values.insert(n);
	}
	median = calcMedian();
}