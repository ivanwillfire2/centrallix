#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>

long long
test(char** tname)
    {
    int i, rval;
    int iter;
    unsigned char buf[44];

	*tname = "qprintf-60 %nSTR&PATH fixed-length insert tests";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[26] = '\n';
	    buf[25] = '\0';
	    buf[24] = 0xff;
	    buf[23] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%8STR&PATH/name", "file/..\0");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%8STR&PATH/name", "one/t/.\0");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%8STR&PATH/name", "one/t/..");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%8STR&PATH/name", "one/t/...");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%8STR&PATH/name", "one/two/");
	    assert(strcmp(buf+4,"/path/one/two//name") == 0);
	    assert(rval == 19);
	    assert(buf[26] == '\n');
	    assert(buf[25] == '\0');
	    assert(buf[24] == 0xff);
	    assert(buf[23] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*5;
    }

