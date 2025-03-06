#include <precompiled/engine_precompiled.hpp>
#include "tools/tools.hpp"

using namespace std;

// Courtesy of: http://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
string bee::StringReplace(const string& subject, const string& search, const string& replace)
{
    string result(subject);
    size_t pos = 0;

    while ((pos = subject.find(search, pos)) != string::npos)
    {
        result.replace(pos, search.length(), replace);
        pos += search.length();
    }

    return result;
}

bool bee::StringEndsWith(const string& subject, const string& suffix)
{
    // Early out test:
    if (suffix.length() > subject.length()) return false;

    // Resort to difficult to read C++ logic:
    return subject.compare(subject.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool bee::StringStartsWith(const string& subject, const std::string& prefix)
{
    // Early out, prefix is longer than the subject:
    if (prefix.length() > subject.length()) return false;

    // Compare per character:
    for (size_t i = 0; i < prefix.length(); ++i)
        if (subject[i] != prefix[i]) return false;

    return true;
}

std::vector<std::string> bee::SplitString(const std::string& input, const std::string& delim)
{
    std::vector<std::string> result;
    size_t pos = 0, pos2 = 0;
    while ((pos2 = input.find(delim, pos)) != std::string::npos)
    {
        result.push_back(input.substr(pos, pos2 - pos));
        pos = pos2 + 1;
    }

    result.push_back(input.substr(pos));

    return result;
}

float bee::GetRandomNumber(float min, float max, int decimals)
{
    int p = (int)pow(10, decimals);
    int imin = static_cast<int>(min) * p;
    int imax = static_cast<int>(max) * p;

    int irand = imin + rand() % (imax - imin);
    float val = (float)irand / p;
    return val;
}