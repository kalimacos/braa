#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/timeb.h>
#include "braaasn.h"
#include "braaclientnet.h"

void help(unsigned char * name)
{
	printf("braa 0.42 - Mateusz 'mteg' Golicz <mtg@elsat.net.pl>, 2003\n", name);
	printf("usage: %s [options]\n", name);
	printf("  -h        Show this help\n");
	printf("  -r <ret>  Number of retries per host (default: 5)\n");
	printf("  -s <rate> Query rate - specify how many hosts query simultaneously (default: 50).\n");
	printf("  -t <time> Quit after time ms (default: 3000)\n");
	printf("  -q <query>Add a query to the query list\n");
	printf("  -f <file> Add queries from this file (one/line)\n");
	printf("  -2        Identify as a SNMP V2c agent.\n");
	printf("  -a        Alternative output format\n");
	printf("  -d        Show response times.\n");
	printf("\n");
	printf("Query format: [community@]host[:port]:oid[/id]\n");
	printf("Output format: host:oid:result\n");
					
}

void addquery(struct braa_hclient *c, unsigned char *q)
{
	unsigned char * community = NULL;
	unsigned char * host = NULL;
	unsigned char * oid = NULL;
	unsigned char * port;
	int i_port = 161;
	unsigned char * t;
	unsigned char * mem;
	unsigned char * orig = q;
	char * id = NULL;
	
	u_int32_t * numoid;
	u_int16_t oidlen;

	u_int32_t starthost, endhost, current;
	
	int i;
	
	q = mem = strdup(q);
	
	if((t = index(q, '@')))
	{
		*t = 0;
		community = q;
		q = t + 1;
	}
	if((t = index(q, ':')))
	{
		*t = 0;
		host = q;
		q = t + 1;
	}
	
	if((port = index(q, ':')))
	{
		char * eptr;
		*port = 0;
		i_port = strtol(q, &eptr, 10);
		if(*eptr)
		{
			fprintf(stderr, "'%s' - invalid port number, ignoring query '%s'\n", q, orig);
			free(mem);
			return;
		}
		q = port + 1;
	}
	
	oid = q;
	if(!(host && oid))
	{
		fprintf(stderr, "'%s' - invalid query, ignoring.\n", orig);
		free(mem);
		return;
	}

	debug("Decoding OID...\n");
	
	if((id = index(oid, '/')))
	{
		*id = 0;
		id++;
	}
	
	if(!(numoid = braa_StringToOID(oid, &oidlen)))
	{
		fprintf(stderr, "'%s' in '%s' - invalid OID, ignoring.\n", oid, orig);
		free(mem);
		return;
	}
#ifdef DEBUG
	printf("OID LEN: %d\n", oidlen);
	for(i = 0; i<oidlen; i++)
		printf("%d.\n", numoid[i]);
#endif
	debug("DONE\n");

	if(!community) community = strdup("public");
	if((t = index(host, '-')))
	{
		unsigned char * sendhost;
		*t = 0;
		sendhost = t + 1;
		starthost = inet_addr(host);
		endhost = inet_addr(sendhost);
	}
	else
	{
		starthost = inet_addr(host);
		endhost = starthost;
	}
	
	if(starthost == -1 || endhost == -1)
	{
		fprintf(stderr, "Invalid IP address in '%s', ignoring\n", orig);
		free(community);
		free(mem);
		return;
	}
	
	endhost = ntohl(endhost);
	starthost = ntohl(starthost);


	
	if(endhost < starthost)
	{
		fprintf(stderr, "'%s' - invalid host range, ignoring.\n", orig);
		free(mem);
		return;
	}

	debug("Adding queries...\n");
#ifdef DEBUG
	for(i = 0; i<oidlen; i++)
		printf("%d.\n", numoid[i]);
#endif
	
	for(current = starthost; current <= endhost; current++)
		braa_HClient_AddQueryTo(c, htonl(current), i_port, community, oidlen, numoid, id);


#ifdef DEBUG
	for(i = 0; i<oidlen; i++)
		printf("%d.", numoid[i]);
#endif

	debug("OK\n");

}

int main(int argc, char ** argv)
{
	struct braa_hclient * client;
	struct braa_hqhost * host;
	struct braa_hcquery * query;
	int c, v, snmpver = 0;
	int timeout = 3000;
	int alternat = 0;
	int rtt = 0;
	
	client = braa_HClient_Create(50, 5);

        while((c = getopt(argc, argv, "hq:f:r:s:t:ad2")) != EOF)
        {
		switch(c)
		{
			case '2': snmpver = 1; break;
			case 'a': alternat = 1;
				break;
			case 'd': rtt = 1; break;
			case 'h':
				help(argv[0]);
				return(0);
			case 'r':
				v = atoi(optarg);
				if(v < 1 || v > 1000)
					fprintf(stderr, "Invalid retry count, setting default of 5.\n");
				else
					client->retries = v;
				break;
			case 's':
				v = atoi(optarg);
				if(v < 1 || v > 1000)
					fprintf(stderr, "Invalid request rate, setting default of 50\n");
				else
					client->requestrate = v;
				break;
			case 't':
				v = atoi(optarg);
				if(v < 500 || v > 300000)
					fprintf(stderr, "Invalid timeout, setting default of 3s\n");
				else
					timeout = v;
				break;
			case 'q':
				addquery(client, optarg);
				break;
			case 'f':
			{
				unsigned char line[1024];
				FILE * fh;
				fh = fopen(optarg, "r");
				if(!fh)
				{
					perror("fopen");
					fprintf(stderr, "Unable to open query list: '%s'\n", optarg); 
					return 1;
				}
				
				while(fgets(line, 1024, fh))
				{
					unsigned char * eol;
					if((eol = index(line, '\n'))) *eol = 0;
					addquery(client, line);
				}
				
				
				fclose(fh);
			}
				
		}
	}
	
	braa_HClient_Process(client, timeout, snmpver);
	
	for(host = client->queries; host; host = host->next)
	{
		unsigned char shost[30];
		struct braa_hcquery * qstack[50];
		int stackpos;
		struct in_addr ina; 

		if(!host->responded) continue;

		ina.s_addr = host->host;
		snprintf(shost, 30, "%s", inet_ntoa(ina));
		
		if(alternat) printf("%s", shost);
		
		for(stackpos = 0, query = host->queries; query && stackpos < 50; query = query->next)
			qstack[stackpos++] = query;

		if(!stackpos) continue;
		
		for(query = qstack[--stackpos]; stackpos >= 0; query = qstack[--stackpos])
		{
			unsigned char oidbuf[200];
			unsigned char resultbuf[200];
			if(query->response)
			{
				braa_OIDToString(query->oid, query->oidlen, oidbuf, 200);

				braa_ASNToString(query->response, resultbuf, 200);
				if(alternat)
					printf(" %s", resultbuf);
				else
					if(query->data)
						printf("%s/%s:%s:%s\n", (char*) query->data, shost, oidbuf, resultbuf);
					else
						printf("%s:%s:%s\n", shost, oidbuf, resultbuf);
			}
		}
		if(rtt)
		{
			if(alternat)
				printf(" %dms", host->responded);
			else
				printf("%s:delay:%dms\n", shost, host->responded);
		}
		if(alternat) printf("\n");
	}
	
}
