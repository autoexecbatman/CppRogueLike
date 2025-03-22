#pragma once

// {y,x} position
struct Vector2D
{
	int y{ 0 };
	int x{ 0 };

	// operator overloads

	// add two vectors
	Vector2D operator+(const Vector2D& rhs) const noexcept
	{
		return { y + rhs.y, x + rhs.x };
	}

	// add two vectors
	Vector2D operator+=(const Vector2D& rhs) noexcept
	{
		y += rhs.y;
		x += rhs.x;
		return *this;
	}

	// subtract two vectors
	Vector2D operator-(const Vector2D& rhs) const noexcept
	{
		return { y - rhs.y, x - rhs.x };
	}

	// compare two vectors
	bool operator==(const Vector2D& rhs) const noexcept
	{
		return y == rhs.y && x == rhs.x;
	}

	// compare two vectors
	bool operator!=(const Vector2D& rhs) const noexcept
	{
		return !(*this == rhs);
	}

	// compare two vectors
	bool operator<(const Vector2D& rhs) const noexcept
	{
		return y < rhs.y || (y == rhs.y && x < rhs.x);
	}

	// compare two vectors
	bool operator>(const Vector2D& rhs) const noexcept
	{
		return y > rhs.y || (y == rhs.y && x > rhs.x);
	}

	// compare two vectors
	bool operator<=(const Vector2D& rhs) const noexcept
	{
		return *this < rhs || *this == rhs;
	}

	// compare two vectors
	bool operator>=(const Vector2D& rhs) const noexcept
	{
		return *this > rhs || *this == rhs;
	}

	// for bool conversion
	explicit operator bool() const noexcept
	{
		return y != 0 || x != 0;
	}

	// for std::cout
	friend std::ostream& operator<<(std::ostream& os, const Vector2D& v)
	{
		os << "Vector2D: {y: " << v.y << ", x: " << v.x << "}";
		return os;
	}
};

namespace std {
	/* implement hash function so we can put GridLocation into an unordered_set */
	template <> struct hash<Vector2D> {
		std::size_t operator()(const Vector2D& id) const noexcept {
			// I wish built-in std::hash worked on pair and tuple
			return std::hash<int>()(id.x ^ (id.y << 16));
		}
	};
}