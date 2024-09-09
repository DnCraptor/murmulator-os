#pragma once

class ifstream {

}; // TODO:

namespace util {

string read_file(const string & filename);
size_t islength(ifstream & is);
size_t filesize(const string & filename);
vector<uint8_t> load_binfile(const string path);
int save_binfile(const string& path_, const vector<uint8_t>& data);

// trim from start (in place)
static inline void ltrim(string& s) {
    s.ltrim();
}

// trim from end (in place)
static inline void rtrim(string& s) {
    s.rtrim();
}

// trim from both ends (in place)
static inline void trim(string& s)
{
    s.ltrim();
    s.rtrim();
}

// trim from start (copying)
static inline string ltrim_copy(string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline string rtrim_copy(string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline string trim_copy(string s)
{
    trim(s);
    return s;
}

void str_toupper(string & s);

string str_tolower_copy(string s);

///tuple<string, string, string> split_path(const string & path);

char printable_char(int c);

string tmpname(const string & basename);

int careful_rename(string const& from, string const& to);

}

template <typename T> inline T min(T x, T y) {
    return x > y ? y : x;
}
