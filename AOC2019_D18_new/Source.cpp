#pragma once
#include "DataStructs.h"

void LoadField(const std::string filename, Field& field)
{
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
					field.allKeys += ch;
				}
				else if (ch == '@')
				{
					//int Y = i / field.fieldWidth;
					//int X = i - Y * field.fieldWidth;
					field.startIndex = (unsigned short)i;
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
	std::sort(field.allKeys.begin(), field.allKeys.end());
	field.nKeys = field.allKeys.size();
	for (char c : field.allKeys)
	{
		field.fullKeyring |= (0b1 << (c - 'a'));
	}
	field.startGridBitPos = { ULL(field.startIndex) << 32 , 0 } ;
	field.startKeyBitPos = { ULL('@')<<32 , 0 };
	return;
}
std::vector<BitPos> GetNeighbors(BitPos& currBitPos, const Field& field)
{
	static std::unordered_map<unsigned short, std::vector<unsigned short>> nMap;
	unsigned short posIndex = unsigned short(currBitPos.pos >> 32);
	auto itNeighbors = nMap[posIndex];
	if (itNeighbors.size() == 0)
	{
		if (posIndex % field.fieldWidth != 0)
		{
			if (field.charVec[posIndex - 1] != '#') itNeighbors.push_back(posIndex - 1);
		}
		if (posIndex + 1 % field.fieldWidth != 0)
		{
			if (field.charVec[posIndex + 1] != '#') itNeighbors.push_back(posIndex + 1);
		}
		if (posIndex > field.fieldWidth)
		{
			if (field.charVec[posIndex - field.fieldWidth ] != '#') itNeighbors.push_back(posIndex - field.fieldWidth);
		}
		if (posIndex < (field.fieldHeight - 1) * field.fieldWidth)
		{
			if (field.charVec[posIndex + field.fieldWidth] != '#') itNeighbors.push_back(posIndex + field.fieldWidth);
		}
	}
	std::vector<BitPos> returnVec;
	for (unsigned short nbor : itNeighbors)
	{
		char fieldChar = field.charVec[nbor];
		//std::string newKey;
		{
			unsigned int keys = (int)((currBitPos.pos << 32) >> 32);
			if (IsDoor(fieldChar) && !(keys & 0b1 << (fieldChar - 'A'))) // If it's a door and you don't have the key, don't spawn
			{}
			else
			{
				if (IsKey(fieldChar) && ((keys & (0b1 << (fieldChar - 'a')))==0) )
				{
					//newKey += fieldChar;
					//currBitPos.path += fieldChar;
					//std::cout << fieldChar;
					keys = keys | 0b1 << (fieldChar - 'a'); // add the key to the keyring
				}
				unsigned long long int newBitPos = nbor;
				newBitPos = (newBitPos << 32) | keys;
				returnVec.push_back({ newBitPos, (unsigned short)(currBitPos.nSteps+1)}); // spawn
			}
		}
	}
	return returnVec;
}
std::vector<BitPos> GetNeighboringKeys(BitPos& startKey, const Field& field)
{
	static std::map<unsigned long long int, std::vector<BitPos>> cache;
	if (cache.find(startKey.pos) != cache.end())
	{
		std::cout << "cache collision.";
	}
	std::queue<BitPos> queue;
	queue.push(startKey);
	std::unordered_set<unsigned long long int> visited;
	visited.emplace(startKey.pos);
	std::vector<BitPos> returnVec;
	while (!queue.empty())
	{
		BitPos currKey = queue.front(); queue.pop();
		auto neighbors = GetNeighbors(currKey, field);
		for (BitPos newBitPos : neighbors)
		{
			if (visited.find(newBitPos.pos) == visited.end())
			{
				if ( (newBitPos.pos<<32)>>32 != (startKey.pos<<32)>>32 )
				{

					char newkey = field.charVec[(unsigned short)(newBitPos.pos>>32)];
					std::vector<char> newpath = newBitPos.path; newpath.push_back(newkey);
					returnVec.push_back({ newBitPos.pos,newBitPos.nSteps,newpath});
				}
				else
				{
					queue.push(newBitPos);
					visited.emplace(newBitPos.pos);
				}
			}
		}
	}
	cache[startKey.pos] = returnVec;
	return returnVec;
}
std::pair<unsigned short, std::string> mazeBFS_(const Field& field)
{
	std::queue<BitPos> queue;
	std::unordered_set<unsigned long long int> visited;
	queue.push(field.startGridBitPos);
	visited.emplace(field.startGridBitPos.pos);
	while (!queue.empty())
	{
		BitPos currBitPos = queue.front(); queue.pop();
		auto neighbors = GetNeighbors(currBitPos, field);
		for (BitPos newBitPos : neighbors)
		{
			if (visited.find(newBitPos.pos) == visited.end())
			{
				if (unsigned int((newBitPos.pos<<32)>>32) == field.fullKeyring)
				{
					return { (unsigned short)(newBitPos.nSteps),{} };
				}
				else
				{
					queue.push(newBitPos);
					visited.emplace(newBitPos.pos);
				}
			}
		}
	}
	return { -1,"error!" };
}
struct GroterePadKosten
{
	// Dit is een compare function die gebruikt wordt voor het sorteren (prioriteren) in de priority_map class voor de toepassing 
	// in het Dijkstra algorithme
	bool operator()(const std::vector<BitPos>& lhs, const std::vector<BitPos>& rhs) const
	{
		return lhs.back().nSteps > rhs.back().nSteps;
	}
};
std::vector<BitPos> FindShortestPath(Field& field,bool debugMode)
{
	std::priority_queue<std::vector<BitPos>, std::vector<std::vector<BitPos>>, GroterePadKosten> queue;
	std::vector<BitPos> path;
	std::map<long long int, int> fixed;
	BitPos start = field.startKeyBitPos;
	if (debugMode)
	{
		std::cout << "Start : " << ToBin(start.pos, 64) << '\n';
		std::cout << "Finish: " << ToBin(field.fullKeyring, 64) << '\n';
	}
	while ((unsigned int)((start.pos << 32) >> 32) != field.fullKeyring)
	{
		if (debugMode)
		{
			std::cout << "\nNew start BitPosition in Dijkstra algo: \n";
			std::cout << "------->" << ToBin(start.pos, 64) << ", n=" << start.nSteps << '\n';
		}
		if (fixed.find(start.pos) == fixed.end())
		{
			fixed[start.pos] = start.nSteps;
			std::vector<BitPos> nborKeys = GetNeighboringKeys(start, field);
			if (debugMode)
			{
				std::cout << "All key neighbor BitPositions: " << '\n';
				for (BitPos n : nborKeys)
				{
					std::cout << "------->" << ToBin(n.pos, 64) << ", n=" << n.nSteps << '\n';
				}
			}
			for (BitPos n : nborKeys)
			{
				if (fixed.find(n.pos) == fixed.end())
				{
					path.push_back(n);
					queue.push(path);
					path.erase(path.end() - 1);
				}
			}
		}
		if (queue.empty())
		{
			return {};
		}
		path = queue.top(); queue.pop();
		if (debugMode)
		{
			std::cout << "Path vector taken from top of priority queue:\n";
			for (BitPos n : path)
			{
				std::cout << "------->" << ToBin(n.pos, 64) << ", n=" << n.nSteps << '\n';
			}
		}
		start = path[path.size() - 1];
	}

	return path;
}


