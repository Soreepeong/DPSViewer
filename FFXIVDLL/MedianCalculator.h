#pragma once
#include<set>
class MedianCalculator {
private:
	std::multiset<int> values;
	int count;
	int median;

	int calcMedian();

public:
	MedianCalculator();
	~MedianCalculator();

	void add(int val);

	int get() {
		return median;
	}

	void save(FILE *f);
	void load(FILE *f);
};

