#pragma once
#include <set>
#include <string>
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

	std::string save();
	void load(char *inp);
};

