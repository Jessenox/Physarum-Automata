#include <random>
#include <time.h>
#include <thread>

using namespace std;


class RandomRange {
	public:
		int getRandom(const int, const int);
	private:
	public:
	private:

};

int RandomRange::getRandom(const int min, const int max) {
	static thread_local mt19937* generator = nullptr;
	if (!generator) generator = new mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
	uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator);
}