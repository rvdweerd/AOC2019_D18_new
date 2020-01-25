#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>

struct Pos
{
	int x=0;
	int y=0;
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
struct Field
{
	int fieldID = 0;
	int fieldWidth = 0;
	int fieldHeight = 0;
	int nKeys = 0;
	unsigned int fullKeyring = 0;
	std::string allKeys;
	std::vector<char> charVec;
	Pawn startPos;
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
template <typename E>
std::string ToBin(E n, int min_digits = 0)
{
	std::string bin_str;
	for (int count = 0; n != 0 || count < min_digits; n >>= 1, count++)
	{
		bin_str.push_back(bool(n & 0b1) ? '1' : '0');
	}
	std::reverse(bin_str.begin(), bin_str.end());
	return bin_str;
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
		if (i!=0 && ((i+1) % (field.fieldWidth) == 0)) std::cout << '\n';
	}
}
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
bool IsKey(char c)
{
	return (c > 96 && c <= 122);
}
bool IsDoor(char c)
{
	return (c > 64 && c <= 90);
}
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
					int Y = i / field.fieldWidth;
					int X = i - Y * field.fieldWidth;
					field.startPos = { X,Y,0b0 };
				}
				field.charVec.push_back(ch);
				i++;
			}
		}
	}
	field.fieldHeight = field.charVec.size() / field.fieldWidth;
	std::sort(field.allKeys.begin(), field.allKeys.end());
	field.nKeys = field.allKeys.size();
	KeyBit kb;
	for (char c : field.allKeys)
	{
		field.fullKeyring |= kb.keyBitcodes[c];
	}
	return;
}
std::vector<Pawn> GetNeighbors(const Pawn& pawn, const Field& field, std::unordered_set<std::string>& visited)
{
	if (visited.find(Hash(pawn)) == visited.end()) return {};
	static KeyBit kb;
	std::vector<Pos> neighbors;
	if (pawn.pos.x > 0)						neighbors.push_back({ pawn.pos.x - 1,pawn.pos.y		});
	if (pawn.pos.x < field.fieldWidth-1)	neighbors.push_back({ pawn.pos.x + 1,pawn.pos.y		});
	if (pawn.pos.y > 0)						neighbors.push_back({ pawn.pos.x    ,pawn.pos.y-1	});
	if (pawn.pos.y < field.fieldHeight-1)	neighbors.push_back({ pawn.pos.x    ,pawn.pos.y+1	});
	std::vector<Pawn> returnVec;
	for (Pos n : neighbors)
	{
		char fieldChar = field.charVec[n.y * field.fieldWidth + n.x];
		if (fieldChar != '#')
		{
			if (fieldChar == '.' || fieldChar == '@')
			{
				returnVec.push_back({ n,pawn.nSteps+1,pawn.keyPath,pawn.keyring });
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
				returnVec.push_back({ n,pawn.nSteps+1,pawn.keyPath,pawn.keyring });
			}
		}
	}
	return returnVec;
}
std::vector<unsigned long long int> GetNeighbors(unsigned long long int& pawn, const Field& field, std::set<unsigned long long int>& visited)
{
	unsigned short pawnIndex = unsigned short((pawn << 16) >> 48);
	std::vector<unsigned short> neighbors;
	if (pawnIndex % field.fieldWidth != 0)	neighbors.push_back(pawnIndex-1);
	if (pawnIndex+1 % field.fieldWidth !=0)	neighbors.push_back(pawnIndex+1);
	if (pawnIndex > field.fieldWidth )		neighbors.push_back(pawnIndex - field.fieldWidth);
	if (pawnIndex < (field.fieldHeight-1)*field.fieldWidth)	neighbors.push_back(pawnIndex + field.fieldWidth);
	std::vector<unsigned long long int> returnVec;
	for (unsigned short n : neighbors)
	{
		char fieldChar = field.charVec[n];
		//unsigned int keys = (int)((pawn << 32) >> 32);
		if (fieldChar != '#')
		{
			unsigned int keys = (int)((pawn << 32) >> 32);
			if (fieldChar == '.' || fieldChar == '@')
			{
				unsigned long long int e = (pawn >> 48)+1; // add one step
				e = (e << 16) | n;
				e = (e << 32) | keys;
				if (visited.find((e << 16) >> 16) == visited.end())	returnVec.push_back(e);
			}
			else if (IsKey(fieldChar))
			{
				keys = keys | 0b1 << (fieldChar - 'a');
				unsigned long long int e = (pawn >> 48) + 1; // add one step
				e = (e << 16) | n;
				e = (e << 32) | keys;
				if (visited.find((e << 16) >> 16) == visited.end()) returnVec.push_back(e);
			}
			else if (IsDoor(fieldChar) && (keys & 0b1<<(fieldChar - 'A') ))
			{
				unsigned long long int e = (pawn >> 48) + 1; // add one step
				e = (e << 16) | n;
				e = (e << 32) | keys;
				if (visited.find((e << 16) >> 16) == visited.end()) returnVec.push_back(e);
			}
		}
	}
	return returnVec;
}
std::pair<int,std::string> mazeBFS(const Field& field)
{
	std::queue<Pawn> queue;
	std::unordered_set<std::string> visited;
	queue.push(field.startPos);
	visited.emplace(Hash(field.startPos));
	while (!queue.empty())
	{
		Pawn currP = queue.front(); queue.pop();
		auto neighbors = GetNeighbors(currP, field,visited);
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
unsigned short mazeBFS_(const Field& field)
{
	std::queue<unsigned long long int> queue;
	std::set<unsigned long long int> visited;
	unsigned long long int start = (long long int)field.startPos.pos.y * (long long int)field.fieldWidth + (long long int)field.startPos.pos.x;
	start = (start << 32);
	queue.push(start);
	{
		unsigned long long int tmp = (start << 16) >> 16;
		visited.emplace(tmp);
	}
	while (!queue.empty())
	{
		Print(queue, field);
		unsigned long long int currP = queue.front(); queue.pop();
		std::cout << "Currrent position under evaluation (neighbor call is next):\n"; Print(currP, field);
		auto neighbors = GetNeighbors(currP, field, visited);
		for (unsigned long long int p : neighbors)
		{
			Print(p,field);
			if (visited.find((p<<16)>>16) == visited.end())
			{
				unsigned int test = ((p << 32) >> 32);
				if (unsigned int((p<<32)>>32) == field.fullKeyring)
				{
					return (unsigned short)(p>>48);
				}
				else
				{
					queue.push(p);
					visited.emplace((p<<16)>>16);
				}
			}
		}
	}

	return -1;
}

int main()
{
	Field field;
	LoadField("field.txt", field);
	//std::pair<int,std::string> result = mazeBFS(field);
	//std::cout << "Number of steps to collect all keys: " << result.first<<", with path "<<result.second ;
	unsigned short result = mazeBFS_(field);
	std::cout << "Number of steps to collect all keys: " << result;
	std::cin.get();
	return 0;
}