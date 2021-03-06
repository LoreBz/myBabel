#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "babeld.h"
#include "kernel.h"
#include "util.h"
#include "neighbour.h"
#include "interface.h"
#include "route.h"
#include "source.h"
#include "centrality.h"

struct destination *destinations = NULL;

void foo(FILE* out) {
  struct destination* ptr = destinations;
  fprintf(out,",\n\t {"
              "\n\t\t\"nodeid\": \"%s\",\n",format_eui64(myid));
  fprintf(out,"\t\t\"interfaces\":[\n");
  int count=0;
  struct interface *ifp;
  FOR_ALL_INTERFACES(ifp) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, ifp->ipv4, addr, INET_ADDRSTRLEN);
    char addr6[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, ifp->ll, addr6, INET6_ADDRSTRLEN);
    if(count!=0) {
      fprintf(out, ",\n");
    }
    fprintf(out, "\t\t\t{\n"
      "\t\t\t\t\"name\": \"%s\",\n"
      "\t\t\t\t\"ipv4\": \"%s\",\n"
      "\t\t\t\t\"ipv6\": \"%s\"\n"
      "\t\t\t}",ifp->name, addr, addr6);
    count++;
  }
  fprintf(out, "\n\t\t],\n");

  fprintf(out,"\t\t\"destinations\":[\n");
  count = 0;
  while(ptr!=NULL) {
    if(count!=0) {
        fprintf(out, ",\n");
    }
    fprintf(out,  "\t\t\t{\n"
                  "\t\t\t\"dest\": \"%s\",\n"
                  "\t\t\t\"NH\": \"%s\",\n"
                  "\t\t\t\"CENTR\": \"%u\""
                  "\n\t\t\t}",
                  format_eui64(ptr->nodeid),format_address(ptr->nexthop),ptr->centrality);
    count++;
    ptr = ptr->next;
    }
  fprintf(out, "\n\t\t]\n\t}");

}

void print_dest_table() {
    struct destination* ptr = destinations;
    printf("--------------------------------------------------------------------\n");
    printf("%-24s %-6s %-12s %-5s %-3s\n", "ID", "METRIC", "NH", "LOAD", "TOTcontr");
    //printf("ID\t\tMETRIC\t\tNH\t\tLOAD\t\tTOTcontr\n");
    while(ptr!=NULL) {
      printf("%-24s %-6hu %-12s %-5u %-3hu\n",format_eui64(ptr->nodeid), ptr->metric,
       format_address(ptr->nexthop), ptr->centrality, total_contribute(ptr->contributors) );
      ptr = ptr->next;
    }
    printf("\n");
}

unsigned node_centrality() {
  struct destination* ptr = destinations;
  unsigned retval = 0;
  while(ptr != NULL) {
    if(ptr->nodeid == myid)
      continue;
    retval = retval + total_contribute(ptr->contributors);
    ptr = ptr->next;
  }
  return retval;
}

void remove_dest(unsigned char* nodeid){
    struct destination* prev = NULL;
    struct destination* ptr = destinations;
    while (ptr!=NULL) {
      if (memcmp(ptr->nodeid, nodeid, 8)==0) {
        //swing pointers and free current
        printf("\t\tREMOVE DEST: nid=%s\n",format_eui64(nodeid));
        if(prev)
          prev->next=ptr->next;
        else
          destinations=NULL;
        free(ptr);
        return;
      } else {
        prev = ptr;
        ptr = ptr->next;
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

/*struct destination *find_update_dest(unsigned char* nodeid,unsigned short metric, unsigned char* NH) {
  struct destination *dest;
  for (dest=destinations; dest; dest=dest->next) {
    if (memcmp(dest->nodeid, nodeid, 8)==0) {
      if (metric < dest->metric) {
        debugf("\t\tUPD DEST: nid=%s, NH=%s\n",format_eui64(nodeid), format_address(NH));
        //if better update metric and NH
        dest->metric=metric;
        memcpy(dest->nexthop, NH, 16);
      }
      return dest;
    }
  }
  //if not already return means we miss this dest
  printf("\t\tCREAT DEST: nid=%s, NH=%s\n",format_eui64(nodeid), format_address(NH));
  struct destination *link =
        (struct destination*) malloc(sizeof(struct destination));
      memcpy(link->nodeid, nodeid, 8);
      link->metric = metric;
      memcpy(link->nexthop, NH, 16);
      link->contributors = NULL;
      link->centrality = 0;
      link->next = destinations;
      destinations = link;
      return destinations;
}*/

void refresh_dest_table() {
  //select best NH for each destination
  struct route_stream *routes;
  routes = route_stream(ROUTE_INSTALLED);
  if(routes) {
      while(1) {
          struct babel_route *rt = route_stream_next(routes);
          if(rt == NULL) break;
          update_dest(rt->src->id, route_metric(rt), rt->nexthop);
      }
      route_stream_done(routes);
  }
}

void update_dest(unsigned char* nodeid, unsigned short metric, unsigned char* NH) {
  struct destination* old = find_destination(nodeid);
  if (old) {
    if (metric < old->metric) {
      printf("\t\tUPD DEST: nid=%s, NH=%s\n",format_eui64(nodeid), format_address(NH));
      //if better update metric and NH
      old->metric=metric;
      memcpy(old->nexthop, NH, 16);
    }
  } else {
    //if not existing we should allocate it and push it at the top of table
    printf("\t\tCREAT DEST: nid=%s, NH=%s\n",format_eui64(nodeid), format_address(NH));
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

unsigned total_contribute(struct contribute *head) {
  struct contribute *ptr = head;
	unsigned total = 0;
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
                        struct neighbour *neigh, unsigned contribute) {
  debugf("\t\tadding %hu, for neigh:%s\n",contribute, format_address(neigh->address));
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
