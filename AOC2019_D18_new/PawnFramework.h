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

struct Pos
{
	int x = 0;
	int y = 0;
	friend bool operator==(const Pos& p1, const Pos& p2)
	{
		return (p1.x == p2.x) && (p1.y == p2.y);
	}
	friend bool operator<(const Pos& p1, const Pos& p2)
	{
		return (p1.x + p1.y) < (p2.x + p2.y);
	}
};
struct Pawn
{
	Pos pos = { 0,0 };
	int nSteps = 0;
	std::string keyPath;
	unsigned int keyring = 0b0;
};
struct KeyBit
{
	KeyBit()
	{
		for (int i = 0; i < 26; i++)
		{
			unsigned int mash = (0b01 << i);
			keyBitcodes[i + 'a'] = mash;
		}
	}
	std::map<char, unsigned int> keyBitcodes;
};
std::string Hash(const Pawn& pawn)
{
	std::string str;
	str += std::to_string(pawn.pos.x);
	str += '_';
	str += std::to_string(pawn.pos.y);
	str += '_';
	str += std::to_string(pawn.keyring);
	return str;
}
void Print(unsigned long long int pawn, const Field& field)
{
	std::cout << "pawn 64bit: " << '\n';
	std::cout << ToBin(pawn, 64) << '\n';
	std::cout << "keys: " << '\n';
	unsigned int keys = pawn;
	std::cout << ToBin(keys, 32) << '\n';
	std::cout << "Position: " << '\n';
	unsigned short posIndex = pawn >> 32;
	int y = posIndex / field.fieldWidth;
	int x = posIndex % field.fieldWidth;
	std::cout << ToBin(posIndex, 16) << '\n';
	std::cout << "x=" << x << ", y=" << y << '\n';
	std::cout << "Stepcount: " << '\n';
	unsigned short nSteps = pawn >> 48;
	std::cout << ToBin(nSteps, 16) << '\n';
	std::cout << "n=" << nSteps << "\n\n";
}
std::vector<Pawn> GetNeighbors(const Pawn& pawn, const Field& field, std::unordered_set<std::string>& visited)
{
	if (visited.find(Hash(pawn)) == visited.end()) return {};
	static KeyBit kb;
	std::vector<Pos> neighbors;
	if (pawn.pos.x > 0)						neighbors.push_back({ pawn.pos.x - 1,pawn.pos.y });
	if (pawn.pos.x < field.fieldWidth - 1)	neighbors.push_back({ pawn.pos.x + 1,pawn.pos.y });
	if (pawn.pos.y > 0)						neighbors.push_back({ pawn.pos.x    ,pawn.pos.y - 1 });
	if (pawn.pos.y < field.fieldHeight - 1)	neighbors.push_back({ pawn.pos.x    ,pawn.pos.y + 1 });
	std::vector<Pawn> returnVec;
	for (Pos n : neighbors)
	{
		char fieldChar = field.charVec[n.y * field.fieldWidth + n.x];
		if (fieldChar != '#')
		{
			if (fieldChar == '.' || fieldChar == '@')
			{
				returnVec.push_back({ n,pawn.nSteps + 1,pawn.keyPath,pawn.keyring });
			}
			else if (IsKey(fieldChar))
			{
				if (kb.keyBitcodes[fieldChar] & pawn.keyring)
				{
					returnVec.push_back({ n,pawn.nSteps + 1,pawn.keyPath,pawn.keyring });
				}
				else
				{
					returnVec.push_back({ n,pawn.nSteps + 1,pawn.keyPath + fieldChar,pawn.keyring | kb.keyBitcodes[tolower(fieldChar)] });
				}
			}
			else if (IsDoor(fieldChar) && (pawn.keyring & kb.keyBitcodes[tolower(fieldChar)]))
			{
				returnVec.push_back({ n,pawn.nSteps + 1,pawn.keyPath,pawn.keyring });
			}
		}
	}
	return returnVec;
}
std::pair<int, std::string> mazeBFS(const Field& field)
{
	std::queue<Pawn> queue;
	std::unordered_set<std::string> visited;
	queue.push(field.startPos);
	visited.emplace(Hash(field.startPos));
	while (!queue.empty())
	{
		Pawn currP = queue.front(); queue.pop();
		auto neighbors = GetNeighbors(currP, field, visited);
		for (Pawn p : neighbors)
		{
			if (visited.find(Hash(p)) == visited.end())
			{
				if (p.keyring == field.fullKeyring)
				{
					return { p.nSteps,p.keyPath };
				}
				else
				{
					queue.push(p);
					visited.emplace(Hash(p));
				}
			}
		}
	}
	return { -1,"error" };
}
