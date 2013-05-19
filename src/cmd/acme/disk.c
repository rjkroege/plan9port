#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"

static	Block	*blist;

int
tempfile(void)
{
	char buf[128];
	int i, fd;

	snprint(buf, sizeof buf, "/tmp/X%d.%.4sacme", getpid(), getuser());
	for(i='A'; i<='Z'; i++){
		buf[5] = i;
		if(access(buf, AEXIST) == 0)
			continue;
		fd = create(buf, ORDWR|ORCLOSE|OCEXEC, 0600);
		if(fd >= 0)
			return fd;
	}
	return -1;
}

/*
	Creates a new Disk object and opens a temporary file to back
	this disk.

	I assume that each Disk has a backing file descriptor and
	hence backing temporary file. (Except that the file is not
	there. But let's assume that the ORCLOSE simply means that
	it's not linked anymore.)
*/
Disk*
diskinit()
{
	Disk *d;

	d = emalloc(sizeof(Disk));
	d->fd = tempfile();
	if(d->fd < 0){
		fprint(2, "acme: can't create temp file: %r\n");
		threadexitsall("diskinit");
	}
	return d;
}


/*

	Blocks are 
	
*/
static
uint
ntosize(uint n, uint *ip)
{
	uint size;

	if(n > Maxblock)
		error("internal error: ntosize");
	size = n;
	if(size & (Blockincr-1))
		size += Blockincr - (size & (Blockincr-1));
	/* last bucket holds blocks of exactly Maxblock */
	if(ip)
		*ip = size/Blockincr;
	return size * sizeof(Rune);
}

/*
	Makes a new Block of size n and associates it with the given Disk.

	Blocks vary in size by Blockincr from 1 to 32. It is forbidden to ask for
	a new Block bigger than 32  Blockincr (8k)

	A given size is rounded to the nearest number of even Blockincr. The Disk
	struct keeps an array of Block pointers. It is a bucket list. If there is a free
	Block on the bucket list already of the desired size, unlink from bucket
	list and return.

	Otherwise, manufacture 100 Blocks in the current Disk and grow that Disk's
	allocation of size.

	Remember that Block instances don't have any storage themselves. They
	manage storage kept elsewhere. Backed by tmp files I would think.

	Editorial: why are we doing manual storage management and not letting the
	kernel do it? This scheme does have the advantage of giving us a variable level
	of RAM backing via buffer cache.
*/
Block*
disknewblock(Disk *d, uint n)
{
	uint i, j, size;
	Block *b;

	size = ntosize(n, &i);
	b = d->free[i];
	if(b)
		d->free[i] = b->u.next;
	else{
		/* allocate in chunks to reduce malloc overhead */
		if(blist == nil){
			blist = emalloc(100*sizeof(Block));
			for(j=0; j<100-1; j++)
				blist[j].u.next = &blist[j+1];
		}
		b = blist;
		blist = b->u.next;
		b->addr = d->addr;
		if(d->addr+size < d->addr){
			error("temp file overflow");
		}
		d->addr += size;
	}
	b->u.n = n;
	return b;
}

/*
	Puts the given block back on the free list.
 */
void
diskrelease(Disk *d, Block *b)
{
	uint i;

	ntosize(b->u.n, &i);
	b->u.next = d->free[i];
	d->free[i] = b;
}

/*
	Writes the provided Rune buffer to the Disk's backing temp file.
	The buffer must fit in a single Block. Returns the block it fits into.
*/
void
diskwrite(Disk *d, Block **bp, Rune *r, uint n)
{
	int size, nsize;
	Block *b;

	b = *bp;
	size = ntosize(b->u.n, nil);
	nsize = ntosize(n, nil);
	if(size != nsize){
		diskrelease(d, b);
		b = disknewblock(d, n);
		*bp = b;
	}
	if(pwrite(d->fd, r, n*sizeof(Rune), b->addr) != n*sizeof(Rune))
		error("write error to temp file");
	b->u.n = n;
}

/*
	Fills the provided Rune buffer r with the contents of block
	held in the backing disk store.
*/
void
diskread(Disk *d, Block *b, Rune *r, uint n)
{
	if(n > b->u.n)
		error("internal error: diskread");

	ntosize(b->u.n, nil);
	if(pread(d->fd, r, n*sizeof(Rune), b->addr) != n*sizeof(Rune))
		error("read error from temp file");
}
