#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <CoreServices/CoreServices.h> 

/* fswatch.c
 *
 * usage: ./fswatch /some/directory[:/some/otherdirectory:...] [-r]
 *
 * r flag indicates that the returned paths should be direct relative paths. you 
 * should probably not use this when declaring multiple paths separated by 
 * colons, but you may if you want to. The behavior is currently only to convert 
 * the paths corresponding to the first item into relative paths.
 *
 * It is not clear how this would work for shortening multiple paths... maybe it
 * will produce output that looks like a/*, b/* etc. 
 *
 * compile me with something like: gcc fswatch.c -framework CoreServices -o fswatch
 *
 * adapted from the FSEvents api PDF
*/

static inline int count_chars(const char* string, char ch)
{
	int count = 0;
	for(; *string; count += (*string++ == ch));
	return count;
}

// todo: make into a general helper just like count_chars
static inline int last_slash(const char* string) {
	int i;
	for(i=strlen(string)-1;i;--i) {
		if (string[i] == '/') {
			return i;
		}
	}
	// the termination condition of the for loop (and the following return) 
	// should be unnecessary since all paths have at least one slash, but I am 
	// not about to inject a potential DoS vuln because of that logic either.
	printf("wtf\n");
	exit(1);
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
			while (path[cur]) {
				if (path[cur] == ' ')
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

// making code wet eliminates conditional check inside callback
void callback_rel(
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
		char* path = ((char **)eventPaths)[i];
		// grab just the basename part
		char* bn = path+last_slash(path)+1;
		int extra = count_chars(bn, ' ');
		if (extra) { // produce escaped spaces in the paths
			char * z = malloc(strlen(bn)+1+extra);
			int cur = 0, zcur = 0;
			while (bn[cur]) {
				if (bn[cur] == ' ')
					z[zcur++] = '\\';
				z[zcur++] = bn[cur++];
			}
			printf("%x %s, ", eventFlags[i], z);
		} else {
			printf("%x %s, ", eventFlags[i], bn);
		}
	}
	printf("\n");
	fflush(stdout);
}

//set up fsevents and callback
int main(int argc, char **argv) {
	if(argc != 2 && argc != 3) {
		fprintf(stderr, "You must specify a directory to watch, you only gave %d args\n", argc);
		exit(1);
	}

	int relative = 0;
	if (strncmp(argv[2], "-r", 2) == 0) {
		fprintf(stderr, "You have specified relative path mode\n");
		relative = 1;
	}

	CFStringRef mypath = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8);
	CFArrayRef pathsToWatch = CFStringCreateArrayBySeparatingStrings (NULL, mypath, CFSTR(":"));

	void *callbackInfo = NULL;
	FSEventStreamRef stream;
	CFAbsoluteTime latency = 1.0;

	stream = FSEventStreamCreate(NULL,
		relative ? &callback_rel : &callback,
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
