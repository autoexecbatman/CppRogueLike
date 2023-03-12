#ifndef PROJECT_PATH_LITERALS_H_
#define PROJECT_PATH_LITERALS_H_

#include <gsl/util>

struct Length
{
public:
	int x = 0;
	int y = 0;
};

// make X a literal operator
constexpr Length operator "" _x(unsigned long long x)
{
	return Length{ gsl::narrow_cast<int>(x), 0 };
}

// make Y a literal operator
constexpr Length operator "" _y(unsigned long long y)
{
	return Length{ 0, gsl::narrow_cast<int>(y)};
}

#endif // !PROJECT_PATH_LITERALS_H_