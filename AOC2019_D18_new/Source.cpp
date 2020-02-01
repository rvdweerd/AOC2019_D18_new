#pragma once
#include "DataStructs.h"

void LoadField(const std::string filename, Field& field)
{
	// Creat vector of useful bitmasks
	//ULL full = (ULL)std::numeric_limits<unsigned int>::max();
	for (unsigned short i = 0; i < 4; i++)
	{
		ULL erasor = std::numeric_limits<unsigned char>::max();
		erasor <<= (8u * i + 32u);
		//erasor ^= erasor;
		field.bitErasor_otherKeysPos.push_back(erasor | std::numeric_limits<size_t>::max());
		field.bitErasor_thisKeyPos.push_back(~erasor);
		field.bitErasor_otherKeysPosAndKeyRing.push_back(erasor);
	}
	for (auto e : field.bitErasor_thisKeyPos)
	{
		std::cout << ToBin(e, 64) << '\n';
	} std::cout << "---------------------------------------------------------\n";
	for (auto e : field.bitErasor_otherKeysPos)
	{
		std::cout << ToBin(e, 64) << '\n';
	} std::cout << "---------------------------------------------------------\n";
	for (auto e : field.bitErasor_otherKeysPosAndKeyRing)
	{
		std::cout << ToBin(e, 64) << '\n';
	} std::cout << "---------------------------------------------------------\n";

	std::string allKeys;
	std::ifstream in(filename);
	bool fieldWidthSet = false;
	char nStartPositions = '1';
	while (!in.eof())
	{
		char ch;
		std::string str;
		int fieldIndex = 0;
		unsigned short Quadrant = 0;
		for (ch = in.get(); !in.eof(); ch = in.get())
		{
			if (ch == '\n')
			{
				if (!fieldWidthSet)
				{
					field.fieldWidth = fieldIndex;
					fieldWidthSet = true;
				}
			}
			else
			{
				if (IsKey(ch))
				{
					allKeys += ch;
				}
				else if (ch == '@')
				{
					field.startIndices[(ULL)(nStartPositions++)] = { (unsigned short)fieldIndex, 0 };
					field.nPawns++;
				}
				if (IsKey(ch) || IsDoor(ch))
				{
					field.startIndices[(ULL)ch] = { (unsigned short)fieldIndex, Quadrant };
				}
				field.charVec.push_back(ch);
				fieldIndex++;
			}
		}
	}
	field.fieldHeight = field.charVec.size() / field.fieldWidth;
	std::sort(allKeys.begin(), allKeys.end());
	field.nKeys = allKeys.size();
	for (char c : allKeys)
	{
		field.fullKeyring |= (0b1 << (c - 'a'));
	}
	for (char i = '1'; i < nStartPositions; i++)
	{
		field.startKeyBitPos.pos = field.startKeyBitPos.pos | (ULL(i) << (32 + (i - '1') * 8)); // compose startstate in bit coding [char=@_Q3].[char=@_Q2].[char=@_Q1].[char=@_Q0].[int=keyring]
	}
	return;
}
std::vector<unsigned short> GetAdjacentCells(const unsigned short posIndex, Field& field)
{
	static std::unordered_map<unsigned short, std::vector<unsigned short>> nborCellsMap;
	std::vector<unsigned short> outputVec;
	if (nborCellsMap.find(posIndex) == nborCellsMap.end())
	{
		if (posIndex % field.fieldWidth != 0)
		{
			if (field.charVec[posIndex - 1] != '#') outputVec.push_back(posIndex - 1);
		}
		if (posIndex + 1 % field.fieldWidth != 0)
		{
			if (field.charVec[posIndex + 1] != '#') outputVec.push_back(posIndex + 1);
		}
		if (posIndex > field.fieldWidth)
		{
			if (field.charVec[posIndex - field.fieldWidth] != '#') outputVec.push_back(posIndex - field.fieldWidth);
		}
		if (posIndex < (field.fieldHeight - 1) * field.fieldWidth)
		{
			if (field.charVec[posIndex + field.fieldWidth] != '#') outputVec.push_back(posIndex + field.fieldWidth);
		}
		nborCellsMap[posIndex] = outputVec;
		return outputVec;
	}
	return nborCellsMap[posIndex]; // a vector of adjacent field indices
}
std::vector<std::pair<ULL, unsigned short>> GetNeighboringPOIs(const ULL startKey, Field& field)
{
	static std::unordered_map<ULL, std::vector<std::pair<ULL, unsigned short>>> nborPOIsMap;
	std::vector<std::pair<ULL, unsigned short>> outputVec;
	if (nborPOIsMap.find(startKey) == nborPOIsMap.end())
	{
		unsigned short startIndex = field.startIndices[(char)startKey].first;
		std::queue<std::pair<unsigned short,unsigned short>> queue;
		queue.push({ startIndex,0 });
		std::unordered_set<unsigned short> visited;
		visited.emplace(startIndex);
		while (!queue.empty())
		{
			unsigned short currPos = queue.front().first; unsigned short nSteps = queue.front().second; queue.pop();
			auto neighbors = GetAdjacentCells(currPos, field);
			for (unsigned short newPos : neighbors)
			{
				if (visited.find(newPos) == visited.end())
				{
					char ch = field.charVec[newPos];
					if (IsKey(ch) || IsDoor(ch))
					{
						outputVec.push_back({ ULL(ch),nSteps + 1 });
					}
					else
					{
						queue.push({ newPos,nSteps + 1 });
						visited.emplace(newPos);
					}
				}
			}
		}
		nborPOIsMap[startKey] = outputVec;
		return outputVec;
	}
	return nborPOIsMap[startKey];
}
std::vector<BitPos> NewBitPos(const BitPos& startBitPos, Field& field,int& count)
{
	static int cache0Used_count = 0;
	static int cache0NotUsed_count = 0;
	std::vector<BitPos> returnVec;
	ULL startKeyring = startBitPos.pos << 32 >> 32;
	std::vector<std::pair<ULL, unsigned short>> borPOIs = GetNeighboringPOIs(startBitPos.pos >> 32, field);
	for (std::pair<ULL, unsigned short> POI : borPOIs)
	{

		if (IsKey((char)POI.first))
		{
			unsigned short newSteps = startBitPos.nSteps + POI.second;
			returnVec.push_back({ (POI.first << 32) | startKeyring | (ULL(0b1) << (POI.first - ULL('a'))) , newSteps,startBitPos.lastPOIQuadrant });
		}
		else // is Door
		{
			if (startKeyring & (ULL(0b1) << (POI.first - ULL('A')))) // You have the key to the door
			{
				unsigned short newSteps = startBitPos.nSteps + POI.second;
				returnVec.push_back({ (POI.first << 32) | startKeyring  , newSteps ,startBitPos.lastPOIQuadrant });
			}
		}
	}
	return returnVec; // a vector BitPositions for nearest reachable [doors that you can open] or [keys]
}
std::vector<BitPos> NewBitPos4D(const BitPos& startBitPos, Field& field, int& count)
{
	std::vector<BitPos> returnVec;
	{
		for (unsigned short i = 0; i < field.nPawns; i++)
		{
			ULL nakedStartPos = startBitPos.pos & field.bitErasor_thisKeyPos[i];
			ULL currKey = (startBitPos.pos & field.bitErasor_otherKeysPosAndKeyRing[i]) >> (i * 8u);
			ULL currKeyRing = startBitPos.pos << 32u >> 32u;
			std::vector<BitPos> newBitPosInQuadrant = NewBitPos({ currKey | currKeyRing,startBitPos.nSteps,i }, field, count);
			for (auto e : newBitPosInQuadrant)
			{
				ULL newKey = e.pos >> 32u << (32u + i * 8u);
				ULL newKeyRing = e.pos << 32u >> 32u;
				BitPos newbitpos = { nakedStartPos | newKey | newKeyRing , e.nSteps , e.lastPOIQuadrant };
				returnVec.push_back(newbitpos);
				count++;
			}
		}
		return returnVec;
	}
}
struct GreaterPathCost
{
	bool operator()(const std::vector<BitPos>& lhs, const std::vector<BitPos>& rhs) const
	{
		return lhs.back().nSteps > rhs.back().nSteps;
	}
};
std::vector<BitPos> FindShortestPath(Field& field, bool debugMode,int& count)
{
	std::priority_queue<std::vector<BitPos>, std::vector<std::vector<BitPos>>, GreaterPathCost> queue;
	std::vector<BitPos> path;
	std::map<ULL, unsigned short> fixedComposite;
	BitPos start = field.startKeyBitPos;

	while (start.pos << 32 >> 32 != field.fullKeyring)
	{
		if (debugMode) { std::cout << "Startpos: \n"; PrintBin(start); }
		if (fixedComposite.find(start.pos) == fixedComposite.end())
		{
			fixedComposite.insert({ start.pos , start.nSteps });
			std::vector<BitPos> newBitPos = NewBitPos4D(start,field,count);
			for (BitPos p : newBitPos)
			{
				if (fixedComposite.find(p.pos) == fixedComposite.end())
				{
					path.push_back(p);
					queue.push(path);
					path.erase(path.end() - 1);
				}
			}
		}
		path = queue.top(); queue.pop();
		if (path.size() == 0)
		{
			return {};
		}
		start = path.back();
	}
	return path;
}

int main()
{
	FrameTimer ft;
	float dt = 0.0f;
	int count = 0;
	bool debugMode = false;
	Field field;
	LoadField("field.txt", field);
	float loadTime = ft.Mark();
		
	std::vector<BitPos> path = FindShortestPath(field, debugMode, count);
	std::cout << "\nMETHOD 1: Number of steps to collect all keys: " << path.back().nSteps << ", with path " ; PrintPath(path);
	std::cout << "\nLoad time: "<<loadTime<<", Execution time: " << ft.Mark() << '\n';
	std::cout << "\nDoor/key positions visited: " << count << '\n';

	std::cin.get();
	return 0;
}