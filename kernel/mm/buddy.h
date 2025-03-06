#define MAX_ORDER 12  // maximum order of buddy system

struct free_list {
    uint64 addr;    // start address
    struct free_list *next; 
};

struct free_area {
    struct free_list *head;
};

struct zone {
    uint npages;  // total number of pages in the zone
    struct free_area free_area[MAX_ORDER];
};
