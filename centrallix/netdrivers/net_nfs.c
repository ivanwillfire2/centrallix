#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h> //for regex functions
#include <regex.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_LIBZ 1
#endif

#include "stparse.h"
#include "st_node.h"

#include "centrallix.h"

#include "nfs/mount.h"
#include "nfs/nfs.h"
#include "nfs/mount_xdr.c"
#include "nfs/nfs_xdr.c"

/** xdr/rpc stuff **/
#include <rpc/xdr.h>
#include <rpc/rpc_msg.h>

/** XRingQueue **/
#include "xringqueue.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	net_nfs.c                 				*/
/* Author:	Jonathan Rupp, Nathan Ehresman,				*/
/*		Michael Rivera, Corey Cooper				*/
/* Creation:	February 19, 2003  					*/
/* Description:	Network handler providing an NFS interface to	 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: net_nfs.c,v 1.14 2003/03/30 20:03:35 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_nfs.c,v $

    $Log: net_nfs.c,v $
    Revision 1.14  2003/03/30 20:03:35  nehresma
    added open object caching for 30 seconds
        NOTES:
        - when opening the object I used "system/object" for the type, I had no
          idea what type to use
        - currently scans the list every second for objects that have been open
          longer than 30 seconds.  (should make configurable??)
        - the caller should not close the pObjects he receives, letting the
          monitor handle that.

    Revision 1.13  2003/03/29 08:55:44  jorupp
     * switched to an XRingQueue for internal queueing
     * added (experimental) code to (recursively) decode what fhandle a request needs
     * added inode 'locking' to a specific thread, and it seems to work pretty well
       -- note: any ideas how to test this well?
          I just added a thSleep(3000) (sleep for 3 seconds) to the nfsstat procedure
          and wached the requests back up on that thread's queue as they came in --
          any good ideas on how to do it better than that?

    Revision 1.12  2003/03/12 00:28:56  jorupp
     * add the output of the 'nextFileHandle' to the inode.map file
     * fix a minor bug with the error handling while reading in values from inode.map

    Revision 1.11  2003/03/11 03:16:33  jorupp
     * Mike and I got dumping and reloading of the inode mapping working

    Revision 1.10  2003/03/09 18:59:13  jorupp
     * add SIGINT handling, which calls shutdown handlers

    Revision 1.9  2003/03/09 07:47:57  jorupp
     * reversed some of the changes nehresma made earlier on bad advice from me
     * added some extra xdr_free calls

    Revision 1.8  2003/03/09 06:27:00  nehresma
    fixed one more place where nmMalloc should be malloc because of the xdr_*
    functions doing the freeing

    Revision 1.7  2003/03/09 06:10:13  nehresma
    - changed dump and export implementations to use malloc instead of nmMalloc
      and then let the xdr_* functions handle the freeing of the lists, rather
      than handling it myself (per jorupp's suggestion).
    - mountd will no longer exit its thread when it doesn't have proper config
      information.  instead it listens for incoming connections, but then reject
      the mount request.

    Revision 1.6  2003/03/09 05:38:03  nehresma
    moved globals to help prevent possible namespace issues

    Revision 1.4  2003/03/08 21:24:34  nehresma
    changes:
      - renamed functions to be nnfs_internal_*
      - added XHashTable stuff for fhandle to path (and back) lookups
      - added config file stuff for export lists - I will rename some ambiguous
        stuff in here later (called exports "mount points" in a couple places)

    Revision 1.3  2003/03/04 00:39:53  nehresma
    couple typo fixes

    Revision 1.2  2003/03/03 09:32:00  jorupp
     * added some stuff to the NFS driver
       * prototypes for all NFS and mount RPC calls
       * table for mapping to those calls
       * extremely basic implimentation of 1 mount function and 1 nfs function
         * this is enough for linux mount:
    	   * connect
    	   * get a file handle for the root node
    	   * ask for attributes on that file handle
    	   * get an error while gettings those attributes
    	 * command is: mount -t nfs localhost:/ nfsroot -o port=5001,mountport=5000
     * note: the .x files aren't required, but that's the source I used for rpcgen to
     	build the basis of the .c and .h files in the nfs directory.  The header was
    	modified quite a bit though (the _xdr.c just had the header line and one function
    	changed)
     * also: some of the attribute names had to be changed (usually just added an nfs
    	the front) so they wouldn't conflict with standard C symbols (ie. stat, timeval, etc.)
     * there's still several data dumps in the code, and if you know the RFCs, you could actually
        figure out what's being sent back to the server from those....

    Revision 1.1  2003/02/23 21:56:59  jorupp
     * added configure and makefile support for net_nfs
     * added basic shell of net_nfs, including the XDR parsing of the RPC headers
       -- it will listen for requests (mount & nfs) and print out some of the decoded RPC headers


 **END-CVSDATA***********************************************************/

#define MAX_PACKET_SIZE 16384

typedef unsigned int inode;

typedef struct
    {
    struct sockaddr_in source;
    int xid;
    int procedure;
    int user;
    void *param;
    } QueueEntry, *pQueueEntry;

typedef struct
    {
    inode lockedInode;
    XRingQueue waitingRequests;
    pThread thread;
    } ThreadInfo, *pThreadInfo;

/** definition for the exports listed in the config file **/
typedef struct
    {
    char *path;
    /** other things such as hosts, flags, etc. go here eventually **/
    } Exports, *pExports;

typedef struct
    {
    char *host;
    char *path;
    } Mount, *pMount;

typedef struct
    {
    pObject obj;
    struct timeval lastused;
    int inode;
    } ObjectUse, *pObjectUse;

#include "xarray.h"
#include "mtask.h"

/*** GLOBALS ***/
struct 
    {
    int numThreads;
    int queueSize;
    XRingQueue queue;
    pSemaphore semaphore;
    pFile nfsSocket;
    pXArray exportList;
    pXArray mountList;
    pXHashTable fhToPath;
    pXHashTable pathToFh;
    int nextFileHandle;
    char *inodeFile;
    pXArray openObjects;
    pObjSession objSess;
    pThreadInfo threads;
    }
    NNFS;

pObject nnfs_internal_open_inode(int inode);

void
nnfs_internal_dump_buffer(unsigned char* buf, int len)
    {
    int i;
    printf("Dumping %i byte buffer at %p\n",len,buf);
    if(len%4)
        printf("WARNING: %i bytes is not a multiple of 4!\n",len);
#define GOODC(c) ( (c>=' ' && c<='~') ?c:'.' )
    for(i=0;i<len;i+=4)
	{
	printf("%5i  %02x %02x %02x %02x  %c%c%c%c\n",i
	    ,(unsigned char)buf[i],(unsigned char)buf[i+1],(unsigned char)buf[i+2],(unsigned char)buf[i+3]
	    ,GOODC(buf[i]),GOODC(buf[i+1]),GOODC(buf[i+2]),GOODC(buf[i+3])
	    );
	}
    }

/** typedef for the functions that impliment the individual RPC calls **/
typedef void* (*rpc_func)(void*);

/** structure to hold the parameter info for each function **/
struct rpc_struct
    {
    rpc_func func;	/** reference to the function that impliements this procedure **/
    xdrproc_t ret;	/** the xdr_* function for the return value **/
    int ret_size;	/** the size of the return value (to be passed to nmMalloc()) **/
    xdrproc_t param;	/** the xdr_* function for the parameter **/
    int param_size;	/** the size of the parameter **/
    };

