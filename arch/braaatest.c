#include <sys/types.h>
#include <stdio.h>
#include "braaasn.h"
#include "braaclientnet.h"

int main(int argc, char ** argv)
{
	struct braa_hclient * client;
	struct braa_hqhost * host;
	
	u_int32_t oid[] = {1,3,6,1,2,1,1,5,0}; 	// 9 units
	u_int32_t * myoid;
	
	myoid = malloc(sizeof(u_int32_t) * 9);
	memcpy(myoid, oid, sizeof(u_int32_t) * 9);
	
	client = braa_HClient_Create(10, 5);
	host = braa_HClient_AddHost(client, inet_addr("10.253.101.1"), 161, "public");
	braa_HClient_AddQuery(host, 9, myoid);
	
	braa_HClient_Process(client, 10000);
}
