#include "MedianCalculator.h"
#include <Windows.h>


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


void MedianCalculator::save(FILE *f) {
	fprintf(f, "%d ", values.size());
	for (auto it = values.begin(); it != values.end(); ++it)
		fprintf(f, "%d ", *it);
}
void MedianCalculator::load(FILE *f) {
	int sz = 0, n;
	fscanf_s(f, "%d", &sz);
	sz = max(0, min(64, sz));
	while (sz-- > 0) {
		n = 0;
		fscanf_s(f, "%d", &n);
		values.insert(n);
	}
	median = calcMedian();
}