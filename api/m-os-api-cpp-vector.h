#pragma once

template<class T> class vector {
    size_t sz;
    size_t alloc;
    char* p;
public:
    inline vector(): sz(1), alloc(16), p(new char[16]) { p[0] = 0; }
    inline ~vector() { delete[] p; }
};
