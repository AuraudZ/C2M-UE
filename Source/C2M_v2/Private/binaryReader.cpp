#include "binaryReader.h"
#include <string>


binaryReader::binaryReader(std::string fileName)
{
	m_File = std::ifstream(fileName, std::ios::binary);
	m_File.seekg(0, std::ios::end);
	m_nSize = m_File.tellg();
	m_File.seekg(0, std::ios::beg);
	m_nPos = 0;
}


bool binaryReader::readBool()
{
	char buffer[1];
	m_File.seekg(m_nPos);
	m_File.read(buffer, 1);
	m_nPos += 1;
	bool value;
	std::memcpy(&value, buffer, 1);
	return value;
}

int binaryReader::readInt()
{
	char buffer[4];
	m_File.seekg(m_nPos);
	m_File.read(buffer, 4);
	m_nPos += 4;
	int value;
	value = (int&)buffer;
	return value;
}



uint8_t binaryReader::readUInt()
{
	char buffer[4];
	m_File.seekg(m_nPos);
	m_File.read(buffer, 4);
	m_nPos += 4;
	uint8_t value;
	std::memcpy(&value, buffer, 4);
	return value;
}


float binaryReader::readFloat()
{
	char buffer[4];
	m_File.seekg(m_nPos);
	m_File.read(buffer, 4);
	m_nPos += 4;
	float value;
	std::memcpy(&value, buffer, 4);
	return value;

}


char* binaryReader::readString()
{
	readByte();
	char* str = (char*)malloc(1);
	int n = 0;
	char* c = readByte();
	while (c[0] != '\x00')
	{
		str[n] = c[0];
		n++;
		str = (char*)realloc(str, n + 1);
		c = readByte();
	}
	str[n] = '\0';
	return str;

}

/*
char* binaryReader::readString()
{
	std::string& buffer = *(new std::string());
	auto& s = std::getline(m_File, buffer, '\0');
	m_nPos += buffer.size();
	return (char*)buffer.c_str();

} */

char* binaryReader::readByte() {

	char* buffer = new char[1];
	//m_File.seekg(m_nPos);
	m_File.read(buffer, 1);
	m_nPos++;
	return buffer;
}

std::vector<uint8_t> binaryReader::readBytes(size_t numBytes) {
	std::vector<uint8_t> buffer(numBytes);
	for (size_t i = 0; i < numBytes; i++) {
		m_File.seekg(m_nPos);
		buffer[i] = static_cast<uint8_t>(m_File.get());
		m_nPos++;
	}
	return buffer;
};

int binaryReader::getPos()
{
	return m_nPos;
}

int binaryReader::getSize()
{
	return m_nSize;
}

FVector binaryReader::readVector3()
{
	return FVector(readFloat(), readFloat(), readFloat());
}

FVector4 binaryReader::readVector4()
{
	return FVector4(readFloat(), readFloat(), readFloat(), readFloat());
}

void binaryReader::setPos(int nPos)
{
	m_nPos = nPos;
}

// Path: binaryReader.h