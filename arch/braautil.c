
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "braaasn.h"
#include "braautil.h"

u_int32_t * oiddup(u_int32_t * o, int l)
{
	u_int32_t * oid = (u_int32_t*) gmalloc(l * sizeof(u_int32_t));
	memcpy(oid, o, sizeof(u_int32_t) * l);
	return(oid);
}
