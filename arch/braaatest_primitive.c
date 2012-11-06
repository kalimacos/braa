#include <sys/types.h>
#include <stdio.h>
#include "braaasn.h"

int main(int argc, char ** argv)
{
	struct braa_asnobject * message;
	struct braa_asnobject * ver;
	struct braa_asnobject * community;
	struct braa_asnobject * zero;

	struct braa_asnobject * pdu;
	struct braa_asnobject * vbl;
	struct braa_asnobject * vb;

	struct braa_asnobject * messagec[3];
	struct braa_asnobject * pduc[4];
	struct braa_asnobject * vblc[1];
	struct braa_asnobject * vbc[2];
	
	unsigned char buffer[1024];
	int s;

	u_int32_t oid[] = {1,3,6,1,2,1,1,5,0}; 	// 9 units
	
	
	zero = braa_CreateASNObject(BRAAASN_INTEGER, 0, NULL);

	vbc[0] = braa_CreateASNObject(BRAAASN_OID, 9, oid);
	vbc[1] = braa_CreateASNObject(BRAAASN_NULL, 0, NULL);

	vb = braa_CreateASNObject(BRAAASN_SEQUENCE, 2, vbc);

	vblc[0] = vb;
	vbl = braa_CreateASNObject(BRAAASN_SEQUENCE, 1, vblc);


	pduc[0] = zero;
	pduc[1] = zero;
	pduc[2] = zero;
	pduc[3] = vbl;
	
	pdu = braa_CreateASNObject(BRAAASN_PDU_GET, 4, pduc);
	
	ver = braa_CreateASNObject(BRAAASN_INTEGER, 0, NULL);
	community = braa_CreateASNObject(BRAAASN_OCTETSTRING, 0, "public");
	
	messagec[0] = ver;
	messagec[1] = community;
	messagec[2] = pdu;
	
	message = braa_CreateASNObject(BRAAASN_SEQUENCE, 3, messagec);

	s = braa_EncodeBER(message, buffer, 1024);
	if(s < 0)
	{
		printf("Error :%d\n", s);
	}
	else
	{
		int i;
		for(i = 0; i < s;)
		{
			int j;
			for(j = 0; j < 20 && i < s; j++, i++)
			{
				printf("%02x ", buffer[i]);
			}
			printf("\n");
		}
	}
	

}
