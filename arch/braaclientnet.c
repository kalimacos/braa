#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/timeb.h>

#include "braaasn.h"
#include "braaclientnet.h"
#include "braautil.h"

#define SELECT_TIME 200000
#define MAXPACKET 1500
#define RETRIES 10

u_int32_t GLOBAL_qid = 0;

int braa_HClient_Process(struct braa_hclient * client, unsigned int mstimeout, int snmp_version)
{
	struct timeval start;
	struct timeval now;
	struct timezone fake_tz;
	int try = 0;
	
	gettimeofday(&start, &fake_tz);
	
	while((client->query_count > client->response_count) && (try < RETRIES))
	{
		unsigned long mili;
		unsigned int sentreq = 0;
		struct braa_hqhost * h;
		struct braa_hcquery * q;
		int i;
		
		gettimeofday(&now, &fake_tz);
		mili = (now.tv_sec - start.tv_sec) * 1000 + ((now.tv_usec - start.tv_usec) / 1000);
		if(mili > mstimeout) return(client->response_count);
		
		/* send requests */
		for(h = client->queries; h; h = h->next)
		{
			if(sentreq > client->requestrate) break;

			if(h->responded) continue;
			if(h->retries > client->retries) continue;
			
			if(!h->messagecache)
			{
				struct braa_asnobject * vars_seq;
				struct braa_asnobject ** vars = NULL;
				struct braa_asnobject ** pduc;
				struct braa_asnobject * pdu;
				struct braa_asnobject ** msgc;
				struct braa_asnobject * msg;
				int varc = 0;
				int ee = 0;
				
				
				
				for(q = h->queries; q; q = q->next)
				{
					struct braa_asnobject * oid = braa_CreateASNObject(BRAAASN_OID, q->oidlen, oiddup(q->oid, q->oidlen));
					struct braa_asnobject * var;
					struct braa_asnobject ** varseql;
#ifdef DEBUG
					{
						int i;
						u_int32_t * oiddata = (u_int32_t*) q->oid;
						for(i = 0; i<oid->ldata; i++)
							printf("Oid while encoding: %d\n", oiddata[i]);
					}
#endif
					
					varseql = (struct braa_asnobject**) gmalloc(sizeof(struct braa_asnobject*) * 2);
					varseql[0] = oid;
					varseql[1] = braa_CreateASNObject(BRAAASN_NULL, 0, NULL);
					
					var = braa_CreateASNObject(BRAAASN_SEQUENCE, 2, varseql);
					vars = (struct braa_asnobject **) grealloc(vars, sizeof(struct braa_asnobject*) * (varc + 1));
					vars[varc ++] = var;
				}
				vars_seq = braa_CreateASNObject(BRAAASN_SEQUENCE, varc, vars);

				pduc = (struct braa_asnobject**) gmalloc(sizeof(struct braa_asnobject*) * 4);
				pduc[0] = braa_CreateASNObject(BRAAASN_INTEGER, 0, NULL);
				pduc[1] = braa_CreateASNObject(BRAAASN_INTEGER, 0, NULL);
				pduc[2] = braa_CreateASNObject(BRAAASN_INTEGER, 0, NULL);
				pduc[3] = vars_seq;
				pdu = braa_CreateASNObject(BRAAASN_PDU_GET, 4, pduc);

				msgc = (struct braa_asnobject**) gmalloc(sizeof(struct braa_asnobject*) * 3);
					
				msgc[0] = braa_CreateASNObject(BRAAASN_INTEGER, snmp_version, NULL);
				msgc[1] = braa_CreateASNObject(BRAAASN_OCTETSTRING, 0, strdup(h->community));
				msgc[2] = pdu;
				
				msg = braa_CreateASNObject(BRAAASN_SEQUENCE, 3, msgc);
				h->messagecache = (u_int8_t*) gmalloc(MAXPACKET);
				if((ee = braa_EncodeBER(msg, h->messagecache, MAXPACKET)) < 0)
				{
					debug("Error during encoding: %d\n", ee);
					return(-1);
				}
				else
				{
#ifdef DEBUG
					debug("Message for host %x generated, @%x, size: %d\n", h->host, h->messagecache, ee);
					braa_DumpASNObject(msg);
#endif	
					h->messagesize = ee;
				}
				debug("Disposing ASN object ...\n");
				braa_DisposeASNObject(msg);
				debug("Done\n");
			}
			
			{
				struct sockaddr_in dst;
				dst.sin_family = PF_INET;
				dst.sin_port = htons(h->port);
				dst.sin_addr.s_addr = h->host;
				fprintf(stderr, "Sending message for %s\n", inet_ntoa(dst.sin_addr));
				if(sendto(client->socket, h->messagecache, h->messagesize, 0, (struct sockaddr*) &dst, (socklen_t) sizeof(struct sockaddr_in)) < 0)
					perror("sendto");

				ftime(&h->lastsentat);

				h->retries++;
			}
			
			sentreq ++;
		}

		{
			struct timeval tv;
			fd_set fds;
			int z;
			
			tv.tv_sec = 0;
			tv.tv_usec = SELECT_TIME;
			FD_ZERO(&fds);
			FD_SET(client->socket, &fds);
			
			if(select(client->socket + 1, &fds, NULL, NULL, &tv))
			{
				do
				{
					int l;
					unsigned char pbuff[MAXPACKET];
					struct sockaddr_in sa;
					socklen_t salen = sizeof(struct sockaddr_in);
					
					if((l = recvfrom(client->socket, pbuff, MAXPACKET, 0, (struct sockaddr *) &sa, (socklen_t *) &salen)) <= 0)
						perror("recvfrom");
					else
					{
						struct braa_asnobject * ao;
						ao = braa_DecodeBER(pbuff, l);
						if(ao)
						{
							int cleanup = 1;
#ifdef DEBUG
							braa_DumpASNObject(ao);
#endif
// dig through the message
							if(ao->type == BRAAASN_SEQUENCE && ao->ldata == 3)
							{
								struct braa_asnobject ** list1 = (struct braa_asnobject **) ao->pdata;
								if(list1[2]->type == BRAAASN_PDU_GETRESPONSE && list1[2]->ldata == 4)
								{
									struct braa_asnobject ** list2 = (struct braa_asnobject **) list1[2]->pdata;
									if(list2[3]->type == BRAAASN_SEQUENCE)
									{
										struct braa_asnobject ** list3 = (struct braa_asnobject **) list2[3]->pdata;
										int i, count = list2[3]->ldata;
										
										cleanup = 0;
										
										debug("This is a valid response. Begin interpretation loop.\n");
										if(count)
										{
											struct braa_hqhost * h;
											// find the host
											for(h = client->queries; h; h = h->next)
												if(h->host == sa.sin_addr.s_addr) break;
											
											if(h)
											{
												struct timeb time;
												
												ftime(&time);
												
												h->responded = (time.time - h->lastsentat.time) * 1000 + time.millitm - h->lastsentat.millitm;
												if(!h->responded) h->responded = 1;
												
												client->response_count++;

												free(h->messagecache);
												h->messagecache = NULL;
												
												for(i = 0; i<count; i++)
												{
													if(list3[i]->type == BRAAASN_SEQUENCE && list3[i]->ldata == 2)
													{
														if(((struct braa_asnobject**) list3[i]->pdata)[0]->type == BRAAASN_OID)
														{
															struct braa_asnobject * oid = (struct braa_asnobject*) (((struct braa_asnobject**) list3[i]->pdata)[0]);
															struct braa_asnobject * data = (struct braa_asnobject*) (((struct braa_asnobject**) list3[i]->pdata)[1]);
															struct braa_hcquery * q;
															for(q = h->queries; q; q = q->next)
															{
																if(q->oidlen == oid->ldata)
																	if(memcmp(q->oid, oid->pdata, q->oidlen * sizeof(u_int32_t)) == 0) break;
															}
															if(q) q->response = data;
															debug("-> Found valid answer.");
														}
													}
												}
											}
											else
												fprintf(stderr, "Unknown host: %s\n", inet_ntoa(sa.sin_addr));
										}
									}
								}
							}
							
							if(cleanup)
							{
								braa_DisposeASNObject(ao);
							}
						}
					}
	
					FD_ZERO(&fds);
					FD_SET(client->socket, &fds);
					z = select(client->socket + 1, &fds, NULL, NULL, &tv);
				}
				while(z);
			}
		}
		
		if(!sentreq)
		{
			if(client->query_count == client->response_count) break;
			try++;
		}
		
	}
}