/** function prototypes for the mountd program **/
void* nnfs_internal_mountproc_null(void*);
fhstatus* nnfs_internal_mountproc_mnt(dirpath*);
mountlist* nnfs_internal_mountproc_dump(void*);
void* nnfs_internal_mountproc_umnt(dirpath*);
void* nnfs_internal_mountproc_umntall(void*);
exportlist* nnfs_internal_mountproc_export(void*);

/** the program information for mountd **/
struct rpc_struct mount_program[] = 
    {
	{
	(rpc_func)nnfs_internal_mountproc_null,
	(xdrproc_t) xdr_void, sizeof(void),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    ,
	{
	(rpc_func)nnfs_internal_mountproc_mnt,
	(xdrproc_t) xdr_fhstatus, sizeof(fhstatus),
	(xdrproc_t) xdr_dirpath, sizeof(dirpath),
	}
    ,
	{
	(rpc_func)nnfs_internal_mountproc_dump,
	(xdrproc_t) xdr_mountlist, sizeof(mountlist),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    ,
	{
	(rpc_func)nnfs_internal_mountproc_umnt,
	(xdrproc_t) xdr_void, sizeof(void),
	(xdrproc_t) xdr_dirpath, sizeof(dirpath),
	}
    ,
	{
	(rpc_func)nnfs_internal_mountproc_umntall,
	(xdrproc_t) xdr_void, sizeof(void),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    ,
	{
	(rpc_func)nnfs_internal_mountproc_export,
	(xdrproc_t) xdr_exportlist, sizeof(exportlist),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    };

/** the number of procedures in mount**/
int num_mount_procs = sizeof(mount_program)/sizeof(struct rpc_struct);

/** function prototypes for the nfs program **/
void* nnfs_internal_nfsproc_null(void*);
attrstat* nnfs_internal_nfsproc_getattr(fhandle*);
attrstat* nnfs_internal_nfsproc_setattr(sattrargs*);
void* nnfs_internal_nfsproc_root(void*);
diropres* nnfs_internal_nfsproc_lookup(diropargs*);
readlinkres* nnfs_internal_nfsproc_readlink(fhandle*);
readres* nnfs_internal_nfsproc_read(readargs*);
void* nnfs_internal_nfsproc_writecache(void*);
attrstat* nnfs_internal_nfsproc_write(writeargs*);
diropres* nnfs_internal_nfsproc_create(createargs*);
nfsstat* nnfs_internal_nfsproc_remove(diropargs*);
nfsstat* nnfs_internal_nfsproc_rename(renameargs*);
nfsstat* nnfs_internal_nfsproc_link(linkargs*);
nfsstat* nnfs_internal_nfsproc_symlink(symlinkargs*);
diropres* nnfs_internal_nfsproc_mkdir(createargs*);
nfsstat* nnfs_internal_nfsproc_rmdir(diropargs*);
readdirres* nnfs_internal_nfsproc_readdir(readdirargs*);
statfsres* nnfs_internal_nfsproc_statfs(fhandle*);

// :'<,'>s/\(.*\)\* \(.*\)(\(.*\)\*);/^I{^M^I(rpc_func) \2,^M^I(xdrproc_t) xdr_\1, sizeof(\1),^M^I(xdrproc_t) xdr_\3, sizeof(\3),^M^I}^M    ,
/** the program information for nfs (made with the above vim command from the function prototypes) **/
struct rpc_struct nfs_program[] = 
    {
	{
	(rpc_func) nnfs_internal_nfsproc_null,
	(xdrproc_t) xdr_void, sizeof(void),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_getattr,
	(xdrproc_t) xdr_attrstat, sizeof(attrstat),
	(xdrproc_t) xdr_fhandle, sizeof(fhandle),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_setattr,
	(xdrproc_t) xdr_attrstat, sizeof(attrstat),
	(xdrproc_t) xdr_sattrargs, sizeof(sattrargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_root,
	(xdrproc_t) xdr_void, sizeof(void),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_lookup,
	(xdrproc_t) xdr_diropres, sizeof(diropres),
	(xdrproc_t) xdr_diropargs, sizeof(diropargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_readlink,
	(xdrproc_t) xdr_readlinkres, sizeof(readlinkres),
	(xdrproc_t) xdr_fhandle, sizeof(fhandle),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_read,
	(xdrproc_t) xdr_readres, sizeof(readres),
	(xdrproc_t) xdr_readargs, sizeof(readargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_writecache,
	(xdrproc_t) xdr_void, sizeof(void),
	(xdrproc_t) xdr_void, sizeof(void),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_write,
	(xdrproc_t) xdr_attrstat, sizeof(attrstat),
	(xdrproc_t) xdr_writeargs, sizeof(writeargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_create,
	(xdrproc_t) xdr_diropres, sizeof(diropres),
	(xdrproc_t) xdr_createargs, sizeof(createargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_remove,
	(xdrproc_t) xdr_nfsstat, sizeof(nfsstat),
	(xdrproc_t) xdr_diropargs, sizeof(diropargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_rename,
	(xdrproc_t) xdr_nfsstat, sizeof(nfsstat),
	(xdrproc_t) xdr_renameargs, sizeof(renameargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_link,
	(xdrproc_t) xdr_nfsstat, sizeof(nfsstat),
	(xdrproc_t) xdr_linkargs, sizeof(linkargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_symlink,
	(xdrproc_t) xdr_nfsstat, sizeof(nfsstat),
	(xdrproc_t) xdr_symlinkargs, sizeof(symlinkargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_mkdir,
	(xdrproc_t) xdr_diropres, sizeof(diropres),
	(xdrproc_t) xdr_createargs, sizeof(createargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_rmdir,
	(xdrproc_t) xdr_nfsstat, sizeof(nfsstat),
	(xdrproc_t) xdr_diropargs, sizeof(diropargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_readdir,
	(xdrproc_t) xdr_readdirres, sizeof(readdirres),
	(xdrproc_t) xdr_readdirargs, sizeof(readdirargs),
	}
    ,
	{
	(rpc_func) nnfs_internal_nfsproc_statfs,
	(xdrproc_t) xdr_statfsres, sizeof(statfsres),
	(xdrproc_t) xdr_fhandle, sizeof(fhandle),
	}
    };

/** the number of procedures in nfs **/
int num_nfs_procs = sizeof(nfs_program)/sizeof(struct rpc_struct);



int
nnfs_internal_get_fhandle(fhandle fh, const dirpath path)
    {
    char *result;
    char *my_fh;
    union 
	{
	int fhi;
	fhandle fhc;
	} fhandle_c;

    /** First lets look in the hash to see if this path has already 
      * been given a unique inode or file handle. **/
    result=xhLookup(NNFS.pathToFh, path);
    if (!result)
	{
	/** need to keep this memory around after this function returns **/
	char *p;
	p = strdup(path);
	printf("path %s not found in hash\n",path);
	memset(fhandle_c.fhc,0,FHSIZE);
	fhandle_c.fhi=NNFS.nextFileHandle++;
	my_fh = (char*)nmMalloc(FHSIZE);
	strncpy(my_fh,fhandle_c.fhc,FHSIZE);

	/** add to both hashes here **/
	xhAdd(NNFS.fhToPath,my_fh,p);
	xhAdd(NNFS.pathToFh,p,my_fh);

	strncpy(fh,fhandle_c.fhc,FHSIZE);
	return 0;
	}
    else
	{
	strncpy(fhandle_c.fhc,result,FHSIZE);
	strncpy(fh,result,FHSIZE);
	printf("path %s found in hash: %d\n",path,fhandle_c.fhi);
	return 0;
	}

    return -1;
    }

int
nnfs_internal_get_path(dirpath *path, const fhandle fh)
    {
    char *result;

    result = xhLookup(NNFS.fhToPath, (char*)fh);
    if (!result)
	return -1;
    path=&result;
    return 0;
    }

//'<,'>s/\(.*\)\* \(.*\)(\(.*\));/\1\* \2(\3 param)^M    {^M    \1\* retval = NULL;^M    retval = (\1\*)nmMalloc(sizeof(\1));^M\/** do work here **\/^M    ^M    return retval;^M    }^M
/** the functions that impliement mount (made using the above vim command from the prototypes) **/
void* nnfs_internal_mountproc_null(void* param)
    {
    return NULL;
    }

fhstatus* nnfs_internal_mountproc_mnt(dirpath* param)
    {
    int i,f;
    fhstatus* retval = NULL;
    pExports exp;

    retval = (fhstatus*)nmMalloc(sizeof(fhstatus));
    if(!retval)
	return NULL;
    memset(retval,0,sizeof(fhstatus));

    printf("mount request recieved for: %s\n",*param);
    
    /** check the exportList **/
    for (i=0,f=0;i<xaCount(NNFS.exportList);i++)
	{
	exp=(pExports)xaGetItem(NNFS.exportList,i);
	if (!strcmp(exp->path,*param))
	    {
	    f=1;
	    break;
	    }
	}
    if (!f)
	{
	printf("mount point %s is not exported\n",*param);
	retval->status = ENODEV;
	return retval;
	}
    
    i=nnfs_internal_get_fhandle(retval->fhstatus_u.directory, *param);
    if(i==-1)
	retval->status = ENODEV;
    else
	retval->status = 0;
    
    return retval;
    }

mountlist* nnfs_internal_mountproc_dump(void* param)
    {
    mountlist* head = NULL;
    mountlist_item* prev = NULL;
    mountlist_item* cur = NULL;
    pMount mnt;
    int i;

    head=(mountlist*)nmMalloc(sizeof(mountlist));
    *head=NULL;

    for (i=0;i<xaCount(NNFS.mountList);i++)
	{
	mnt=xaGetItem(NNFS.mountList,i);
	cur=(mountlist_item*)malloc(sizeof(mountlist_item));
	cur->hostname=(name)malloc(sizeof(mnt->host)+1);
	cur->directory=(dirpath)malloc(sizeof(mnt->path)+1);
	strcpy(cur->hostname,mnt->host);
	strcpy(cur->directory,mnt->path);
	cur->nextentry=NULL;
	if (prev)
	    prev->nextentry=cur;
	else
	    *head=cur;
	prev=cur;
	}
    
    return head;
    }

/** this function's work is done in the mount listener since we need to know the
  * host for which we are unmounting **/
void* nnfs_internal_mountproc_umnt(dirpath* param)
    {
    return NULL;
    }

/** this function's work is done in the mount listener since we need to know the
  * host for which we are unmounting **/
void* nnfs_internal_mountproc_umntall(void* param)
    {
    return NULL;
    }

exportlist* nnfs_internal_mountproc_export(void* param)
    {
    exportlist* head = NULL;
    exportlist_item* prev = NULL;
    exportlist_item* cur = NULL;
    pExports exp;
    int i;

    head=(exportlist*)nmMalloc(sizeof(exportlist));
    *head=NULL;

    for (i=0;i<xaCount(NNFS.exportList);i++)
	{
	exp=(pExports)xaGetItem(NNFS.exportList,i);
	cur=(exportlist)malloc(sizeof(exportlist_item));
	cur->filesys=(dirpath)malloc(sizeof(exp->path)+1);
	strcpy(cur->filesys,exp->path);
	/** FIXME **/
	cur->groups=NULL;
	/** END FIXME **/
	cur->next=NULL;
	if (prev)
	    prev->next=cur;
	else
	    *head=cur;
	prev=cur;
	}
    
    return head;
    }

/** the functions that impliment nfs **/
void* nnfs_internal_nfsproc_null(void* param)
    {
    void* retval = NULL;
    /** do work here **/
    
    return retval;
    }

attrstat* nnfs_internal_nfsproc_getattr(fhandle* param)
    {
    attrstat* retval = NULL;
    char *path;
    int i;
    retval = (attrstat*)nmMalloc(sizeof(attrstat));
    memset(retval,0,sizeof(attrstat));
    /** do work here **/

    i=nnfs_internal_get_path(&path, *param);
    if(i==-1)
	retval->status = 1; /** this should be the UNIX error **/
    else
	{
	retval->status = 0;
	retval->attrstat_u.attributes.type = NFDIR;
	retval->attrstat_u.attributes.mode = 0;
	retval->attrstat_u.attributes.nlink = 1;
	retval->attrstat_u.attributes.uid = 1;
	retval->attrstat_u.attributes.gid = 1;
	retval->attrstat_u.attributes.size = 0;
	retval->attrstat_u.attributes.blocksize = 0;
	retval->attrstat_u.attributes.rdev = 0;
	retval->attrstat_u.attributes.blocks = 0;
	retval->attrstat_u.attributes.fsid = 0;
	retval->attrstat_u.attributes.fileid = 0;
	retval->attrstat_u.attributes.atime.seconds = 0;
	retval->attrstat_u.attributes.atime.useconds = 0;
	retval->attrstat_u.attributes.ctime.seconds = 0;
	retval->attrstat_u.attributes.ctime.useconds = 0;
	retval->attrstat_u.attributes.mtime.seconds = 0;
	retval->attrstat_u.attributes.mtime.useconds = 0;
	}
    
    return retval;
    }

attrstat* nnfs_internal_nfsproc_setattr(sattrargs* param)
    {
    attrstat* retval = NULL;
    retval = (attrstat*)nmMalloc(sizeof(attrstat));
    /** do work here **/
    
    return retval;
    }

void* nnfs_internal_nfsproc_root(void* param)
    {
    void* retval = NULL;
    /** do work here **/
    
    return retval;
    }

diropres* nnfs_internal_nfsproc_lookup(diropargs* param)
    {
    diropres* retval = NULL;
    retval = (diropres*)nmMalloc(sizeof(diropres));
    /** do work here **/
    
    return retval;
    }

readlinkres* nnfs_internal_nfsproc_readlink(fhandle* param)
    {
    readlinkres* retval = NULL;
    retval = (readlinkres*)nmMalloc(sizeof(readlinkres));
    /** do work here **/
    
    return retval;
    }

readres* nnfs_internal_nfsproc_read(readargs* param)
    {
    readres* retval = NULL;
    retval = (readres*)nmMalloc(sizeof(readres));
    /** do work here **/
    
    return retval;
    }

void* nnfs_internal_nfsproc_writecache(void* param)
    {
    void* retval = NULL;
    /** do work here **/
    
    return retval;
    }

attrstat* nnfs_internal_nfsproc_write(writeargs* param)
    {
    attrstat* retval = NULL;
    retval = (attrstat*)nmMalloc(sizeof(attrstat));
    /** do work here **/
    
    return retval;
    }

diropres* nnfs_internal_nfsproc_create(createargs* param)
    {
    diropres* retval = NULL;
    retval = (diropres*)nmMalloc(sizeof(diropres));
    /** do work here **/
    
    return retval;
    }

nfsstat* nnfs_internal_nfsproc_remove(diropargs* param)
    {
    nfsstat* retval = NULL;
    retval = (nfsstat*)nmMalloc(sizeof(nfsstat));
    /** do work here **/
    
    return retval;
    }

nfsstat* nnfs_internal_nfsproc_rename(renameargs* param)
    {
    nfsstat* retval = NULL;
    retval = (nfsstat*)nmMalloc(sizeof(nfsstat));
    /** do work here **/
    
    return retval;
    }

nfsstat* nnfs_internal_nfsproc_link(linkargs* param)
    {
    nfsstat* retval = NULL;
    retval = (nfsstat*)nmMalloc(sizeof(nfsstat));
    /** do work here **/
    
    return retval;
    }

nfsstat* nnfs_internal_nfsproc_symlink(symlinkargs* param)
    {
    nfsstat* retval = NULL;
    retval = (nfsstat*)nmMalloc(sizeof(nfsstat));
    /** do work here **/
    
    return retval;
    }

diropres* nnfs_internal_nfsproc_mkdir(createargs* param)
    {
    diropres* retval = NULL;
    retval = (diropres*)nmMalloc(sizeof(diropres));
    /** do work here **/
    
    return retval;
    }

nfsstat* nnfs_internal_nfsproc_rmdir(diropargs* param)
    {
    nfsstat* retval = NULL;
    retval = (nfsstat*)nmMalloc(sizeof(nfsstat));
    /** do work here **/
    
    return retval;
    }

readdirres* nnfs_internal_nfsproc_readdir(readdirargs* param)
    {
    readdirres* retval = NULL;
    retval = (readdirres*)nmMalloc(sizeof(readdirres));
    /** do work here **/
    
    return retval;
    }

statfsres* nnfs_internal_nfsproc_statfs(fhandle* param)
    {
    statfsres* retval = NULL;
    retval = (statfsres*)nmMalloc(sizeof(statfsres));
    /** do work here **/

    /** not implimented **/
    retval->status = ENOSYS;
    
    return retval;
    }

/***
**** nnfs_internal_create_inode_map
***/
void
nnfs_internal_create_inode_map()
    {
    pFile inFile;
    pStructInf stInode;
    pStructInf nextHandle;
    int i;

    if(!NNFS.inodeFile)
	{
	return;
	}

    inFile = fdOpen(NNFS.inodeFile,O_RDONLY, 0600);
    if(!inFile)
	{
	mssError(1,"NNFS","Warning: cannot open %s: %s",NNFS.inodeFile,strerror(errno));
	return;
	}
    
    stInode = stParseMsg(inFile,0);
    if(!stInode)
	{
	mssError(1,"NNFS","Cannot parse %s",NNFS.inodeFile);
	return;
	}

    nextHandle = stLookup(stInode,"nextFileHandle");
    if(!nextHandle || stGetAttrValue(nextHandle,DATA_T_INTEGER,POD(&NNFS.nextFileHandle),0)<0)
	mssError(1,"NNFS","Unable to read nextFileHandle");

    for(i=0;i<stInode->nSubInf;i++)
	{
	pStructInf group,entry;

	char *name;
	fhandle *fh;
	int inode;

	group = stInode->SubInf[i];
	if(group && strcmp(group->Name,"nextFileHandle"))
	    {
	    entry = stLookup(group,"path");
	    if(!entry || stGetAttrValue(entry,DATA_T_STRING,POD(&name),0)<0)
		{
		mssError(0,"NNFS","Unable to read inode");
		continue;
		}
	    entry = stLookup(group,"inode");
	    if(!entry || stGetAttrValue(entry,DATA_T_INTEGER,POD(&inode),0)<0)
		{
		mssError(0,"NNFS","Unable to read inode for: %s",name);
		continue;
		}
	    name = strdup(name);
	    fh = (fhandle*)nmMalloc(sizeof(fhandle));
	    memset(fh,0,sizeof(fhandle));
	    *(int*)fh = inode;
	    xhAdd(NNFS.fhToPath,(char*)fh,name);
	    xhAdd(NNFS.pathToFh,name,(char*)fh);
	    }

	}

    stFreeInf(stInode);
    fdClose(inFile,0);
    }


/***
**** nnfs_internal_dump_inode_map
***/
void
nnfs_internal_dump_inode_map()
    {
    pFile outFile;
    pStructInf stInode;
    pStructInf nextHandle;
    int i;

    if(!NNFS.inodeFile)
	{
	mssError(0,"NNFS","No inode map file specified in configuration");
	return;
	}

    outFile = fdOpen(NNFS.inodeFile,O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if(!outFile)
	{
	mssError(1,"NNFS","Cannot open inode.map: %s",strerror(errno));
	return;
	}
    stInode = stCreateStruct("inode_map","system/inode-map");
    if(!stInode)
	{
	mssError(1,"NNFS","Cannot create StructInf");
	return;
	}
    /** how do we tell it to create version 2 structure files? **/
    //stInode->Flags |= ST_F_VERSION2;

    nextHandle = stAddAttr(stInode,"nextFileHandle");
    if(!nextHandle || stSetAttrValue(nextHandle, DATA_T_INTEGER, POD(&NNFS.nextFileHandle), 0) <0 )
	mssError(1,"NNFS","Unable to create nextFileHandle attribute");

    for(i=0;i<NNFS.fhToPath->nRows;i++)
	{
	pXHashEntry ptr;
	ptr = (pXHashEntry)xaGetItem(&(NNFS.fhToPath->Rows),i);
	while(ptr)
	    {
	    pStructInf group;
	    pStructInf entry;
	    group = stAddGroup(stInode,"node","system/inode");
	    if(group)
		{
		entry = stAddAttr(group,"path");
		if(!entry || stSetAttrValue(entry, DATA_T_STRING, POD(&(ptr->Data)), 0)<0 )
		    mssError(1,"NNFS","Unable to create path attribute");
		entry = stAddAttr(group,"inode");
		if(!entry || stSetAttrValue(entry, DATA_T_INTEGER, POD(ptr->Key), 0)<0)
		    mssError(1,"NNFS","Unable to create inode attribute");
		}
	    else
		{
		mssError(1,"NNFS","Unable to create inode group");
		}
	    ptr=ptr->Next;
	    }
	}

    if(stGenerateMsg(outFile,stInode,ST_F_VERSION2)<0)
	{
	mssError(1,"NNFS","Unable to write inode mapping");
	}

    fdClose(outFile,0);
    stFreeInf(stInode);
    }

/***
**** nnfs_internal_get_inode - given a pointer to an XDR structure and a reference
****   to the XDR function that created it, returns the inode referenced in the
****   request (0 if there is none)
***/
inode
nnfs_internal_get_inode(void* data, xdrproc_t func)
    {
    if(func==(xdrproc_t)xdr_void)
	return 0;
    if(func==(xdrproc_t)xdr_fhandle)
	return *(inode*)data;
    if(func==(xdrproc_t)xdr_sattrargs)
	return nnfs_internal_get_inode(&((sattrargs*)data)->file,(xdrproc_t)xdr_fhandle);
    if(func==(xdrproc_t)(xdrproc_t)xdr_diropargs)
	return nnfs_internal_get_inode(&((diropargs*)data)->dir,(xdrproc_t)xdr_fhandle);
    if(func==(xdrproc_t)(xdrproc_t)xdr_readargs)
	return nnfs_internal_get_inode(&((readargs*)data)->file,(xdrproc_t)xdr_fhandle);
    if(func==(xdrproc_t)(xdrproc_t)xdr_writeargs)
	return nnfs_internal_get_inode(&((writeargs*)data)->file,(xdrproc_t)xdr_fhandle);
    if(func==(xdrproc_t)(xdrproc_t)xdr_createargs)
	return 0;
    /** may have problems with destination on some of these -- not sure how to handle **/
    if(func==(xdrproc_t)(xdrproc_t)xdr_renameargs)
	return nnfs_internal_get_inode(&((renameargs*)data)->from,(xdrproc_t)xdr_diropargs);
    if(func==(xdrproc_t)(xdrproc_t)xdr_linkargs)
	return nnfs_internal_get_inode(&((linkargs*)data)->from,(xdrproc_t)xdr_fhandle);
    if(func==(xdrproc_t)(xdrproc_t)xdr_symlinkargs)
	return nnfs_internal_get_inode(&((symlinkargs*)data)->from,(xdrproc_t)xdr_diropargs);
    if(func==(xdrproc_t)(xdrproc_t)xdr_readdirargs)
	return nnfs_internal_get_inode(&((readdirargs*)data)->dir,(xdrproc_t)xdr_fhandle);
    mssError("NNFS",0,"Error getting inode from %p (%p)\n",data,func);
    return 0;
    }

/***
**** nnfs_internal_destroy_queueentry - frees all memory used by a queueentry
***/
void
nnfs_internal_destroy_queueentry(pQueueEntry entry)
    {
    xdr_free(nfs_program[entry->procedure].param,(char*)entry->param);
    nmFree(entry->param,nfs_program[entry->procedure].param_size);
    nmFree(entry,sizeof(QueueEntry));
    }

/***
**** nnfs_internal_request_handler - waits for and processes queued nfs requests
***/
void
nnfs_internal_request_handler(void* v)
    {
    int threadNum = *(int*)v;
    pThreadInfo cThread = &NNFS.threads[threadNum];
    char name[32];

    nmFree(v,sizeof(int));

    snprintf(name,32,"NFS-RH-#%i",threadNum);
    name[31]='\0';
    thSetName(thCurrent(),name);

    while(1)
	{
	struct rpc_msg msg_out;
	pQueueEntry entry;
	char *buf;
	XDR xdr_out;
	inode requestInode;

	memset(&msg_out,0,sizeof(struct rpc_msg));

	/** block until there's a request in the queue **/
	syGetSem(NNFS.semaphore,1,0);

	/** get the request **/
	entry = xrqDequeue(&NNFS.queue);

	printf("entry: %p\n",entry);
	printf("xid: %x\n",entry->xid);

	/** get the inode of the request **/
	requestInode = nnfs_internal_get_inode(entry->param,nfs_program[entry->procedure].param);
	if(requestInode)
	    {
	    int i;
	    for(i=0;i<NNFS.numThreads;i++)
		{
		if(NNFS.threads[i].lockedInode==requestInode)
		    {
		    /** thread 'i' has it locked -- give it up **/
		    if(xrqEnqueue(&NNFS.threads[i].waitingRequests,entry)!=0)
			mssError("NNFS",1,"Unable to give request on inode %i to thread %i",requestInode,i);
		    else
			printf("successfully gave request on %i to %i\n",requestInode,i);
		    entry = NULL;
		    break;
		    }
		}
	    }
	/** if we gave away the request (entry is null), jump back to the top of the while loop **/
	if(!entry)
	    continue;

	/** enqueue the initial request (this makes processing easier) **/
	if(xrqEnqueue(&(cThread->waitingRequests),entry)!=0)
	    {
	    mssError("NNFS",1,"Unable to enqueue request on %i",threadNum);
	    nnfs_internal_destroy_queueentry(entry);
	    continue;
	    }

	/** lock the inode **/
	cThread->lockedInode = requestInode;
	printf("%i locked %i\n",threadNum,requestInode);

	/** grab elements off the top of the queue until there are no more **/
	while((entry = xrqDequeue(&(cThread->waitingRequests))) != NULL)
	    {
	    printf("procedure: %i\n",entry->procedure);
	    msg_out.rm_xid = entry->xid;
	    msg_out.rm_direction = REPLY;
	    msg_out.rm_reply.rp_stat = MSG_ACCEPTED;
	    msg_out.rm_reply.rp_acpt.ar_stat = SUCCESS;
	    msg_out.rm_reply.rp_acpt.ar_results.where = nfs_program[entry->procedure].func(entry->param);
	    msg_out.rm_reply.rp_acpt.ar_results.proc = nfs_program[entry->procedure].ret;

	    buf = (char*)nmMalloc(MAX_PACKET_SIZE);
	    xdrmem_create(&xdr_out,buf,MAX_PACKET_SIZE,XDR_ENCODE);
	    if(!xdr_replymsg(&xdr_out,&msg_out))
		{
		mssError(0,"NNFS","unable to create message to send");
		}
	    else
		{
		int i;
		i = xdr_getpos(&xdr_out);
		//nnfs_internal_dump_buffer(buf,i);
		if(netSendUDP(NNFS.nfsSocket,buf,i,0,&(entry->source),NULL,0) == -1)
		    {
		    mssError(0,"NNFS","unable to send message: %s",strerror(errno));
		    }
		}

	    xdr_free((xdrproc_t)xdr_replymsg,(char*)&msg_out);
	    /** free the return value of the function we called**/
	    nmFree(msg_out.rm_reply.rp_acpt.ar_results.where,nfs_program[entry->procedure].ret_size);
	    xdr_destroy(&xdr_out);
	    nmFree(buf,MAX_PACKET_SIZE);
	    nnfs_internal_destroy_queueentry(entry);
	    }
	cThread->lockedInode = 0;
	}
    
    }

/*** nnfs_internal_nfs_listener - listens for and processes nfs requests
 ***   RFC 1094
 ***/
void
nnfs_internal_nfs_listener(void* v)
    {
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    int i;
    char *buf;
    char *outbuf;
    char *remotehost;
    int remoteport;
    struct sockaddr_in remoteaddr;


	/** Set the thread's name **/
	thSetName(NULL,"NFS Listener");

	/** Get our configuration **/
	strcpy(listen_port,"5001");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_nfs");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the UDP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "nfs_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    {
		    memccpy(listen_port, strval, 0, 31);
		    listen_port[31] = '\0';
		    }
		else
		    {
		    snprintf(listen_port,32,"%d",intval);
		    }
		}
	    }

    	/** Open the server listener socket. **/
	NNFS.nfsSocket = netListenUDP(listen_port, 0);
	if (!NNFS.nfsSocket) 
	    {
	    mssErrorErrno(1,"NNFS","Could not open nfs listener");
	    thExit();
	    }
	
	buf = (char*)nmMalloc(MAX_PACKET_SIZE);
	outbuf = (char*)nmMalloc(MAX_PACKET_SIZE);
	/** Loop, accepting requests **/
	while((i=netRecvUDP(NNFS.nfsSocket,buf,MAX_PACKET_SIZE,0,&remoteaddr,&remotehost,&remoteport)) != -1)
	    {
	    XDR xdr_in;
	    XDR xdr_out; // only used on error
	    struct rpc_msg msg_in;
	    struct rpc_msg msg_out; // only used on error
	    int wasError=1; // mark if there was an error
	    int isDup=0; // mark if this is a duplicate
	    pQueueEntry entry;

	    entry = (pQueueEntry)nmMalloc(sizeof(QueueEntry));
	    if(!entry) continue;
	    memset(entry,0,sizeof(QueueEntry));
	    entry->source = remoteaddr; // copy address

	    /** process packet **/
	    printf("%i bytes recieved from: %s:%i\n",i,remotehost,remoteport);
	    xdrmem_create(&xdr_in,buf,i,XDR_DECODE);
	    xdrmem_create(&xdr_out,outbuf,MAX_PACKET_SIZE,XDR_ENCODE);
	    if(!xdr_callmsg(&xdr_in,&msg_in))
		{
		mssError(0,"NNFS","unable to retrieve message");
		xdr_destroy(&xdr_in);
		nmFree(entry,sizeof(QueueEntry));
		continue;
		}
	    if(msg_in.rm_direction==CALL)
		{
		int i;
		/** note: ignoring authorization for now... **/
		//printf("auth flavor: %i\n",msg_in.rm_call.cb_cred.oa_flavor);
		//printf("bytes of auth data: %u\n",msg_in.rm_call.cb_cred.oa_length);
		entry->xid = msg_out.rm_xid = msg_in.rm_xid;
#if 0
		if(NNFS.nextIn < NNFS.nextOut)
		    {
		    for(i=NNFS.nextOut;i<NNFS.queueSize;i++)
			if(NNFS.queue[i]->xid == entry->xid)
			    isDup = 1;
		    for(i=0;i<NNFS.nextIn;i++)
			if(NNFS.queue[i]->xid == entry->xid)
			    isDup = 1;
		    }
		for(i=NNFS.nextOut;i<NNFS.nextIn;i++)
		    if(NNFS.queue[i]->xid == entry->xid)
			isDup = 1;
#endif
		if(isDup==0)
		    {
		    msg_out.rm_direction = REPLY;
		    if(msg_in.rm_call.cb_rpcvers == 2)
			{
			msg_out.rm_reply.rp_stat = MSG_ACCEPTED;
			if(msg_in.rm_call.cb_prog == NFS_PROGRAM)
			    {
			    if(msg_in.rm_call.cb_vers == NFS_VERSION)
				{
				if(msg_in.rm_call.cb_proc < num_nfs_procs)
				    {
				    entry->procedure = msg_in.rm_call.cb_proc;
				    entry->param = (void*)nmMalloc(nfs_program[entry->procedure].param_size);
				    memset(entry->param,0,nfs_program[entry->procedure].param_size);
				    if(nfs_program[entry->procedure].param(&xdr_in,(char*)entry->param))
					{
					if(xrqCount(&NNFS.queue)<NNFS.queueSize && xrqEnqueue(&NNFS.queue,entry)==0)
					    {
					    wasError=0;
					    syPostSem(NNFS.semaphore,1,0);
					    }
					else
					    {
					    mssError(0,"NNFS","no more room in queue");
					    /** no good error -- send GARBAGE_ARGS I guess **/
					    msg_out.rm_reply.rp_acpt.ar_stat = GARBAGE_ARGS;
					    xdr_free(nfs_program[entry->procedure].param,entry->param);
					    nmFree(entry->param,nfs_program[entry->procedure].param_size);
					    }
					}
				    else
					{
					mssError(0,"NNFS","unable to parse parameters");
					msg_out.rm_reply.rp_acpt.ar_stat = GARBAGE_ARGS;
					nmFree(entry->param,nfs_program[entry->procedure].param_size);
					}
				    }
				else
				    {
				    mssError(0,"NNFS","Bad mountd procedure requested: %i\n",msg_in.rm_call.cb_proc);
				    msg_out.rm_reply.rp_acpt.ar_stat = PROC_UNAVAIL;
				    }
				}
			    else
				{
				mssError(0,"Invalid mount version requested: %i\n",msg_in.rm_call.cb_vers);
				msg_out.rm_reply.rp_acpt.ar_stat = PROG_MISMATCH;
				msg_out.rm_reply.rp_acpt.ar_vers.low = MOUNTVERS;
				msg_out.rm_reply.rp_acpt.ar_vers.high = MOUNTVERS;
				}
			    }
			else
			    {
			    mssError(0,"Invalid program requested: %i\n",msg_in.rm_call.cb_prog);
			    msg_out.rm_reply.rp_acpt.ar_stat = PROG_UNAVAIL;
			    }
			}
		    else
			{
			mssError(0,"Invalid RPC version requested: %i\n",msg_in.rm_call.cb_rpcvers);
			msg_out.rm_reply.rp_stat = MSG_DENIED;
			msg_out.rm_reply.rp_rjct.rj_vers.low = 2;
			msg_out.rm_reply.rp_rjct.rj_vers.high = 2;
			}
		    }
		else
		    {
		    printf("recieved duplicate request: %i\n",entry->xid);
		    }
		if(wasError==1 && isDup==0)
		    {
		    if(!xdr_replymsg(&xdr_out,&msg_out))
			{
			mssError(0,"NNFS","unable to create message to send");
			}
		    else
			{
			int i;
			i=xdr_getpos(&xdr_out);
//			nnfs_internal_dump_buffer(outbuf,i);
			if(netSendUDP(NNFS.nfsSocket,outbuf,i,0,&remoteaddr,NULL,0) == -1)
			    {
			    mssError(0,"NNFS","unable to send message: %s",strerror(errno));
			    }
			}
		    }
		}
	    else if(msg_in.rm_direction==REPLY)
		{
		/** it's a reply message.... -- ignore it **/
		mssError(0,"reply message recieved, but we shouldn't get one.....");
		}
	    else
		{
		mssError(0,"NNFS","invalid message direction: %i",msg_in.rm_direction);
		}
	    xdr_destroy(&xdr_out);
	    xdr_destroy(&xdr_in); // param is still valid, even after the XDR it came from is destroyed
	    if(wasError==1 && isDup==0)
		{
		/** might not need **/
		xdr_free((xdrproc_t)&xdr_replymsg,(char*)&msg_out);
		nmFree(entry,sizeof(QueueEntry));
		}
	    }
	nmFree(buf,MAX_PACKET_SIZE);
	nmFree(outbuf,MAX_PACKET_SIZE);

	/** Exit. **/
	mssError(1,"NNFS","Could not continue to accept requests.");
	netCloseTCP(NNFS.nfsSocket,0,0);

    thExit();
    }

/** Read the export list from the config file **/
void nnfs_internal_get_exports(pStructInf inf)
    {
    int i;
    char* path;
    pExports exp;

    /** If we want to provide "reread" functionality in the future, be
      * sure to get rid of the old mount list data first. **/
    NNFS.exportList=(pXArray)nmMalloc(sizeof(XArray));
    xaInit(NNFS.exportList, 2);
    for (i=0;i<inf->nSubInf;i++)
	{
	path=NULL;
	stAttrValue(stLookup(inf->SubInf[i],"path"),NULL,&path,0);

	if (!path)
	    {
	    mssError(1,"NNFS","Mount point '%s' must have a path defined.",inf->SubInf[i]->Name);
	    continue;
	    }

	exp=(pExports)malloc(sizeof(Exports));
	exp->path=path;

	xaAddItem(NNFS.exportList,exp);
	}
    }

/*** nnfs_internal_mount_listener - listens for and processes mount requests
 ***   RFC 1094
 ***/
void
nnfs_internal_mount_listener(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    pStructInf mp_config;
    char listen_port[32];
    char* strval;
    int intval;
    int i;
    char *buf;
    char *outbuf;
    char *remotehost;
    int remoteport;
    struct sockaddr_in remoteaddr;

	/** Set the thread's name **/
	thSetName(NULL,"MOUNTD Listener");

	/** Get our configuration **/
	strcpy(listen_port,"5000");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_nfs");
	if (!my_config)
	    {
	    mssError(1,"NNFS","No configuration for net_nfs in config file.");
	    }

	/** Got the config.  Now lookup what the UDP port is that we listen on **/
	strval=NULL;
	if (stAttrValue(stLookup(my_config, "mount_port"), &intval, &strval, 0) >= 0)
	    {
	    if (strval)
	        {
		memccpy(listen_port, strval, 0, 31);
		listen_port[31] = '\0';
		}
	    else
		{
		snprintf(listen_port,32,"%d",intval);
		}
	    }

	/** Look for mountpoints **/
	mp_config=NULL;
	mp_config=stLookup(my_config, "exports");
	if (!mp_config)
	    {
	    mssError(1,"NNFS","No mount points defined in config file");
	    }
	else
	    {
	    nnfs_internal_get_exports(mp_config);
	    }
	NNFS.mountList=(pXArray)nmMalloc(sizeof(XArray));
	xaInit(NNFS.mountList,4);
	
    	/** Open the server listener socket. **/
	listen_socket = netListenUDP(listen_port, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NNFS","Could not open mount listener");
	    thExit();
	    }
	
	buf = (char*)nmMalloc(MAX_PACKET_SIZE);
	outbuf = (char*)nmMalloc(MAX_PACKET_SIZE);
	/** Loop, accepting requests **/
	while((i=netRecvUDP(listen_socket,buf,MAX_PACKET_SIZE,0,&remoteaddr,&remotehost,&remoteport)) != -1)
	    {
	    void *ret=NULL; // pointer to results returned
	    int ret_size=0; // how much memory is nmMalloc()ed at ret
	    XDR xdr_in;
	    XDR xdr_out;
	    struct rpc_msg msg_in;
	    struct rpc_msg msg_out;
	    /** process packet **/

	    printf("%i bytes recieved from: %s:%i\n",i,remotehost,remoteport);
	    xdrmem_create(&xdr_in,buf,i,XDR_DECODE);
	    xdrmem_create(&xdr_out,outbuf,MAX_PACKET_SIZE,XDR_ENCODE);
	    if(!xdr_callmsg(&xdr_in,&msg_in))
		{
		mssError(0,"NNFS","unable to retrieve message");
		xdr_destroy(&xdr_in);
		continue;
		}
	    if(msg_in.rm_direction==CALL)
		{
		/** note: ignoring authorization for now... **/
		//printf("auth flavor: %i\n",msg_in.rm_call.cb_cred.oa_flavor);
		//printf("bytes of auth data: %u\n",msg_in.rm_call.cb_cred.oa_length);
		msg_out.rm_xid = msg_in.rm_xid;
		msg_out.rm_direction = REPLY;
		if(msg_in.rm_call.cb_rpcvers == 2)
		    {
		    msg_out.rm_reply.rp_stat = MSG_ACCEPTED;
		    if(msg_in.rm_call.cb_prog == MOUNTPROG)
			{
			if(msg_in.rm_call.cb_vers == MOUNTVERS)
			    {
			    if(msg_in.rm_call.cb_proc < num_mount_procs)
				{
				int procnum = msg_in.rm_call.cb_proc;
				void *param;
				pMount mnt;
				param = (void*)nmMalloc(mount_program[procnum].param_size);
				memset(param,0,mount_program[procnum].param_size);
				if(mount_program[procnum].param(&xdr_in,param))
				    {
				    ret = mount_program[procnum].func(param);
				    ret_size = mount_program[procnum].ret_size;
				    msg_out.rm_reply.rp_acpt.ar_stat = SUCCESS;
				    msg_out.rm_reply.rp_acpt.ar_results.where = ret;
				    msg_out.rm_reply.rp_acpt.ar_results.proc = mount_program[procnum].ret;

				    /** Ideally this stuff would be in nnfs_internal_* but to perform these ops, we
				      * must know the remote host. **/
				    switch (procnum)
					{
					case MOUNTPROC_MNT:
					    /** only add to the mountList on success **/
					    if (!(*(fhstatus*)ret).status)
						{
						/** add to the list of mounts **/
						mnt=(pMount)nmMalloc(sizeof(Mount));
						mnt->path=(char*)nmMalloc(strlen(*(char**)param)+1);
						mnt->host=(char*)nmMalloc(strlen(remotehost)+1);
						strncpy(mnt->path,*(char**)param,strlen(*(char**)param)+1);
						strncpy(mnt->host,remotehost,strlen(remotehost)+1);
						xaAddItem(NNFS.mountList,mnt);
						}
					    break;
					case MOUNTPROC_UMNT:
					    {
					    int j;
					    char *dir = *(char**)param;
					    /** remove the mount point from the list **/
					    for (j=0;j<xaCount(NNFS.mountList);j++)
						{
						pMount mnt=(pMount)xaGetItem(NNFS.mountList,j);
						if (!(strcmp(mnt->host,remotehost)) && !(strcmp(mnt->path,dir)))
						    {
						    xaRemoveItem(NNFS.mountList,j);
						    nmFree(mnt->host,strlen(mnt->host)+1);
						    nmFree(mnt->path,strlen(mnt->path)+1);
						    nmFree(mnt,sizeof(Mount));
						    break;
						    }
						}
					    }
					    break;
					case MOUNTPROC_UMNTALL:
					    {
					    int j;
					    char *dir = *(char**)param;
					    /** remove the all mount points from remotehost from the list **/
					    for (j=0;j<xaCount(NNFS.mountList);j++)
						{
						pMount mnt=(pMount)xaGetItem(NNFS.mountList,j);
						if (!strcmp(mnt->host,remotehost))
						    {
						    xaRemoveItem(NNFS.mountList,j);
						    nmFree(mnt->host,strlen(mnt->host)+1);
						    nmFree(mnt->path,strlen(mnt->path)+1);
						    nmFree(mnt,sizeof(Mount));
						    }
						}
					    }
					    break;
					}
				    xdr_free(mount_program[procnum].param,param);
				    }
				else
				    {
				    mssError(0,"NNFS","unable to parse parameters");
				    msg_out.rm_reply.rp_acpt.ar_stat = GARBAGE_ARGS;
				    }
				nmFree(param,mount_program[procnum].param_size);
				}
			    else
				{
				mssError(0,"NNFS","Bad mountd procedure requested: %i\n",msg_in.rm_call.cb_proc);
				msg_out.rm_reply.rp_acpt.ar_stat = PROC_UNAVAIL;
				}
			    }
			else
			    {
			    mssError(0,"Invalid mount version requested: %i\n",msg_in.rm_call.cb_vers);
			    msg_out.rm_reply.rp_acpt.ar_stat = PROG_MISMATCH;
			    msg_out.rm_reply.rp_acpt.ar_vers.low = MOUNTVERS;
			    msg_out.rm_reply.rp_acpt.ar_vers.high = MOUNTVERS;
			    }
			}
		    else
			{
			mssError(0,"Invalid program requested: %i\n",msg_in.rm_call.cb_prog);
			msg_out.rm_reply.rp_acpt.ar_stat = PROG_UNAVAIL;
			}
		    }
		else
		    {
		    mssError(0,"Invalid RPC version requested: %i\n",msg_in.rm_call.cb_rpcvers);
		    msg_out.rm_reply.rp_stat = MSG_DENIED;
		    msg_out.rm_reply.rp_rjct.rj_vers.low = 2;
		    msg_out.rm_reply.rp_rjct.rj_vers.high = 2;
		    }
		if(!xdr_replymsg(&xdr_out,&msg_out))
		    {
		    mssError(0,"NNFS","unable to create message to send");
		    }
		else
		    {
		    int i;
		    i = xdr_getpos(&xdr_out);
//		    nnfs_internal_dump_buffer(outbuf,i);
		    if(netSendUDP(listen_socket,outbuf,i,0,&remoteaddr,NULL,0) == -1)
			{
			mssError(0,"NNFS","unable to send message: %s",strerror(errno));
			}
		    }
		xdr_free((xdrproc_t)&xdr_replymsg,(char*)&msg_out);
		/** if the return value wasn't null, free it **/
		if(ret)
		    nmFree(ret,ret_size);
		}
	    else if(msg_in.rm_direction==REPLY)
		{
		/** it's a reply message.... -- ignore it **/
		mssError(0,"reply message recieved, but we shouldn't get one.....");
		}
	    else
		{
		mssError(0,"NNFS","invalid message direction: %i",msg_in.rm_direction);
		}
	    xdr_free((xdrproc_t)xdr_callmsg,(char*)&msg_in);
	    xdr_destroy(&xdr_in);
	    xdr_destroy(&xdr_out);
	    }
	nmFree(buf,MAX_PACKET_SIZE);
	nmFree(outbuf,MAX_PACKET_SIZE);

	/** Exit. **/
	mssError(1,"NNFS","Mount listener could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }

/*** nnfs_internal_monitor_objects - keep objects open for 30 seconds unless
 *** they are being used.
 ***/
void nnfs_internal_monitor_objects()
    {
    int i;
    pObjectUse obj;
    struct timeval cur_time;

    NNFS.openObjects=(pXArray)nmMalloc(sizeof(XArray));
    xaInit(NNFS.openObjects, 8);
    while (1)
    	{
	thSleep(1000);
	gettimeofday(&cur_time, NULL);
	for (i=0;i<xaCount(NNFS.openObjects);i++)
	    {
	    /** examine time on each object **/
	    obj=(pObjectUse)xaGetItem(NNFS.openObjects,i);
	    if ((cur_time.tv_sec - obj->lastused.tv_sec) >= 30)
	    	{
		/** close the object, remove from cache, free memory **/
		objClose(obj->obj);
		xaRemoveItem(NNFS.openObjects,i);
		nmFree(obj,sizeof(ObjectUse));
		}
	    }
	}
    }

/***  Get a pObject for a specific inode from the cache, or open one up
 ***  if there isn't one already open.  Do _NOT_ close the returned pObject,
 ***  but let the monitor thread handle the closes.
 ***/
pObject nnfs_internal_open_inode(int inode)
    {
    int i;
    pObjectUse obj;
    int found=0;
    char *path;
    union 
	{
	int fhi;
	fhandle fhc;
	} fhandle_c;

    /* look through the open objects for this inode */
    for (i=0;i<xaCount(NNFS.openObjects);i++)
    	{
	obj=(pObjectUse)xaGetItem(NNFS.openObjects,i);
	if(obj->inode == inode)
	    {
    	    gettimeofday(&(obj->lastused), NULL);
	    return obj->obj;
	    }
	}

    /* grab a path for the inode */
    memset(fhandle_c.fhc,0,FHSIZE);
    fhandle_c.fhi=inode;
    path = xhLookup(NNFS.fhToPath, fhandle_c.fhc);
    if (!path)
    	{
	nmFree(obj,sizeof(ObjectUse));
	return NULL;
	}

    /* set the inode and lastused on the ObjectUse */
    obj=(pObjectUse)nmMalloc(sizeof(ObjectUse));
    obj->inode=inode;
    gettimeofday(&(obj->lastused), NULL);

    /** set the pObject 
     ** FIXME: is system/object the right type to use?
     **/
    obj->obj = objOpen(NNFS.objSess,path,O_RDWR|O_CREAT,0600,"system/object");
    if (!obj->obj)
    	{
	nmFree(obj,sizeof(ObjectUse));
	return NULL;
	}
    xaAddItem(NNFS.openObjects,(char*)obj);

    return obj->obj;
    }


/*** nnfsShutdownHandler - shutdown the NFS driver
***/
void
nnfsShutdownHandler()
    {
    int i;
    pObjectUse obj;

    mssError(0,"NNFS","NFS netdriver is shutting down");
    nnfs_internal_dump_inode_map();

    /** close the open objects **/
    for (i=0;i<xaCount(NNFS.openObjects);i++)
	{
	obj=(pObjectUse)xaGetItem(NNFS.openObjects,i);
	objClose(obj->obj);
	nmFree(obj,sizeof(ObjectUse));
	}
    objCloseSession(NNFS.objSess);
    }


/*** nhtInitialize - initialize the HTTP network handler and start the 
 *** listener thread.
 ***/
int
nnfsInitialize()
    {
    pStructInf my_config;
    int i;

	/** init global object **/
	memset(&NNFS,0,sizeof(NNFS));

	my_config = stLookup(CxGlobals.ParsedConfig, "net_nfs");
	if (my_config)
	    {
	    /** Got the config.  Now lookup how many threads to use **/
	    if (stAttrValue(stLookup(my_config, "num_threads"), &(NNFS.numThreads), NULL, 0) < 0)
		{
		NNFS.numThreads = 10;
		}
	    /** Now lookup how big the queue is **/
	    if (stAttrValue(stLookup(my_config, "queue_size"), &(NNFS.queueSize), NULL, 0) < 0)
		{
		NNFS.queueSize = 100;
		}
	    /** Now lookup where the inode map file should be **/
	    if (stAttrValue(stLookup(my_config, "inode_map"), NULL, &(NNFS.inodeFile) , 0) < 0)
		{
		NNFS.inodeFile=NULL;
		}
	    }
	else
	    {
	    NNFS.numThreads = 10;
	    NNFS.queueSize = 100;
	    NNFS.inodeFile=NULL;
	    }

	//NNFS.queue = (pQueueEntry*)nmMalloc(NNFS.queueSize*sizeof(pQueueEntry));
	//memset(NNFS.queue,0,NNFS.queueSize*sizeof(pQueueEntry));
	//NNFS.nextIn=0;
	//NNFS.nextOut=0;
	xrqInit(&NNFS.queue,16);
	NNFS.semaphore = syCreateSem(0,0);
	/** 0 is reserved **/
	NNFS.nextFileHandle=1;

	NNFS.fhToPath=(pXHashTable)nmMalloc(sizeof(XHashTable));
	NNFS.pathToFh=(pXHashTable)nmMalloc(sizeof(XHashTable));
	xhInit(NNFS.fhToPath,16,0);
	xhInit(NNFS.pathToFh,16,0);
	nnfs_internal_create_inode_map();

	/** add shutdown handler **/
	cxAddShutdownHandler(nnfsShutdownHandler);
	
	/** Start the mountd listener **/
	thCreate(nnfs_internal_mount_listener, 0, NULL);

	/** Start the nfs listener. **/
	thCreate(nnfs_internal_nfs_listener, 0, NULL);

	NNFS.threads = (pThreadInfo)nmMalloc(NNFS.numThreads*sizeof(ThreadInfo));
	if(!NNFS.threads)
	    {
	    mssError("NNFS",1,"Unable to get memory for threads");
	    return -1;
	    }

	/** Start the request handler(s) **/
	for(i=0;i<NNFS.numThreads;i++)
	    {
	    int *j;
	    j=(int*)nmMalloc(sizeof(int));
	    *j=i;
	    NNFS.threads[i].lockedInode=0;
	    xrqInit(&(NNFS.threads[i].waitingRequests),4);
	    NNFS.threads[i].thread=thCreate(nnfs_internal_request_handler, 0, j);
	    }

	NNFS.objSess=objOpenSession("/");

	/** Start the open object monitoring thread **/
	thCreate(nnfs_internal_monitor_objects, 0, NULL);
    return 0;
    }

MODULE_INIT(nnfsInitialize);
MODULE_PREFIX("nnfs");
MODULE_DESC("NFS Network Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

