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
struct BitPos
{
	unsigned long long int pos = 0;
	int nSteps = 0;
	//std::string path;
	friend bool operator==(const BitPos& p1, const BitPos& p2)
	{
		return (p1.pos == p2.pos);
	}
	friend bool operator<(const BitPos& p1, const BitPos& p2)
	{
		return p1.pos < p2.pos;
	}
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
				if (visited.find(newBitPos) == visited.end()) returnVec.push_back({ newBitPos, currBitPos.nSteps+1}); // spawn
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
std::pair<unsigned short, std::string> mazeBFS_(const Field& field)
{
	
	std::queue<BitPos> queue;
	std::unordered_set<unsigned long long int> visited;
	BitPos bitPos;
	unsigned long long int startIndex = (long long int)field.startPos.pos.y * (long long int)field.fieldWidth + (long long int)field.startPos.pos.x;
	bitPos.pos = (startIndex << 32);
	queue.push(bitPos);
	visited.emplace(bitPos.pos);
	while (!queue.empty())
	{
		//Print(queue, field);
		BitPos currBitPos = queue.front(); queue.pop();
		//std::cout << "Currrent position under evaluation (neighbor call is next):\n"; Print(currP, field);
		auto neighbors = GetNeighbors(currBitPos, field, visited);
		for (BitPos newBitPos : neighbors)
		{
			//Print(p,field);
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

int main()
{
	FrameTimer ft;
	float dt = 0.0f;
	Field field;
	LoadField("field.txt", field);
	std::cout << "Load time: " << ft.Mark() << '\n';

	//std::pair<int,std::string> result = mazeBFS(field);
	//std::cout << "METHOD 1: Number of steps to collect all keys: " << result.first<<", with path "<<result.second ;
	//std::cout << "\nExecution time: " << ft.Mark() << '\n';
	
	std::pair<unsigned short, std::string> result2 = mazeBFS_(field);
	std::cout << "\nMETHOD 2: Number of steps to collect all keys: " << result2.first<<", with path "<<result2.second;
	std::cout << "\nExecution time: " << ft.Mark() << '\n';

	std::cin.get();
	return 0;
}