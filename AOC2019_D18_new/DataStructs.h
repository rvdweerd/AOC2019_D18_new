#pragma once
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <chrono>

class FrameTimer
{
public:
	FrameTimer()
	{
		last = std::chrono::steady_clock::now();
	}
	float Mark()
	{
		const auto old = last;
		last = std::chrono::steady_clock::now();
		const std::chrono::duration<float> frameTime = last - old;
		return frameTime.count();
	}
private:
	std::chrono::steady_clock::time_point last;
};
struct BitPos
{
	unsigned long long int pos = 0;
	unsigned short nSteps = 0;
	std::vector<char> path;
	friend bool operator==(const BitPos& p1, const BitPos& p2)
	{
		return (p1.pos == p2.pos);
	}
	friend bool operator<(const BitPos& p1, const BitPos& p2)
	{
		return p1.pos < p2.pos;
	}
};
struct Field
{
	int fieldID = 0;
	int fieldWidth = 0;
	int fieldHeight = 0;
	int nKeys = 0;
	unsigned int fullKeyring = 0;
	std::string allKeys;
	std::vector<char> charVec;
	unsigned short startIndex = 0;
	BitPos startGridBitPos;
	BitPos startKeyBitPos;
	//Pawn startPos;
};
template <typename E>
std::string ToBin(E n, int min_digits = 0)
{
	std::string bin_str;
	for (int count = 0; n != 0 || count < min_digits; n >>= 1, count++)
	{
		bin_str.push_back(bool(n & 0b1) ? '1' : '0');
		if ((count+1)%8==0) bin_str.push_back('.');
	}
	std::reverse(bin_str.begin(), bin_str.end());
	bin_str = bin_str.substr(1,bin_str.size()-1);
	return bin_str;
}
void Print(std::queue<unsigned long long int> queue, Field field)
{
	while (!queue.empty())
	{

		unsigned short qpos = (queue.front() << 16) >> 48;
		queue.pop();
		field.charVec[qpos] = '$';
	}
	for (int i = 0; i < field.fieldWidth * field.fieldHeight; i++)
	{
		std::cout << field.charVec[i];
		if (i != 0 && ((i + 1) % (field.fieldWidth) == 0)) std::cout << '\n';
	}
}
bool IsKey(char c)
{
	return (c > 96 && c <= 122);
}
bool IsDoor(char c)
{
	return (c > 64 && c <= 90);
}