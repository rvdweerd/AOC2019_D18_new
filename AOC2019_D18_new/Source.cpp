#pragma once
#include "DataStructs.h"

void LoadField(const std::string filename, Field& field)
{
	std::string allKeys;
	std::ifstream in(filename);
	bool fieldWidthSet = false;
	while (!in.eof())
	{
		char ch;
		std::string str;
		int i = 0;
		for (ch = in.get(); !in.eof(); ch = in.get())
		{
			if (ch == '\n' )
			{
				if (!fieldWidthSet)
				{
					field.fieldWidth = i;
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
					field.startIndices[(ULL)ch] = (unsigned short)i;
				}
				if (IsKey(ch) || IsDoor(ch))
				{
					field.startIndices[(ULL)ch] = (unsigned short)i;
				}
				field.charVec.push_back(ch);
				i++;
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
	field.startKeyBitPos = { ULL('@')<<32 , 0 };
	return;
}
std::vector<unsigned short> GetAdjacentCells(const unsigned short posIndex, Field& field)
{
	static std::unordered_map<unsigned short, std::vector<unsigned short>> nborCellsMap;
	std::vector<unsigned short> outputVec;
	//auto itNeighbors = nborCellsMap[posIndex];
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
		unsigned short startIndex = field.startIndices[(char)startKey];
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
std::vector<BitPos> NewBitPos(const BitPos startBitPos, Field& field,int& count)
{
	static std::unordered_set<ULL> set;
	if (set.find(startBitPos.pos) == set.end())
	{
		ULL startKeyring = startBitPos.pos << 32 >> 32;
		std::vector<BitPos> returnVec;
		std::vector<std::pair<ULL, unsigned short>> borPOIs = GetNeighboringPOIs(startBitPos.pos >> 32, field);
		count++;
		for (std::pair<ULL, unsigned short> POI : borPOIs)
		{
			if (IsKey((char)POI.first))
			{
				unsigned short newSteps = startBitPos.nSteps + POI.second;
				returnVec.push_back({ (POI.first << 32) | startKeyring | (ULL(0b1) << (POI.first - ULL('a'))) , newSteps });
			}
			else // is Door
			{
				if (startKeyring & (ULL(0b1) << (POI.first - ULL('A')))) // You have the key to the door
				{
					unsigned short newSteps = startBitPos.nSteps + POI.second;
					returnVec.push_back({ (POI.first << 32) | startKeyring  , newSteps });
				}
			}
		}
		set.insert(startBitPos.pos);
		return returnVec; // a vector BitPositions for nearest reachable [doors that you can open] or [keys]
	}
	return {};
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
	std::map<ULL, unsigned short> fixed;
	BitPos start = field.startKeyBitPos;

	while (start.pos << 32 >> 32 != field.fullKeyring)
	{
		if (fixed.find(start.pos) == fixed.end())
		{
			fixed.insert({ start.pos , start.nSteps });
			std::vector<BitPos> newBitPos = NewBitPos(start, field,count);
			for (BitPos p : newBitPos)
			{
				if (fixed.find(p.pos) == fixed.end())
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
	Field field;
	LoadField("field.txt", field);
	float loadTime = ft.Mark();

	int count = 0;
	std::vector<BitPos> path = FindShortestPath(field,false,count);
	std::cout << "\nMETHOD 1: Number of steps to collect all keys: " << path.back().nSteps << ", with path " ;
	for (auto v : path) 
	{ 
		std::cout << char(v.pos>>32); 
	}
	std::cout << "\nLoad time: "<<loadTime<<", Execution time: " << ft.Mark() << '\n';
	std::cout << "\nDoor/key positions visited: " << count << '\n';

	std::cin.get();
	return 0;
}