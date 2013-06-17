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
  char **a = malloc(sizeof(char*) * numEvents);
  int *flags = malloc(sizeof(int) * numEvents);
  int c = 0;
  for (int i=0; i<numEvents; ++i) {
    int same = 0;
	for (int j=0; j < c; ++j) {
	  if (!strcmp(a[j], ((char **)eventPaths)[i])) {
		same = 1;
		printf("Found a dupe with %s\n", ((char **)eventPaths)[i]);
	  }
	}
	if (!same) {
	  a[c] = ((char **)eventPaths)[i];
	  flags[c] = eventFlags[i];
	  ++c;
	}
  }
  for (int i=0; i<c; ++i) {
	printf("%x %s; ", flags[i], a[i]);
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
