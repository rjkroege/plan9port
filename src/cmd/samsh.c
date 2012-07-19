#include <u.h>
#include <signal.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <9pclient.h>
#include <auth.h>
#include <thread.h>
#include <regexp.h>

typedef	struct	CurrentPathFid	CurrentPathFid;
typedef 	void (*PerIndexEntryFunction)(CurrentPathFid*, char*, char*, int);

struct CurrentPathFid {
	CFsys *fs;
	CFid *fid;
	char *path;
	int mode;
	char *wantedbuffer;
	int wantedwid;
	int wid;
};

int MAXARGS = 1000;
static char* CONS = "cons";
int MAXREAD = 1000;

CurrentPathFid* mountService(char*);
char* openPathAsNeeded(CurrentPathFid*, char*, int);
void xwrite(CFid*, int, char*);
void printIndexCore(CurrentPathFid*, char*, char*, int);
void findBufferCore(CurrentPathFid*, char*, char*, int);
void overAcmeIndex(CurrentPathFid*, char*, int, PerIndexEntryFunction);
void bCmdCore(CurrentPathFid*, char*);

/*
 * Mounts the requested service if possible and places the
 * requisite state in the CurrentPathFid object.
 */
CurrentPathFid*
mountService(char *name)
{
	CFsys *fs;
	CurrentPathFid* fidp;
	fidp = mallocz(sizeof(CurrentPathFid), 1);
	fs = nsamount(name, 0);
	if(fs == nil)
		sysfatal("mount: %r");
	fidp->fs = fs;
	fidp->wid = -1;
	return fidp;
}

/*
 * If the fid is not open or the path and/or mode has changed,
 * close the fid as necessary and open a new fid.
 * return the path if it's changed or nil.
 * Can retain a ref to path.
 */
char*
openPathAsNeeded(CurrentPathFid* pfid, char *path, int mode)
{
	char *oldpath = pfid->path;
	CFid *fid = pfid->fid;

	if (fid == nil || strcmp(path, oldpath) != 0 || pfid->mode != mode) {
		print("need to change the fid\n");
		if (fid) fsclose(fid);
		fid = fsopen(pfid->fs, path, mode);
		
		if(fid == nil)
			sysfatal("fsopen %s: %r", path);
		
		pfid->fid = fid;
		pfid->path = path;
		pfid->mode = mode;
		return oldpath;
	}
	print("didn't need to change the fid\n");
	return nil;
}

void
xwrite(CFid* fid, int n, char *s)
{
	if(fswrite(fid, s, n) != n)
		sysfatal("write error: %r");
}

void
printIndexCore(CurrentPathFid* pfid, char* ln, char* fn, int wid)
{
	/*
		Leading ` means the file is modified.
		+ means that the file has a window open. (always)
		 ' ' means that the file is not current. (i.e. the target of edits')
		 . means that the file is current (i.e. edit target)
	*/
	 if (access(fn, F_OK | R_OK) == 0)	/* Only show real files. */
		print("%c+%c %s\n",
				(ln[4 * 12 + 10] == '1') ? '`' : ' ',
				(pfid->wid == wid) ? '.' : ' ',
				ln + 5*12);
}

void
findBufferCore(CurrentPathFid* pfid, char* line, char* fn, int wid)
{
	if (strcmp(pfid->wantedbuffer, fn) == 0) {
		pfid->wantedwid = wid;
	}
}

// FIXME: always opens the index so why pass the path and mode in?
/**
 * Opens the index file, reads it, and passes each buffer name
 * and its window to the helper function f
 */
