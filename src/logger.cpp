#include "logger.h"

#include <chrono>
#include <stdexcept>

namespace
{
    std::string csv_escape(const std::string &s)
    {
        std::string out = "\"";
        for (char c : s)
        {
            if (c == '"')
            {
                out += "\"\"";
            }
            else
            {
                out += c;
            }
        }
        out += "\"";
        return out;
    }
}

CsvLogger::CsvLogger(const std::string &path)
{
    bool is_new = true;
    {
        std::ifstream probe(path);
        is_new = !probe.good();
    }

    file_.open(path, std::ios::app);
    if (!file_)
    {
        throw std::runtime_error("Failed to open log file: " + path);
    }

    if (is_new)
    {
        file_ << "timestamp_ms,region,text,confidence\n";
    }
}

void CsvLogger::log(const std::string &tag, const std::string &text, float confidence)
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    file_ << ms << "," << tag << "," << csv_escape(text) << "," << confidence << "\n";
    file_.flush();
}
