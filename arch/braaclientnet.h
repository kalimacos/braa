
#define gmalloc(a) malloc(a)
#define grealloc(a, b) realloc(a, b)

#define HASH_SIZE 128

struct braa_hqhost
{
	struct braa_hqhost * next;
	u_int32_t host;
	u_int16_t port;
	
	struct braa_asnobject * message;
	u_int8_t * messagecache;
	u_int16_t messagesize;

	unsigned char * community;	
	
	struct braa_hcquery * queries;
	int retries;
	struct timeb lastsentat;
	int responded;
};

struct braa_hcquery
{
	struct braa_hcquery * next;
	struct braa_hqhost * host;

	u_int32_t qid;
	
	u_int16_t oidlen;
	u_int32_t * oid;
	
	struct braa_asnobject * response;
	void * data;
};

struct braa_hclient
{
	int socket;
	
	int requestrate;
	int retries;
	
	int query_count, response_count;
	struct braa_hqhost * queries;
	
	
};

int braa_HClient_Process(struct braa_hclient * client, unsigned int mstimeout, int version);
struct braa_hclient * braa_HClient_Create(int requestrate, int retries);
struct braa_hqhost * braa_HClient_AddHost(struct braa_hclient * c, u_int32_t host, u_int16_t port, unsigned char * community);
struct braa_hcquery * braa_HClient_AddQuery(struct braa_hqhost * h, u_int16_t oidlen, u_int32_t * oid);
struct braa_hqhost * braa_HClient_FindHost(struct braa_hclient * c, u_int32_t host, u_int16_t port, unsigned char * community);
struct braa_hcquery * braa_HClient_AddQueryTo(struct braa_hclient * c, u_int32_t host, u_int16_t port, unsigned char * community, u_int16_t oidlen, u_int32_t * oid, void *data);