void
overAcmeIndex(CurrentPathFid* pfid, char* path, int mode, PerIndexEntryFunction f)
{
	Biobuf *bp;
	int fd;
	char *ln;
	int len, wid;
	char *p, *fn;

	bp = malloc(sizeof *bp);
	fd = fsopenfd(pfid->fs, path, mode);
	if (fd < 0)
			sysfatal("fsopenfd %s: %r", path);
	Binit(bp, fd, mode);

	while(ln = Brdstr(bp, '\n', 1)) {
		len = Blinelen(bp);
		
		/* Insert alternative middle here */
		/* extract name, modified state */
//		print( "<diag: %c, %c> -> %s\n", ln[3 * 12 + 10], ln[4 * 12 + 10], ln + 5*12);

		if (ln[3 * 12 + 10] == '0') {
			ln[12] = 0;					/* Window id */
			wid = atoi(ln);		

			fn = ln + 5 * 12;			/* Backing file name. */
			for(p = fn; p && *p != ' '; p++);	/* Doesn't handle filenames with spaces. */
			*p = 0;
			
			(*f)(pfid, ln, fn, wid);			/* Core op */
		}
		free(ln);
	}
	close(fd);
	free(bp);
}

void
bCmdCore(CurrentPathFid* pfid, char* wantedbuffer)
{
	char *b;

	pfid->wantedwid = -1;
	pfid->wantedbuffer = wantedbuffer;
	overAcmeIndex(pfid, "index", OREAD, findBufferCore);

	if (pfid->wantedwid >= 0) {			
		b = malloc(20);
		sprint(b, "%d/edit", pfid->wantedwid);
		b = openPathAsNeeded(pfid, b, OREAD | OWRITE);
		if (b != 0 && b != CONS) free(b);
		pfid->wid = pfid->wantedwid;
		pfid->wantedwid = -1;
	}						
}

void
threadmain(int argc, char **argv)
{
	Biobuf* input;
	char *line, *cmd;
	int len, i;
	CurrentPathFid* pfid;
	char *w[MAXARGS];
	char *b;
	int wc;
	char *rb;
	int rc;


	input = malloc(sizeof *input);
	pfid = mountService("acme");
	input = Bfdopen(0, OREAD);
	rb = malloc(MAXREAD);	

	while (1) {
		line = Brdstr(input, '\n', 1);
		len = Blinelen(input); 					// Can use this... pass length, deeper
		if (line == nil) 
			break;
		print("read, len %d <%s>\n", len, line);

		cmd = malloc(len + 1);				/* preserve line */
		strcpy(cmd, line);

		wc = tokenize(cmd, w, MAXARGS);

		/* Dump the tokens for now */
		for (i = 0; i < wc; i++) print("%d <%s>\n", i, w[i]);

		if (strcmp(w[0], "q") == 0)
			break;
		if (strcmp(w[0], "n") == 0) {
			overAcmeIndex(pfid, "index", OREAD, printIndexCore);
			goto end;
		}
		if (strcmp(w[0], "b") == 0 && wc == 2) {
			print("should be opening file <%s>\n", w[1]);
			bCmdCore(pfid, w[1]);
			goto end;
		}
		if (strcmp(w[0], "B") == 0 && wc == 2) {
			/* As coded, will only do the right thing with absolute paths. */
			print("B should be opening file <%s>\n", w[1]);
			int fc = rfork(RFFDG|RFPROC);
			if (fc == 0) {
				execl("plumb", "plumb", w[1], 0);	/* On pure Plan9, this should be "/bin/plumb" */
			} else {
				Waitmsg* wm;
				wm = waitfor(fc);
				if (wm->msg[0] == 0) 
					bCmdCore(pfid, w[1]);
				else
					fprint(2, "Couldn't open %s\n", w[1]);
				free(wm);
			}
			goto end;
		}

		/* Echo if there is no window to edit. */
		if (pfid->wid < 0) {
			b = openPathAsNeeded(pfid, CONS, OWRITE|OTRUNC);
			if (b != 0 && b != CONS) free(b);
			line[len++] = '\n';
			print("pre-write <%s>\n", line);
			xwrite(pfid->fid, len, line);
		} else {
			xwrite(pfid->fid, len, line);
			while (rc = fsread(pfid->fid, rb, MAXREAD)) {
				write(1, rb, rc);
			}
		}
	end:
		free(cmd);
		free(line);
	}
	
	free(input);
	fsclose(pfid->fid);
	threadexitsall(0);	
}

