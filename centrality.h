
struct contribute {
    struct neighbour *neigh;
    unsigned short contribute;
    struct contribute *next;
};

struct destination {
  unsigned char nodeid[8];
  unsigned short metric;
  unsigned char nexthop[16];
  struct neighbour* neigh;
  struct contribute *contributors;
  unsigned short centrality;
  struct destination *next;
};


extern struct destination *destinations;

void print_dest_table();

void refresh_dest_table(unsigned char* nodeid);
void remove_dest(unsigned char* nodeid);
struct destination* find_destination(const unsigned char *nodeid);
void update_dest(unsigned char* nodeid, unsigned short metric, unsigned char* NH, struct neighbour* neigh);
unsigned short total_contribute(struct contribute *head);
struct contribute *update_contributors(struct contribute *head,
                        struct neighbour *neigh, unsigned short contribute);
struct contribute* remove_contribute(struct contribute *head,
                                          struct neighbour *neigh);
unsigned short node_centrality();
