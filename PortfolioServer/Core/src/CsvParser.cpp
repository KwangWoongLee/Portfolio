#include "CorePch.h"
#include "CsvParser.h"
#include <fstream>
#include <sstream>

std::string const& CsvParser::Row::GetString(size_t const index) const
{
    return _columns.at(index);
}

int32_t CsvParser::Row::GetInt32(size_t const index) const
{
    return std::stoi(_columns.at(index));
}

int64_t CsvParser::Row::GetInt64(size_t const index) const
{
    return std::stoll(_columns.at(index));
}

uint32_t CsvParser::Row::GetUint32(size_t const index) const
{
    return static_cast<uint32_t>(std::stoul(_columns.at(index)));
}

float CsvParser::Row::GetFloat(size_t const index) const
{
    return std::stof(_columns.at(index));
}

bool CsvParser::Load(std::string const& filePath)
{
    std::ifstream file(filePath);
    if (not file.is_open())
    {
        std::cerr << "[CsvParser] Failed to open: " << filePath << std::endl;
        return false;
    }

    std::string line;

    // 첫 줄: 헤더
    if (not std::getline(file, line))
    {
        std::cerr << "[CsvParser] Empty file: " << filePath << std::endl;
        return false;
    }
    _headers = ParseLine(line);

    // 나머지: 데이터 행
    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        Row row;
        row._columns = ParseLine(line);
        _rows.push_back(std::move(row));
    }

    std::cout << "[CsvParser] Loaded " << filePath
        << " (" << _rows.size() << " rows)" << std::endl;

    return true;
}

std::vector<std::string> CsvParser::ParseLine(std::string const& line)
{
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string field;

    while (std::getline(ss, field, ','))
    {
        // 앞뒤 공백 제거
        auto const start = field.find_first_not_of(" \t\r");
        auto const end = field.find_last_not_of(" \t\r");

        if (std::string::npos == start)
        {
            result.emplace_back();
        }
        else
        {
            result.push_back(field.substr(start, end - start + 1));
        }
    }

    return result;
}
