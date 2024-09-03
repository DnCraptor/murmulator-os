#ifndef ALLOCATOR_H
#define ALLOCATOR_H

struct allocator {
inline void* operator new(size_t sz) {
    void* r = calloc(1, sz);
    goutf("new(%d): [%p]\n", sz, r);
    return r;
}
inline void operator delete(void* p) {
    gouta("delete\n");
    return free(p);
}
inline void operator delete(void* p, size_t) {
    gouta("delete\n");
    return free(p);
}
inline void* operator new[](size_t sz) {
    gouta("new[]\n");
    return calloc(1, sz);
}
inline void operator delete[](void* p) {
    gouta("delete[]\n");
    return free(p);
}
inline void operator delete[](void* p, size_t) {
    gouta("delete[]\n");
    return free(p);
}
};

#endif
