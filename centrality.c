#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "babeld.h"
#include "kernel.h"
#include "util.h"
#include "neighbour.h"
#include "interface.h"
#include "route.h"
#include "source.h"
#include "centrality.h"

struct destination *destinations = NULL;

void print_dest_table() {
    struct destination* ptr = destinations;
    printf("-----------------DST----------------\n");
    while(ptr!=NULL) {
      printf("%s\t%hu\t%s\t%hu\n",format_eui64(ptr->nodeid), ptr->metric,
       format_address(ptr->nexthop), ptr->centrality );
      ptr = ptr->next;
    }
}

void refresh_dest_table(unsigned char* nodeid) {

}

void remove_dest(unsigned char* nodeid){
  struct destination* toRem = find_destination(nodeid);
  if(toRem) {
    struct destination* prev = NULL;
    struct destination* ptr = destinations;
    while (ptr!=NULL) {
      if (memcmp(ptr->nodeid, nodeid, 8)==0) {
        //swing pointers and free current
        if(prev)
          prev->next=ptr->next;
        free(ptr);
      }
    }
  }

}

struct destination* find_destination(const unsigned char *nodeid){
  if (destinations==NULL)
    return NULL;
  struct destination *ptr=destinations;
  while (ptr!=NULL) {
    if (memcmp(ptr->nodeid, nodeid, 8)==0) {
      return ptr;
    }
    ptr=ptr->next;
  }
  return NULL;
}

void update_dest(unsigned char* nodeid, unsigned short metric, unsigned char* NH) {
  struct destination* old = find_destination(nodeid);
  if (old) {
    if (metric < old->metric) {
      //if better update metric and NH
      old->metric=metric;
      memcpy(old->nexthop, NH, 16);
    }
  } else {
    //if not existing we should allocate it and push it at the top of table
    struct destination *link =
          (struct destination*) malloc(sizeof(struct destination));
        memcpy(link->nodeid, nodeid, 8);
        link->metric = metric;
       	memcpy(link->nexthop, NH, 16);
        link->contributors = NULL;
        link->centrality = 0;
       	link->next = destinations;
       	destinations = link;
  }
  return;
}