struct braa_hclient * braa_HClient_Create(int requestrate, int retries)
{
	struct braa_hclient * c = (struct braa_hclient *) gmalloc(sizeof(struct braa_hclient)); 
	
	c->requestrate = requestrate;
	c->retries = retries;
	c->queries = NULL;
	c->query_count = 0;
	c->response_count = 0;
	
	if((c->socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("socket");
		free(c);
		return(NULL);
	}
	return(c);
}

struct braa_hqhost * braa_HClient_AddHost(struct braa_hclient * c, u_int32_t host, u_int16_t port, unsigned char * community)
{
	struct braa_hqhost * h = (struct braa_hqhost *) gmalloc(sizeof(struct braa_hqhost));
	h->next = c->queries;
	h->host = host;
	h->port = port;
	h->message = NULL;
	h->messagecache = NULL;
	h->community = community;
	h->queries = NULL;
	h->retries = 0;
	h->responded = 0;
	
	c->queries = h;
	c->query_count ++;
	
	return(h);
}

struct braa_hqhost * braa_HClient_FindHost(struct braa_hclient * c, u_int32_t host, u_int16_t port, unsigned char * community)
{
	struct braa_hqhost * h;
	for(h = c->queries; h; h = h->next)
	{
		if(h->host == host && h->port == port && strcmp(h->community, community) == 0)
			return(h);
	}
	return(braa_HClient_AddHost(c, host, port, community));
}

struct braa_hcquery * braa_HClient_AddQuery(struct braa_hqhost * h, u_int16_t oidlen, u_int32_t * oid)
{
	struct braa_hcquery * q = (struct braa_hcquery *) gmalloc(sizeof(struct braa_hcquery));
	q->next = h->queries;
	q->host = h;
	
	q->qid = GLOBAL_qid++;
	
	q->oidlen = oidlen;
	q->oid = oid;
	
	q->response = NULL;
	
	h->queries = q;
	
	return(q);
}

struct braa_hcquery * braa_HClient_AddQueryTo(struct braa_hclient * c, u_int32_t host, u_int16_t port, unsigned char * community, u_int16_t oidlen, u_int32_t * oid, void *data)
{
	struct braa_hqhost * h = braa_HClient_FindHost(c, host, port, community);
	struct braa_hcquery * q = (struct braa_hcquery *) gmalloc(sizeof(struct braa_hcquery));
	
	q->next = h->queries;
	q->host = h;
	
	q->qid = GLOBAL_qid++;
	
	q->oidlen = oidlen;
	q->oid = oid;
	
	q->response = NULL;
	
	h->queries = q;
	
	q->data = data;

	return(q);
}

