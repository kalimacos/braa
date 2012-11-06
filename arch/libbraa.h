
struct braa_server
{

	int (*StartListening)(struct braa_server *, u_int32_t ip, u_int16_t port);
	int (*ProcessSingleMessage)(struct braa_server *);
	void (*ProcessMessages)(struct braa_server *);
	
	void (*RegisterOIDHandler)(struct braa_server *, struct braa_asnobject *, struct braa_asnobject * (*)(struct braa_server *, struct braa_asnobject*)));

	int fd;
	u_int16_t port;
};
