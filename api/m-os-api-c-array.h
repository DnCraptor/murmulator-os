// array of pointers to some objects
typedef void (*dealloc_fn_ptr_t)(void*);
typedef void* (*alloc_fn_ptr_t)(void);
typedef size_t (*size_fn_ptr_t)(void*);
typedef struct array {
    alloc_fn_ptr_t allocator;
    dealloc_fn_ptr_t deallocator;
    size_fn_ptr_t size_fn; // to ask each element
    void* *p; // pointer to array of pointers
    size_t alloc; // in bytes
    size_t size; // in elements
} array_t;

inline static array_t* new_array_v(alloc_fn_ptr_t allocator, dealloc_fn_ptr_t deallocator, size_fn_ptr_t size_fn) {
    array_t* res = malloc(sizeof(array_t));
    res->allocator = allocator ? allocator : malloc;
    res->deallocator = deallocator ? deallocator : free;
    res->size_fn = size_fn;
    res->alloc = 10 * sizeof(void*);
    res->p = malloc(res->alloc);
    res->size = 0;
    return res;
}

inline static void delete_array(array_t* arr) {
    for (size_t i = 0; i < arr->size; ++i) {
        arr->deallocator(arr->p[i]);
    }
    free(arr->p);
    free(arr);
}

inline static void array_reserve(array_t* arr, size_t sz) { // sz - in bytes
    if (sz <= arr->alloc)  return;
    uint8_t* p = malloc(sz);
    memcpy(p, arr->p, arr->alloc);
    free(arr->p);
    arr->p = p;
    arr->alloc = sz;
}

inline static void array_push_back(array_t* arr, const void* data) {
    register size_t min_sz_bytes = (arr->size + 1) * sizeof(void*);
    if (min_sz_bytes > arr->alloc) {
        array_reserve(arr, min_sz_bytes);
    }
    arr->p[arr->size] = data;
    ++arr->size;
}

inline static void* array_get_at(array_t* arr, size_t n) { // by element number
    return arr->p[n];
}

inline static void* array_resize(array_t* arr, size_t n) {
    if (n == arr->size) return;
    register size_t min_sz_bytes = (arr->size + 1) * sizeof(void*);
    if (min_sz_bytes > arr->alloc) {
        array_reserve(arr, min_sz_bytes);
    }
    if (n < arr->size) {
        for (size_t i = n; i < arr->size; ++i) {
            arr->deallocator(arr->p[i]);
        }
    } else {
        for (size_t i = arr->size; i < n; ++i) {
            arr->p[i] = arr->allocator();
        }
    }
    arr->size = n;
}
