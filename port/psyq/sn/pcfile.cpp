/*	libsn PC-file calls over the host filesystem (M3).

	On real hardware these talked to the SN target-debugger's host PC; here
	the host IS the PC, so they map straight onto the CRT.  Their only
	in-tree users are the DEBUG screen utils (SaveScreen's TGA writer and
	its FileExists probe, source/system/main.cpp) - flags follow the LIBSN.H
	contract: PCopen mode 0 read / 1 write / 2 read-write, PClseek whence
	0 set / 1 cur / 2 end, all returning -1 on error.
*/
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>

extern "C" {

int PCinit(void)
{
	return 0;
}

int PCopen(char *name, int flags, int perms)
{
	(void)perms;
	int oflag = _O_BINARY;
	switch (flags)
	{
	case 0:  oflag |= _O_RDONLY;  break;
	case 1:  oflag |= _O_WRONLY;  break;
	default: oflag |= _O_RDWR;    break;
	}
	return _open(name, oflag);
}

int PCcreat(char *name, int perms)
{
	(void)perms;
	return _open(name, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY,
				 _S_IREAD | _S_IWRITE);
}

int PClseek(int fd, int offset, int mode)
{
	return (int)_lseek(fd, offset, mode);	/* 0/1/2 == SEEK_SET/CUR/END */
}

int PCread(int fd, char *buff, int len)
{
	return _read(fd, buff, len);
}

int PCwrite(int fd, char *buff, int len)
{
	return _write(fd, buff, len);
}

int PCclose(int fd)
{
	return _close(fd);
}

}
