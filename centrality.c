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
    printf("------------------------\n");
    printf("ID\t\tMETRIC\t\tNH\t\tLOAD\t\tTOTcontr\n");
    while(ptr!=NULL) {
      printf("%s\t\t%hu\t\t%s\t\t%hu\t\t%hu\n",format_eui64(ptr->nodeid), ptr->metric,
       format_address(ptr->nexthop), ptr->centrality, total_contribute(ptr->contributors) );
      ptr = ptr->next;
    }
}

unsigned short node_centrality() {
  struct destination* ptr = destinations;
  unsigned short retval = 0;
  while(ptr != NULL) {
    if(ptr->nodeid == myid)
      continue;
    retval = retval + total_contribute(ptr->contributors);
    ptr = ptr->next;
  }
  return retval;
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
        return;
      } else {
        prev = ptr;
        ptr = ptr->next;
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

void update_dest(unsigned char* nodeid, unsigned short metric, unsigned char* NH, struct neighbour* neigh) {
  printf("\t\tUPD DEST: nid=%s, NH=%s\n",format_eui64(nodeid), format_address(NH));
  struct destination* old = find_destination(nodeid);
  if (old) {
    if (metric < old->metric) {
      //if better update metric and NH
      old->metric=metric;
      memcpy(old->nexthop, NH, 16);
      old->neigh = neigh;
    }
  } else {
    //if not existing we should allocate it and push it at the top of table
    struct destination *link =
          (struct destination*) malloc(sizeof(struct destination));
        memcpy(link->nodeid, nodeid, 8);
        link->metric = metric;
       	memcpy(link->nexthop, NH, 16);
        link->neigh = neigh;
        link->contributors = NULL;
        link->centrality = 0;
       	link->next = destinations;
       	destinations = link;
  }
  return;
}

unsigned short total_contribute(struct contribute *head) {
  struct contribute *ptr = head;
	unsigned short total = 0;
  //printf("\t");
	while(ptr != NULL) {
      total = total + (ptr->contribute);
      /*printf("\t<%s,%i> ",
          format_address(ptr->neigh->address), ptr->contribute);*/
      ptr = ptr->next;
   }
   //printf("\n");
   return total;
}

struct contribute *update_contributors(struct contribute *head,
                        struct neighbour *neigh, unsigned short contribute) {
  printf("\t\tadding %hu, for neigh:%s\n",contribute, format_address(neigh->address));
	struct contribute *ptr = head;
	int found = 0;
	while(ptr != NULL) {
      if (ptr->neigh == neigh) {
      	ptr->contribute = contribute;
      	/*printf("CENTR; Item for neigh:%s updated with value %i\n",
        format_address(neigh->address), contribute);*/
        found = 1;
      	break;
      }
      ptr = ptr->next;
   }
   if (!found) {
   	/*printf("CENTR; Adding new element <%s,%i>\n",
    format_address(neigh->address), contribute);*/
    struct contribute *link =
      (struct contribute*) malloc(sizeof(struct contribute));
   	link->neigh = neigh;
   	link->contribute = contribute;
   	link->next = head;
   	head = link;
   }
   return head;
}

struct contribute* remove_contribute(struct contribute *head,
                        struct neighbour *neigh) {
   struct contribute* current = head;
   struct contribute* previous = NULL;
   if(head == NULL)
      return head;
   while(current->neigh != neigh) {
      if(current->next == NULL) {
         return head;
      } else {
         previous = current;
         current = current->next;
      }
   }
   //found a match, remove it!
   printf("CENTR; Removing contribute<%s,%hu>\n",
          format_address(current->neigh->address), current->contribute);
   if(current == head) {
      head = head->next;
   } else {
      previous->next = current->next;
   }
   free(current);
   return head;
}
