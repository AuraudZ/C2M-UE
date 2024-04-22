#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <Math/Vector.h>
#include <Math/Vector4.h>	
class binaryReader
{
	const char* m_pBuffer;
	std::ifstream m_File;
	int m_nSize;
	int m_nPos;
public:
	binaryReader(std::string fileName);
	int readInt();
	uint8_t readUInt();
	bool readBool();
	float readFloat();
	char* readString();
	char* readByte();
	std::vector<uint8_t> readBytes(size_t numBytes);
	int getPos();
	int getSize();
	FVector readVector3();
	FVector4 readVector4();
	void setPos(int nPos);


};