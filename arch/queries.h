
struct idmap
{
	struct idmap * next;
	oid * o;
	char * id;
};

struct walk_data
{
	oid * first;
	oid * latest;
	char retries;
	char sent;
};

struct query_hostrange
{
	struct query_hostrange * next;
	
	u_int32_t start;
	u_int32_t end;
	u_int16_t port;
	
	int query_count;
	char ** queries;
	char * community;

	struct idmap * ids;

	asnobject * gets;
	char * get_response_table;
	u_int8_t * get_message;

	asnobject * sets;
	char * set_response_table;
	u_int8_t * set_message;

	u_int16_t get_length;
	u_int16_t set_length;

	int walk_count;
	struct walk_data ** walks;
}; 

int bapp_rangesplit_query(struct query_hostrange ** head, char * string, char * errbuff, int len);
int bapp_parse_queries(int version, struct query_hostrange * head, char * errbuf, int len);
char * bapp_findid(struct query_hostrange * head, oid * o);

struct iterator
{
	struct query_hostrange * head;
	struct query_hostrange * hr;
	u_int32_t current_host;
};

typedef struct iterator iterator;
int bapp_sendmessage(int s, iterator *it, int retries, int version, int xdelay);
void bapp_inititerator(iterator *i, struct query_hostrange *head);
int bapp_processmessages(int s, struct query_hostrange * head);

#define MAX_RECV_PACKET 1500
#define RETRIES_MAX 126
#define DEFAULT_XDELAY 100000
