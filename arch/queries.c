#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include "braaoids.h"
#include "braaasn.h"
#include "braaprotocol.h"
#include "queries.h"

#define MAX_PACKET_SIZE 1300

static struct query_hostrange * duplicate_hostrange(struct query_hostrange *h)
{
	struct query_hostrange * n;
	int i;
	
	assert(n = (struct query_hostrange*) gmalloc(sizeof(struct query_hostrange)));
	
	n->port = h->port;
	assert(n->community = strdup(h->community));
	
	n->query_count = h->query_count;
	assert(n->queries = (char**) gmalloc(sizeof(char*) * h->query_count));
	for(i = 0; i<n->query_count; i++)
		assert(n->queries[i] = strdup(h->queries[i]));

	return(n);
}

int bapp_rangesplit_query(struct query_hostrange ** head, char * string, char * errbuff, int len)
{
	/* [public@]hostrange[:port]:whatever/id,whatever/id,whatever/id */
	char * community = "public",
	     * pstr, * hostrange = NULL,
		 * queries, * portno = NULL,
		 * nxqry;
	struct query_hostrange * hr, * prev;
	u_int32_t hostrange_start, hostrange_end;
	u_int16_t port = 161;
	struct in_addr ina;
		 
	assert(string = strdup(string));
	
	if((pstr = index(string, '@')))
	{
		community = string;
		*pstr = 0;
		hostrange = string + 1;
	}
	else
		hostrange = string;
	
	if(!(pstr = index(string, ':')))
	{
		snprintf(errbuff, len, "Invalid syntax.");
		goto err;
	}
	
	*pstr = 0;
	queries = pstr + 1;

	if((pstr = index(queries, ':')))
	{
		portno = queries;
		*pstr = 0;
		queries = pstr + 1;
	}

	if((pstr = index(hostrange, '-')))
	{
		*pstr = 0;
		pstr++;
	}
	
	if(!inet_aton(hostrange, &ina))
	{
		snprintf(errbuff, len, "Invalid IP address: '%s'.", hostrange);
		goto err;
	}
	
	hostrange_start = ntohl(ina.s_addr);
	
	if(pstr)
	{
		if(!inet_aton(pstr, &ina))
		{
			snprintf(errbuff, len, "Invalid IP address: '%s'.", pstr);
			goto err;
		}
	
		hostrange_end = ntohl(ina.s_addr);
	}
	else
		hostrange_end = hostrange_start;
	
	if(hostrange_end < hostrange_start)
	{
		snprintf(errbuff, len, "Invalid range: '%s - %s'.", hostrange, pstr);
		goto err;
	}
	
	if(portno)
	{
		char * eptr;
		port = strtol(portno, &eptr, 10);
		if(*eptr)
		{
			snprintf(errbuff, len, "Invalid port: '%s'.", portno);
			goto err;
		}
		if(port > 65535 || port < 1) 
		{
			snprintf(errbuff, len, "Invalid port: '%s'.", portno);
			goto err;
		}
	}
	
	for(hr = *head, prev = (struct query_hostrange*) head; hr; prev = hr, hr = hr->next)
	{
		if(port != hr->port) continue;
		if(strcmp(community, hr->community)) continue;

		if(hostrange_start >= hr->start && hostrange_end <= hr->end)
		{
			u_int32_t bef = hostrange_start - hr->start, aft = hr->end - hostrange_end;
			
			if(bef)
			{
				struct query_hostrange * bh;
				bh = duplicate_hostrange(hr);
				
				bh->start = hr->start;
				bh->end = hostrange_start - 1;
				prev->next = bh;
				bh->next = hr;
			}
			if(aft)
			{
				struct query_hostrange * ah;
				ah = duplicate_hostrange(hr);
				
				ah->start = hostrange_end + 1;
				ah->end = hr->end;
				prev->next = ah;
				ah->next = hr;
			}
			hr->start = hostrange_start;
			hr->end = hostrange_end;
			break;
		}
	}
	
	if(!hr)
	{
		assert(hr = (struct query_hostrange*) gmalloc(sizeof(struct query_hostrange)));
		prev->next = hr;
		hr->next = NULL;

		hr->start = hostrange_start;
		hr->end = hostrange_end;
		assert(hr->community = strdup(community));
		hr->query_count = 0;
		hr->queries = NULL;
		hr->port = port;
	}
	
	do
	{
		if((nxqry = index(queries, ',')))
		{
			*nxqry = 0;
			nxqry++;
		}
		
		hr->queries = (char**) grealloc(hr->queries, (hr->query_count + 1) * sizeof(char*));
		assert(hr->queries[hr->query_count++] = strdup(queries));
	}
	while((queries = nxqry));
	
	return(1);

err:
	free(string);
	return(0);
}

