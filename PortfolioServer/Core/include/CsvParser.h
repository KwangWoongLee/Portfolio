#pragma once
#include "CorePch.h"

class CsvParser final
{
public:
    struct Row final
    {
        std::vector<std::string> _columns;

        std::string const& GetString(size_t const index) const;
        int32_t GetInt32(size_t const index) const;
        int64_t GetInt64(size_t const index) const;
        uint32_t GetUint32(size_t const index) const;
        float GetFloat(size_t const index) const;
    };

    bool Load(std::string const& filePath);

    std::vector<std::string> const& GetHeaders() const { return _headers; }
    std::vector<Row> const& GetRows() const { return _rows; }
    size_t GetRowCount() const { return _rows.size(); }

private:
    static std::vector<std::string> ParseLine(std::string const& line);

    std::vector<std::string> _headers;
    std::vector<Row> _rows;
};
