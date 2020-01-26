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
	field.startGridBitPos = { (((unsigned long long int)field.startIndex) << 32) , 0 } ;
	//field.startKeyBitPos = { (field.startGridBitPos.pos | (0b1 << 26)) , 0 };
	field.startKeyBitPos = { (field.startGridBitPos.pos ) , 0 };
	return;
}
std::vector<BitPos> GetNeighbors(BitPos& currBitPos, const Field& field, std::unordered_set<unsigned long long int>& visited)
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
				if (visited.find(newBitPos) == visited.end()) returnVec.push_back({ newBitPos, (unsigned short)(currBitPos.nSteps+1)}); // spawn
			}
		}
	}
	return returnVec;
}
std::vector<BitPos> GetNeighboringKeys(BitPos& startKey, const Field& field, std::unordered_set<unsigned long long int>& visited)
{
	std::queue<BitPos> queue;
	queue.push(startKey);
	visited.emplace(startKey.pos);
	std::vector<BitPos> returnVec;
	while (!queue.empty())
	{
		BitPos currKey = queue.front(); queue.pop();
		//std::cout << "Currrent position under evaluation (neighbor call is next):\n"; Print(currP, field);
		auto neighbors = GetNeighbors(currKey, field, visited);
		for (BitPos newBitPos : neighbors)
		{
			//Print(p,field);
			if (visited.find(newBitPos.pos) == visited.end())
			{
				if ( (newBitPos.pos<<32)>>32 != (startKey.pos<<32)>>32 )
				{

					char newkey = field.charVec[(unsigned short)(newBitPos.pos>>32)];
					std::vector<char> newpath = newBitPos.path; newpath.push_back(newkey);
					//newBitPos.pos <<= 32; newBitPos.pos >>= 32;
					//newBitPos.pos |= (unsigned long long int(0b1) << (newkey - 'a')) << 32;
					returnVec.push_back({ newBitPos.pos,newBitPos.nSteps,newpath});
				}
				else
				{
					queue.push(newBitPos);
					visited.emplace(newBitPos.pos);
					//visited.emplace(newBitPos.pos | ((long long int(newBitPos.nSteps)) << 48));
				}
			}
		}
	}
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
		auto neighbors = GetNeighbors(currBitPos, field, visited);
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
	//auto cmp = [](std::vector<BitPos> left, std::vector<BitPos> right) {return left.back().nSteps > right.back().nSteps; };
	//std::priority_queue<std::vector<BitPos>, std::vector<std::vector<BitPos>>, decltype(cmp)> queue(cmp);
	std::priority_queue<std::vector<BitPos>, std::vector<std::vector<BitPos>>, GroterePadKosten> queue;
	std::vector<BitPos> path;
	std::map<long long int, int> fixed;
	std::unordered_set<unsigned long long int> visitedOnGrid;
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
			visitedOnGrid.clear();
			std::vector<BitPos> nborKeys = GetNeighboringKeys(start, field, visitedOnGrid);
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
				//std::cout << ToBin(n.pos, 64) << '\n';
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


int main()
{
	FrameTimer ft;
	float dt = 0.0f;
	Field field;
	LoadField("field.txt", field);
	//std::unordered_set<unsigned long long int> visited;
	
	std::vector<BitPos> path = FindShortestPath(field, true);

	
	//std::vector<BitPos> newKeys = GetNeighboringKeys(field.startKeyBitPos, field, visited);
	










	
	//std::cout << "Load time: " << ft.Mark() << '\n';
	//std::pair<int,std::string> result = mazeBFS(field);
	//std::cout << "METHOD 1: Number of steps to collect all keys: " << result.first<<", with path "<<result.second ;
	//std::cout << "\nExecution time: " << ft.Mark() << '\n';
	//std::pair<unsigned short, std::string> result2 = mazeBFS_(field);
	//std::cout << "\nMETHOD 2: Number of steps to collect all keys: " << result2.first<<", with path "<<result2.second;
	//std::cout << "\nExecution time: " << ft.Mark() << '\n';

	std::cin.get();
	return 0;
}