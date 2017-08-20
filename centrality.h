
struct contribute {
    struct neighbour *neigh;
    unsigned contribute;
    struct contribute *next;
};

struct destination {
  unsigned char nodeid[8];
  unsigned short metric;
  unsigned char nexthop[16];
  struct contribute *contributors;
  unsigned centrality;
  struct destination *next;
};


extern struct destination *destinations;

void print_dest_table();

void refresh_dest_table();
void remove_dest(unsigned char* nodeid);
struct destination* find_destination(const unsigned char *nodeid);
void update_dest(unsigned char* nodeid, unsigned short metric, unsigned char* NH);
unsigned total_contribute(struct contribute *head);
struct contribute *update_contributors(struct contribute *head,
                        struct neighbour *neigh, unsigned contribute);
struct contribute* remove_contribute(struct contribute *head,
                                          struct neighbour *neigh);
unsigned node_centrality();
struct destination *find_update_dest(unsigned char* nodeid,unsigned short metric, unsigned char* NH);
void foo(FILE* out);
