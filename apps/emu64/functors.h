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
    unsigned char operator()(unsigned short a) { return h ? (h->*fn)(a) : 0; }
    template<typename T2> ReadProcFn operator=(const ReadProcFn<T2>& c) {
        *this = *(ReadProcFn<T>*)&c;
        return *this;
    }
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
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
    void operator()(unsigned short a, unsigned char x) { if(h) (h->*fn)(a, x); }
    template<typename T2> WriteProcFn operator=(const WriteProcFn<T2>& c) {
        *this = *(WriteProcFn<T>*)&c;
        return *this;
    }
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
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
    void operator()(uint8_t* b) { if (h) (h->*fn)(b); }
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
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
    void operator()(void) { if (h) (h->*fn)(); }
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
};

template<typename T> class VCProcFn {
public:
    typedef void (T::*fnProc)(unsigned char);
private:
    fnProc fn;
    T* h;
public:
    VCProcFn(void) : fn(NULL), h(NULL) {}
    VCProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    VCProcFn(const VCProcFn& c) : fn(c.fn), h(c.h) {}
    VCProcFn& operator=(const VCProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    void operator()(unsigned char x) { if (h) (h->*fn)(x); }
};

template<typename T> class BVProcFn {
public:
    typedef bool (T::*fnProc)(void);
private:
    fnProc fn;
    T* h;
public:
    BVProcFn(void) : fn(NULL), h(NULL) {}
    BVProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    BVProcFn(const BVProcFn& c) : fn(c.fn), h(c.h) {}
    BVProcFn& operator=(const BVProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    bool operator()(void) { return h ? (h->*fn)() : false; }
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
};

template<typename T> class CVProcFn {
public:
    typedef unsigned char (T::*fnProc)(void);
private:
    fnProc fn;
    T* h;
public:
    CVProcFn(void) : fn(NULL), h(NULL) {}
    CVProcFn(fnProc fn, T* h) : fn(fn), h(h) {}
    CVProcFn(const CVProcFn& c) : fn(c.fn), h(c.h) {}
    CVProcFn& operator=(const CVProcFn& c) {
        fn = c.fn;
        h = c.h;
        return *this;
    }
    unsigned char operator()(void) { return h ? (h->*fn)() : 0; }
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
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
    bool operator==(void* i) { return h == i; }
    bool operator!=(void* i) { return h != i; }
};

#endif
