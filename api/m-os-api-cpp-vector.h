#pragma once

template<typename T> class vector {
    size_t sz;
    size_t alloc;
    T* p;
public:
    inline vector(void): sz(0), alloc(16), p(new T[16]) { }
    inline vector(size_t al): sz(0), alloc(al), p(new T[al]) { }
    inline vector(size_t sz, T init_by): sz(sz), alloc(sz), p(new T[sz]) {
        for (size_t i = 0; i < sz; ++i) {
            p[i] = init_by;
        }
    }
    inline ~vector(void) { delete[] p; }
    inline size_t size(void) const { return this->sz; }
    inline const T& operator[](size_t a) const { return p[a]; }
    inline T& operator[](size_t a) { return p[a]; }
    inline const T* data(void) const { return p;}
    inline T* data(void) { return p;}
    inline void clear(void) { sz = 0; }
    class iterator {};
};
