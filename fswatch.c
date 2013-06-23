#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <CoreServices/CoreServices.h> 

/* fswatch.c
 * 
 * usage: ./fswatch /some/directory[:/some/otherdirectory:...]  
 *
 * compile me with something like: gcc fswatch.c -framework CoreServices -o fswatch
 *
 * adapted from the FSEvents api PDF
*/

extern char **environ;
//the command to run
int count_chars(const char* string, char ch)
{
    int count = 0;
    for(; *string; count += (*string++ == ch));
    return count;
}
// write out some info when there's any change in watched files
void callback( 
    ConstFSEventStreamRef streamRef, 
    void *clientCallBackInfo, 
    size_t numEvents, 
    void *eventPaths, 
    const FSEventStreamEventFlags eventFlags[], 
    const FSEventStreamEventId eventIds[]) 
{ 
  pid_t pid;
  int   status;

  for (int i=0; i<numEvents; ++i) {
	const char* path = ((char **)eventPaths)[i];
	int extra = count_chars(path, ' ');
	if (extra) { // produce escaped spaces in the paths
		char * z = malloc(strlen(path)+1+extra);
		int cur = 0, zcur = 0;
		while (*path) {
			if (*path == ' ')
				z[zcur++] = '\\';
			z[zcur++] = path[cur++];
		}
		printf("%x %s, ", eventFlags[i], z);
	} else {
		printf("%x %s, ", eventFlags[i], path);
	}
  }
  printf("\n");
  fflush(stdout);
} 
 
//set up fsevents and callback
int main(int argc, char **argv) {

  if(argc != 2) {
    fprintf(stderr, "You must specify a directory to watch\n");
    exit(1);
  }

  CFStringRef mypath = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8); 
  CFArrayRef pathsToWatch = CFStringCreateArrayBySeparatingStrings (NULL, mypath, CFSTR(":"));

  void *callbackInfo = NULL; 
  FSEventStreamRef stream; 
  CFAbsoluteTime latency = 1.0;

  stream = FSEventStreamCreate(NULL,
    &callback,
    callbackInfo,
    pathsToWatch,
    kFSEventStreamEventIdSinceNow,
    latency,
	kFSEventStreamCreateFlagFileEvents
  ); 

  FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode); 
  FSEventStreamStart(stream);
  CFRunLoopRun();

}