int bapp_parse_queries(int version, struct query_hostrange * head, char * errbuf, int len)
{
	struct query_hostrange * hr;
	int needed_responses = 0; 
	
	for(hr = head; hr; hr = hr->next)
	{
		int qc = hr->query_count, i;

		hr->gets = NULL;
		hr->sets = NULL;
		hr->walks = NULL;
		hr->walk_count = 0;
		hr->ids = NULL;
		
		for(i = 0; i<qc; i++)
		{
			char * q = hr->queries[i];
			char * pstr, * id = NULL;
			int type = 0;
			asnobject * value;
			oid * o;
			
			if((pstr = index(q, '/')))
			{
				*pstr = 0;
				id = pstr + 1;
			}
			
			if(strlen(q) < 2)
			{
				snprintf(errbuf, len, "Invalid query: '%s'!", q);
				return(0);
			}
			
			if(q[strlen(q) - 1] == '*')
			{
				type = 2; /* walk */
				if(q[strlen(q) - 2] != '.')
				{
					snprintf(errbuf, len, "Invalid query: '%s'!", q);
					return(0);
				}
				q[strlen(q) - 2] = 0;
			}
			else if((pstr = index(q, '=')))
			{
				type = 1; /* set */
				*pstr = 0;
				pstr++;
				
				if(!*pstr)
				{
					snprintf(errbuf, len, "Invalid value: '%s'!", pstr);
					return(0);
				}
				
				if(!(value = braa_ASNObject_CreateFromString(pstr)))
				{
					snprintf(errbuf, len, "Invalid value: '%s'!", pstr);
					return(0);
				}
			}
			else
				type = 0;
			
			if(!(o = braa_OID_CreateFromString(q)))
			{
				snprintf(errbuf, len, "Invalid OID: '%s'!", q);
				return(0);
			}
			
			if(id)
			{
				struct idmap * n;
				assert(n = (struct idmap*) gmalloc(sizeof(struct idmap)));
				n->o = o;
				assert(n->id = strdup(id));
				n->next = hr->ids;
				hr->ids = n;
			}
			
			
			switch(type)
			{
				case 0:
					if(!hr->gets)
					{
						hr->gets = braa_GetRequestMsg_Create(hr->community, version);
						assert(hr->get_response_table = (char*) gmalloc(sizeof(char) * (hr->end - hr->start + 1)));
						memset(hr->get_response_table, 0, (hr->end - hr->start + 1) * sizeof(char));
					}
					
					braa_GetRequestMsg_Insert(hr->gets, o);
					break;
				case 1:
					if(!hr->sets)
					{
						hr->sets = braa_SetRequestMsg_Create(hr->community, version);
						assert(hr->set_response_table = (char*) gmalloc(sizeof(char) * (hr->end - hr->start + 1)));
						memset(hr->set_response_table, 0, (hr->end - hr->start + 1) * sizeof(char));
					}
					
					braa_SetRequestMsg_Insert(hr->sets, o, value);
					break;
				case 2:
				{
					struct walk_data * wd;
					u_int32_t i, l;
					
					assert(wd = (struct walk_data*) gmalloc(sizeof(struct walk_data) * (l = hr->end - hr->start + 1)));
					for(i = 0; i<l; i++)
					{
						wd[i].first = o;
						wd[i].latest = NULL;
						wd[i].retries = 0;
					}
					
					assert(hr->walks = (struct walk_data**) grealloc(hr->walks, sizeof(struct walk_data*) * hr->walk_count));
					hr->walks[hr->walk_count++] = wd;
					break;
				}
			}
			needed_responses += hr->end - hr->start + 1;
		}
		
		if(hr->sets)
		{
			int l;
			assert(hr->set_message = (u_int8_t*) gmalloc(MAX_PACKET_SIZE));
			if((l = braa_ASNObject_EncodeBER(hr->sets, hr->set_message, MAX_PACKET_SIZE)) < 0)
			{
				snprintf(errbuf, len, "Trouble while encoding the SET message. Internal error?");
				return(0);
			}
			hr->set_length = l;
		}
		else
			hr->set_message = NULL;

		if(hr->gets)
		{
			int l;
			assert(hr->get_message = (u_int8_t*) gmalloc(MAX_PACKET_SIZE));
			if((l = braa_ASNObject_EncodeBER(hr->gets, hr->get_message, MAX_PACKET_SIZE)) < 0)
			{
				snprintf(errbuf, len, "Trouble while encoding the GET message. Internal error?");
				return(0);
			}
			hr->get_length = l;
		}
		else
			hr->get_message = NULL;
	}
	return(needed_responses);
}

