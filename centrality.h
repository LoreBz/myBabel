
struct contribute {
    struct neighbour *neigh;
    unsigned short contribute;
    struct contribute *next;
};

struct destination {
  unsigned char nodeid[8];
  unsigned short metric;
  unsigned char nexthop[16];
  struct contribute *contributors;
  unsigned short centrality;
  struct destination *next;
};


extern struct destination *destinations;

void refresh_dest_table(unsigned char* nodeid);
void remove_dest(unsigned char* nodeid);
struct destination* find_destination(const unsigned char *nodeid);
void update_dest(unsigned char* nodeid, unsigned short metric, unsigned char* NH);
