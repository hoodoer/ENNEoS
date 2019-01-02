#ifndef timer_h
#define timer_h


#include <time.h>
#include <sys/time.h>


#define usec (1.0f/1000000.0f)



class Timer
{
 public:
 Timer()
   {
     reset();
   }

 float since()
   {
     float sinceTime;
     struct timeval currentTime;
     gettimeofday(&currentTime, NULL);
     sinceTime = timeDifference(&lastTime, &currentTime);
     lastTime = currentTime;
     return sinceTime;
   }

 float total()
   {
     struct timeval currentTime;
     gettimeofday(&currentTime, NULL);
     return timeDifference(&startTime, &currentTime);
   }

 float reset()
   {
     gettimeofday(&startTime, NULL);
     lastTime = startTime;
   }

 private:
 struct timeval startTime;
 struct timeval lastTime;
 inline float timeDifference(timeval* startTime, timeval* endTime)
   {
     return (float)(endTime->tv_sec - startTime->tv_sec) +
       (float)(endTime->tv_usec - startTime->tv_usec) * usec;
   }
};


#endif