char * bapp_findid(struct query_hostrange * head, oid * o)
{
	struct idmap * ids;
	for(ids = head->ids; ids; ids = ids->next)
		if(braa_OID_Compare(ids->o, o)) return(ids->id);
	return(NULL);
	
}

void bapp_inititerator(iterator *i, struct query_hostrange *head)
{
	i->head = head;
	i->hr = head;
	i->current_host = 0;
}

int bapp_sendmessage(int s, iterator *it, int retries, int version, int xdelay)
{
	struct query_hostrange *hr = it->hr;
	int activity = 0;
	struct sockaddr_in dst;
	struct query_hostrange *start_hr = hr;
	int start_host = it->current_host;

	for(;;)
	{
		dst.sin_family = PF_INET;
		dst.sin_port = htons(hr->port);
		dst.sin_addr.s_addr = htonl(it->current_host + hr->start);

		if(hr->get_message)
		{
			if(hr->get_response_table[it->current_host] < retries)
			{
				if(sendto(s, hr->get_message, hr->get_length, 0, (struct sockaddr*) &dst, (socklen_t) sizeof(struct sockaddr_in)) < 0)
					perror("sendto");
						
				hr->get_response_table[it->current_host]++;
				activity = 1;
			}
		}

		if(hr->set_message)
		{
			if(hr->set_response_table[it->current_host] < retries)
			{
				if(sendto(s, hr->set_message, hr->set_length, 0, (struct sockaddr*) &dst, (socklen_t) sizeof(struct sockaddr_in)) < 0)
					perror("sendto");
						
				hr->set_response_table[it->current_host]++;
				activity = 1;
			}
		}

		if(hr->walks)
		{
			int wc = hr->walk_count;
			int i, size;
			asnobject * walkmsg = braa_GetNextRequestMsg_Create(hr->community, version);
			
			for(i = 0, size = 0; i<wc; i++)
			{
				if(hr->walks[i][it->current_host].retries < retries)
				{
					if(hr->walks[i][it->current_host].latest)
						braa_GetNextRequestMsg_Insert(walkmsg, hr->walks[i][it->current_host].latest);
					else
						braa_GetNextRequestMsg_Insert(walkmsg, hr->walks[i][it->current_host].first);
					hr->walks[i][it->current_host].retries++;
					size++;
				}
			}
			
			if(size)
			{
				char buffer[MAX_PACKET_SIZE];
				int len;
				
				if((len = braa_ASNObject_EncodeBER(walkmsg, buffer, MAX_PACKET_SIZE)) < 0)
					fprintf(stderr, "Trouble while encoding walk message, internal error?\n");
				else
				{
					if(sendto(s, buffer, len, 0, (struct sockaddr*) &dst, (socklen_t) sizeof(struct sockaddr_in)) < 0)
						perror("sendto");
				}

			}
			braa_ASNObject_Dispose(walkmsg);
		}
		
		it->current_host++;

		if((it->current_host + hr->start) > hr->end)
		{
			hr = hr->next;
			
			if(!hr)
				bapp_inititerator(it, it->head);
			
			hr = it->head;
			if(xdelay)
				usleep(xdelay);
		}
		
		if(activity)
			break;
		else
		{
			if(hr == start_hr && start_host == it->current_host)
				return(0);
		}
	}
	return(1);
}