std::vector<unsigned short> GetAdjacentCells(const unsigned short posIndex, Field& field)
{
	//static std::unordered_map<unsigned short, std::vector<unsigned short>> nborCellsMap;
	static std::unordered_map<unsigned short, std::vector<unsigned short>> nborCellsMap;
	auto itNeighbors = nborCellsMap[posIndex];
	if (itNeighbors.size() == 0)
	{
		if (posIndex % field.fieldWidth != 0)
		{
			if (field.charVec[posIndex - 1] != '#') itNeighbors.push_back(posIndex - 1);
		}
		if (posIndex + 1 % field.fieldWidth != 0)
		{
			if (field.charVec[posIndex + 1] != '#') itNeighbors.push_back(posIndex + 1);
		}
		if (posIndex > field.fieldWidth)
		{
			if (field.charVec[posIndex - field.fieldWidth] != '#') itNeighbors.push_back(posIndex - field.fieldWidth);
		}
		if (posIndex < (field.fieldHeight - 1) * field.fieldWidth)
		{
			if (field.charVec[posIndex + field.fieldWidth] != '#') itNeighbors.push_back(posIndex + field.fieldWidth);
		}
		nborCellsMap[posIndex] = itNeighbors;
	}
	//std::vector<unsigned short> v= itNeighbors;
	//nborCellsMap[posIndex]=itNeighbors;
	return itNeighbors; // a vector of adjacent field indices
}
std::vector<std::pair<ULL, unsigned short>> GetNeighboringPOIs(ULL startKey, Field& field)
{
	static std::unordered_map<ULL, std::vector<std::pair<ULL, unsigned short>>> nborPOIsMap;
	auto itNeighbors = nborPOIsMap[startKey];
	if (itNeighbors.size() == 0)
	{
		unsigned short startIndex = field.startIndices[startKey];
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
						itNeighbors.push_back({ ULL(ch),nSteps + 1 });
					}
					else
					{
						queue.push({ newPos,nSteps + 1 });
						visited.emplace(newPos);
					}
				}
			}
		}
		nborPOIsMap[startKey] = itNeighbors;
	}
	return itNeighbors; // a vector of pairs containing: (1) ASCII value (in ULL) of all reachable keys and doors from startKey (2) nSteps to these
}
std::vector<BitPos> NewBitPos(BitPos startBitPos, Field& field,int& count)
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
std::vector<BitPos> FindShortestPath2(Field& field, bool debugMode,int& count)
{
	std::priority_queue<std::vector<BitPos>, std::vector<std::vector<BitPos>>, GroterePadKosten> queue;
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

	int count = 0;
	std::vector<BitPos> path = FindShortestPath2(field,false,count);
	std::cout << "\nMETHOD 1: Number of steps to collect all keys: " << path.back().nSteps << ", with path " ;
	for (auto v : path) 
	{ 
		std::cout << char(v.pos>>32); 
	}
	std::cout << "\nExecution time: " << ft.Mark() << '\n';
	std::cout << "\nDoor/key positions visited: " << count << '\n';


	//std::vector<BitPos> path = FindShortestPath(field, false);
	//std::cout << "\nMETHOD 1: Number of steps to collect all keys: " << path.back().nSteps << ", with path " ;
	//for (auto v : path) 
	//{ 
	//	std::cout << v.path[0]; 
	//}
	//std::cout << "\nExecution time: " << ft.Mark() << '\n';

	//	
	//std::pair<unsigned short, std::string> result2 = mazeBFS_(field);
	//std::cout << "\nMETHOD 2: Number of steps to collect all keys: " << result2.first<<", with path "<<result2.second;
	//std::cout << "\nExecution time: " << ft.Mark() << '\n';

	std::cin.get();
	return 0;
}