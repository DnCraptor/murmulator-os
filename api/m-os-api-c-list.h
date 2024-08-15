typedef struct node {
    struct node* prev;
    struct node* next;
    void* data;
} node_t;

typedef void (*dealloc_fn_ptr_t)(void*);
typedef void* (*alloc_fn_ptr_t)(void);
typedef size_t (*size_fn_ptr_t)(void*);
typedef struct list {
    alloc_fn_ptr_t allocator;
    dealloc_fn_ptr_t deallocator;
    size_fn_ptr_t size_fn;
    node_t* first;
    node_t* last;
    size_t size;
} list_t;

inline static list_t* new_list_v(alloc_fn_ptr_t allocator, dealloc_fn_ptr_t deallocator, size_fn_ptr_t size_fn) {
    list_t* res = malloc(sizeof(list_t));
    res->allocator = allocator ? allocator : malloc;
    res->deallocator = deallocator ? deallocator : free;
    res->size_fn = size_fn;
    res->first = NULL;
    res->last = NULL;
    res->size = 0;
    return res;
}

inline static void list_push_back(list_t* lst, void* s) {
    node_t* i = malloc(sizeof(node_t));
    i->prev = lst->last;
    i->next = NULL;
    i->data = s;
    if (lst->first == NULL) lst->first = i;
    if (lst->last != NULL) lst->last->next = i;
    lst->last = i;
    ++lst->size;
}

inline static size_t list_data_bytes(list_t* lst) {
    if (!lst->size_fn) return 0; // TODO: some default impl.
    size_t res = 0;
    node_t* i = lst->last;
    while(i) {
        node_t* prev = i->prev;
        if (i->data) {
            res += lst->size_fn(i->data);
        }
        i = prev;
    }
    return res;
}

inline static void delete_list(list_t* lst) {
    node_t* i = lst->last;
    while(i) {
        node_t* prev = i->prev;
        if (i->data) {
            lst->deallocator(i->data);
        }
        free(i);
        i = prev;
    }
    free(lst);
}

inline static node_t* list_inject_till(list_t* lst, size_t idx) {
    node_t* i = lst->first;
    size_t n = 0;
    while (i) {
        if (!i->next) {
            list_push_back(lst, lst->allocator());
        }
        if (n == idx) {
            return i;
        }
        i = i->next;
        ++n;
    }
    // unreachable
    return NULL;
}

inline static node_t* list_get_node_at(list_t* lst, size_t idx) {
    node_t* i = lst->first;
    size_t n = 0;
    while (i) {
        if (n == idx) {
            return i;
        }
        i = i->next;
        ++n;
    }
    return NULL;
}

inline static size_t list_count(list_t* lst) {
    return lst->size;
}

inline static void* list_get_data_at(list_t* lst, size_t idx) {
    node_t* i = list_get_node_at(lst, idx);
    return i ? i->data : NULL;
}

inline static void list_inset_node_after(list_t* lst, node_t* n, node_t* i) {
    i->prev = n;
    i->next = n->next;
    n->next = i;
    if (n == lst->last) lst->last = i;
    ++lst->size;
}

inline static void list_inset_data_after(list_t* lst, node_t* n, void* s) {
    node_t* i = malloc(sizeof(node_t));
    i->data = s;
    list_inset_node_after(lst, n, i);
}

inline static void list_erase_node(list_t* lst, node_t* n) {
    node_t* prev = n->prev;
    node_t* next = n->next;
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    if (lst->first == n) lst->first = next;
    if (lst->last == n) lst->last = prev;
    if (n->data) lst->deallocator(n->data);
    free(n);
    --lst->size;
}