int bapp_processmessages(int s, struct query_hostrange * head)
{
	int recvd_messages = 0;
	for(;;)
	{
		struct sockaddr_in sa;
		socklen_t salen = sizeof(struct sockaddr_in);
		int l;
		char pbuff[MAX_RECV_PACKET];
		asnobject * ao;
							
		if((l = recvfrom(s, pbuff, MAX_RECV_PACKET, 0, (struct sockaddr *) &sa, (socklen_t *) &salen)) <= 0)
		{
			if(errno == EAGAIN) break;
			perror("recvfrom");
		}
				
		ao = braa_ASNObject_DecodeBER(pbuff, l);
		if(ao)
		{
			u_int32_t host;
			struct query_hostrange *hr;
			int resp, error, ei;
			
			if(braa_Msg_Identify(ao) != BRAAASN_PDU_GETRESPONSE)
			{
				printf("Wrong message type!\n");
				goto dispose;
			}
			
			host = ntohl(sa.sin_addr.s_addr);
			
			for(hr = head; hr; hr = hr->next)
			{
				if(host >= hr->start && host <= hr->end)
					break;
			}
			
			if(!hr) goto dispose;
			
			resp = braa_PDUMsg_GetRequestID(ao);
			error = braa_PDUMsg_GetErrorCode(ao);
			ei = braa_PDUMsg_GetErrorIndex(ao);


			switch(resp)
			{
				case BRAAASN_PDU_GETREQUEST:
				{
					int v, vc;
					
					
					if(hr->get_response_table[host - hr->start] == RETRIES_MAX) goto dispose;
					recvd_messages++;
					hr->get_response_table[host - hr->start] = RETRIES_MAX;
					vc = braa_PDUMsg_GetVariableCount(ao);
					
					if(error)
					{
						ei--;
						if(ei >= 0 && ei < vc)
						{
							asnobject *name;
							char buffer[200];

							name = braa_PDUMsg_GetVariableName(ao, ei);
							assert(name->type == BRAAASN_OID);
							braa_OID_ToString((oid*) name->pdata, buffer, 200);
							
							fprintf(stderr, "%s:%s:Error %s.\n", inet_ntoa(sa.sin_addr), buffer, braa_StrError(error));
						}
						else
							fprintf(stderr, "%s:Error %s. Index: %d.\n", inet_ntoa(sa.sin_addr), braa_StrError(error), ei);
					}
					else
					{
						for(v = 0; v<vc; v++)
						{
							asnobject * name, * value;
							char buffer[200];
							char *id;
							
							name = braa_PDUMsg_GetVariableName(ao, v);
							value = braa_PDUMsg_GetVariableValue(ao, v);
							assert(name->type == BRAAASN_OID);
							
							id = bapp_findid(hr, (oid*) name->pdata);
							if(id)
								printf("%s/", id);
							
							printf("%s:", inet_ntoa(sa.sin_addr));
							braa_OID_ToString((oid*) name->pdata, buffer, 200);
							printf("%s:", buffer);
							braa_ASNObject_ToString(value, buffer, 200);
							printf("%s\n", buffer);
						}
					}
					break;
				}
				case BRAAASN_PDU_GETNEXTREQUEST:
				{
					int v, vc;
					
					if(error)
					else
					{
						host -= hr->start;
					
						vc = braa_PDUMsg_GetVariableCount(ao);
						if(vc >= hr->walk_count) vc = hr->walk_count;
						
						for(v = 0; v<vc; v++)
						{
							int wn = 0, i;
							asnobject * name, * value;

							/* find the walk */
							/* assumes the sequence is the same, which is not really good */

							for(wn = 0, i = v; wn < hr->walk_count && i > 0; wn++)
								if(!hr->walks[wn][host].done) i--;
							
							name = braa_PDUMsg_GetVariableName(ao, v);
							assert(name->type == BRAAASN_OID);
							
							if(braa_OID_CompareN(hr->walks[wn][host].first, (oid*) name->pdata))
							{
								value = braa_PDUMsg_GetVariableValue(ao, v);
								printf("%s:", inet_ntoa(sa.sin_addr));
								braa_OID_ToString((oid*) name->pdata, buffer, 200);
								printf("%s:", buffer);
								braa_ASNObject_ToString(value, buffer, 200);
								printf("%s\n", buffer);
								if(hr->walks[wn][host].latest)
									braa_OID_Dispose(hr->walks[wn][host].latest);
								
								hr->walks[wn][host].latest = braa_OID_Duplicate((oid*) name->pdata);
							}
							else
							{	/* end of walk */
								hr->walks[wn][host].done = 1;
								hr->walks[wn][host].retries = RETRIES_MAX;
								recvd_messages++;
							}
						}
					}
				
				}
				
			}
			
//			braa_ASNObject_Dump(ao);
dispose:
			braa_ASNObject_Dispose(ao);
		}
	}
	return(recvd_messages);
}
