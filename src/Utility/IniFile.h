#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

// Minimal INI file reader.
//
// Supported syntax:
//   ; comment          # comment
//   [section]
//   key = value        (whitespace around key/value is trimmed)
//   key = "quoted value"
//
// Keys that appear before any [section] belong to the "" (global) section.
// Section and key names are case-insensitive.
class IniFile
{
public:
    IniFile() = default;
    explicit IniFile(const std::string& path) { load(path); }

    // Returns true if the file was opened and parsed.
    bool load(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "IniFile: could not open '" << path << "'\n";
            return false;
        }

        m_data.clear();
        std::string section;
        std::string line;
        int lineNo = 0;

        while (std::getline(file, line))
        {
            ++lineNo;
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            // [section]
            if (line.front() == '[')
            {
                auto end = line.find(']');
                if (end == std::string::npos)
                {
                    std::cerr << "IniFile: " << path << ":" << lineNo
                        << ": missing ']' in section header\n";
                    continue;
                }
                section = toLower(trim(line.substr(1, end - 1)));
                continue;
            }

            // key = value
            auto eq = line.find('=');
            if (eq == std::string::npos)
            {
                std::cerr << "IniFile: " << path << ":" << lineNo
                    << ": expected 'key = value'\n";
                continue;
            }

            std::string key = toLower(trim(line.substr(0, eq)));
            std::string value = trim(line.substr(eq + 1));

            // Strip surrounding quotes if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);

            m_data[makeKey(section, key)] = value;
        }
        return true;
    }

    bool has(const std::string& section, const std::string& key) const
    {
        return m_data.count(makeKey(toLower(section), toLower(key))) > 0;
    }

    std::string getString(const std::string& section, const std::string& key,
        const std::string& defaultValue = "") const
    {
        auto it = m_data.find(makeKey(toLower(section), toLower(key)));
        return it != m_data.end() ? it->second : defaultValue;
    }

    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const
    {
        auto it = m_data.find(makeKey(toLower(section), toLower(key)));
        if (it == m_data.end()) return defaultValue;
        try { return std::stoi(it->second); } catch (...) { return defaultValue; }
    }

    float getFloat(const std::string& section, const std::string& key, float defaultValue = 0.0f) const
    {
        auto it = m_data.find(makeKey(toLower(section), toLower(key)));
        if (it == m_data.end()) return defaultValue;
        try { return std::stof(it->second); } catch (...) { return defaultValue; }
    }

    // Accepts: true/false, yes/no, on/off, 1/0 (case-insensitive)
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const
    {
        auto it = m_data.find(makeKey(toLower(section), toLower(key)));
        if (it == m_data.end()) return defaultValue;
        std::string v = toLower(it->second);
        if (v == "true" || v == "yes" || v == "on" || v == "1") return true;
        if (v == "false" || v == "no" || v == "off" || v == "0") return false;
        return defaultValue;
    }

private:
    std::map<std::string, std::string> m_data; // "section.key" -> value

    static std::string makeKey(const std::string& section, const std::string& key)
    {
        return section + "." + key;
    }

    static std::string trim(const std::string& s)
    {
        auto begin = std::find_if_not(s.begin(), s.end(),
            [](unsigned char c) { return std::isspace(c); });
        auto end = std::find_if_not(s.rbegin(), s.rend(),
            [](unsigned char c) { return std::isspace(c); }).base();
        return begin < end ? std::string(begin, end) : std::string();
    }

    static std::string toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }
};