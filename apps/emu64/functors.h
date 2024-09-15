#ifndef FUNCTORS
#define FUNCTORS
template<typename T> class ReadProcFn {
public:
    typedef unsigned char (T::*fnProc)(unsigned short);
private:
    fnProc fn;
    T* h;
public:
    ReadProcFn(void) : fn(NULL), h(NULL) {}
    ReadProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    ReadProcFn(const ReadProcFn& c) : fn(c.fn), h(c.h) {}
    ReadProcFn& operator=(const ReadProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    unsigned char operator()(unsigned short a) { return (h->*fn)(a); }
    template<typename T2> ReadProcFn operator=(const ReadProcFn<T2>& c) {
        *this = *(ReadProcFn<T>*)&c;
        return *this;
    }
};

template<typename T> class WriteProcFn {
public:
    typedef void (T::*fnProc)(unsigned short,unsigned char);
private:
    fnProc fn;
    T* h;
public:
    WriteProcFn(void) : fn(NULL), h(NULL) {}
    WriteProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    WriteProcFn(const WriteProcFn& c) : fn(c.fn), h(c.h) {}
    WriteProcFn& operator=(const WriteProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(unsigned short a, unsigned char x) { (h->*fn)(a, x); }
    template<typename T2> WriteProcFn operator=(const WriteProcFn<T2>& c) {
        *this = *(WriteProcFn<T>*)&c;
        return *this;
    }
};

template<typename T> class RefreshProcFn {
public:
    typedef void (T::*fnProc)(uint8_t*);
private:
    fnProc fn;
    T* h;
public:
    RefreshProcFn(void) : fn(NULL), h(NULL) {}
    RefreshProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    RefreshProcFn(const RefreshProcFn& c) : fn(c.fn), h(c.h) {}
    RefreshProcFn& operator=(const RefreshProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(uint8_t* b) { (h->*fn)(b); }
};

template<typename T> class VVProcFn {
public:
    typedef void (T::*fnProc)(void);
private:
    fnProc fn;
    T* h;
public:
    VVProcFn(void) : fn(NULL), h(NULL) {}
    VVProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    VVProcFn(const VVProcFn& c) : fn(c.fn), h(c.h) {}
    VVProcFn& operator=(const VVProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(void) { (h->*fn)(); }
};

template<typename T> class VIProcFn {
public:
    typedef void (T::*fnProc)(int);
private:
    fnProc fn;
    T* h;
public:
    VIProcFn(void) : fn(NULL), h(NULL) {}
    VIProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    VIProcFn(const VIProcFn& c) : fn(c.fn), h(c.h) {}
    VIProcFn& operator=(const VIProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(int x) { (h->*fn)(x); }
};

#endif
