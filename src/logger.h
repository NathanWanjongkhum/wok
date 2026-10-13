#pragma once
#include <fstream>
#include <string>

// Append-only CSV log: timestamp_ms,region,text,confidence.
// Writes a header row only when the file didn't already exist.
class CsvLogger
{
public:
    explicit CsvLogger(const std::string &path);

    void log(const std::string &tag, const std::string &text, float confidence);

private:
    std::ofstream file_;
};
