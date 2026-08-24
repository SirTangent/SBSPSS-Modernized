/*	Replaces the generated source/system/<VER>/<TERR>/<FS>/info.mip.
	The strings come from CMake (see port/CMakeLists.txt) so each build
	variant carries its own identity; the CD file shim uses them to locate
	out/<TERR>/<VER>/version/<FS>/BIGLUMP.BIN exactly like pcfile.cpp did.
*/
#ifndef SBSP_INFO_VERSION
#define SBSP_INFO_VERSION		"DEBUG"
#endif
#ifndef SBSP_INFO_TERRITORY
#define SBSP_INFO_TERRITORY		"USA"
#endif
#ifndef SBSP_INFO_FILESYSTEM
#define SBSP_INFO_FILESYSTEM	"CD"
#endif

extern char INF_Version[];
extern char INF_Territory[];
extern char INF_FileSystem[];

char INF_Version[]    = SBSP_INFO_VERSION;
char INF_Territory[]  = SBSP_INFO_TERRITORY;
char INF_FileSystem[] = SBSP_INFO_FILESYSTEM;
