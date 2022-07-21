#include <mutex>
#include <string>
#include <thread>

#include "afterSleep.h"
#include "functions.h"
#include "goingSleep.h"
#include "monitorEvents.h"
#include "prepareSleep.h"

using namespace std;

extern bool watchdogStartJob;
extern mutex watchdogStartJob_mtx;

extern goSleepCondition newSleepCondition;
extern mutex newSleepCondition_mtx;

extern sleepBool sleepJob;
extern mutex sleep_mtx;

extern sleepBool watchdogNextStep;

// this variable says what thread is currently active, to know which to kill
extern sleepBool CurrentActiveThread;
extern mutex CurrentActiveThread_mtx;

// I wanted to write a function for all those joins:
/*
void join_smarter(thread threadArg)
{
  if(threadArg.joinable() == true)
  {
    threadArg.join();
  }
}
*/
// but it doesnt work, some weird error so...

void startWatchdog() {
  std::chrono::milliseconds timespan(150);

  thread prepareThread;
  thread afterThread;
  thread goingThread;

  while (true) {
    bool saveWatchdogState = false;
    waitMutex(&watchdogStartJob_mtx);
    saveWatchdogState = watchdogStartJob;
    watchdogStartJob = false;
    watchdogStartJob_mtx.unlock();

    // this here takes signals from monitorEvents and assigns them to do things
    if (saveWatchdogState == true) {
      log("Watchdog event received");
      saveWatchdogState = false;

      // Proritise user events over next steps
      watchdogNextStep = Nothing;

      waitMutex(&sleep_mtx);

      if (sleepJob == Nothing) {
        log("Launching prepare thread becouse of nothing sleep job");
        // This is here to avoid waiting too long after
        waitMutex(&CurrentActiveThread_mtx);
        sleepJob = Prepare;
        sleep_mtx.unlock();

        if (CurrentActiveThread != Nothing) {
          bool check = false;
          CurrentActiveThread_mtx.unlock();
          while (check == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waitMutex(&CurrentActiveThread_mtx);
            if (CurrentActiveThread == Nothing) {
              check = true;
            }
            CurrentActiveThread_mtx.unlock();
          }
        } else {
          CurrentActiveThread_mtx.unlock();
        }

        waitMutex(&CurrentActiveThread_mtx);
        CurrentActiveThread = Prepare;
        CurrentActiveThread_mtx.unlock();
        prepareThread = thread(prepareSleep);
        prepareThread.detach();

        //
      } else if (sleepJob == Prepare) {
        log("Launching after thread becouse of prepare sleep job");
        waitMutex(&CurrentActiveThread_mtx);
        sleepJob = After;
        sleep_mtx.unlock();

        if (CurrentActiveThread != Nothing) {
          bool check = false;
          CurrentActiveThread_mtx.unlock();
          while (check == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waitMutex(&CurrentActiveThread_mtx);
            if (CurrentActiveThread == Nothing) {
              check = true;
            }
            CurrentActiveThread_mtx.unlock();
          }
        } else {
          CurrentActiveThread_mtx.unlock();
        }

        waitMutex(&CurrentActiveThread_mtx);
        CurrentActiveThread = After;
        CurrentActiveThread_mtx.unlock();

        afterThread = thread(afterSleep);
        afterThread.detach();

        //
      } else if (sleepJob == After) {
        log("Launching prepare thread becouse of after sleep job");
        waitMutex(&CurrentActiveThread_mtx);
        sleepJob = Prepare;
        sleep_mtx.unlock();

        if (CurrentActiveThread != Nothing) {
          bool check = false;
          CurrentActiveThread_mtx.unlock();
          while (check == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waitMutex(&CurrentActiveThread_mtx);
            if (CurrentActiveThread == Nothing) {
              check = true;
            }
            CurrentActiveThread_mtx.unlock();
          }
        } else {
          CurrentActiveThread_mtx.unlock();
        }

        waitMutex(&CurrentActiveThread_mtx);
        CurrentActiveThread = Prepare;
        CurrentActiveThread_mtx.unlock();

        prepareThread = thread(prepareSleep);
        prepareThread.detach();

        //
      } else if (sleepJob == GoingSleep) {
        log("Launching after thread becouse of goingsleep sleep job");
        waitMutex(&CurrentActiveThread_mtx);
        sleepJob = After;
        sleep_mtx.unlock();

        if (CurrentActiveThread != Nothing) {
          bool check = false;
          CurrentActiveThread_mtx.unlock();
          while (check == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waitMutex(&CurrentActiveThread_mtx);
            if (CurrentActiveThread == Nothing) {
              check = true;
            }
            CurrentActiveThread_mtx.unlock();
          }
        } else {
          CurrentActiveThread_mtx.unlock();
        }

        waitMutex(&CurrentActiveThread_mtx);
        CurrentActiveThread = After;
        CurrentActiveThread_mtx.unlock();

        afterThread = thread(afterSleep);
        afterThread.detach();

        //
      } else {
        log("This will never happen: watchdog");
        sleep_mtx.unlock();
      }
    }
    if (watchdogNextStep != Nothing) {
      // Make sure all jobs exit. they propably already are, becouse they called
      // it
      waitMutex(&sleep_mtx);
      sleepJob = Nothing;
      sleep_mtx.unlock();

      waitMutex(&CurrentActiveThread_mtx);
      if (CurrentActiveThread != Nothing) {
        bool check = false;
        CurrentActiveThread_mtx.unlock();
        while (check == false) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          waitMutex(&CurrentActiveThread_mtx);
          if (CurrentActiveThread == Nothing) {
            check = true;
          }
          CurrentActiveThread_mtx.unlock();
        }
      } else {
        CurrentActiveThread_mtx.unlock();
      }

      if (watchdogNextStep == After) {
        waitMutex(&sleep_mtx);
        sleepJob = After;
        sleep_mtx.unlock();

        waitMutex(&CurrentActiveThread_mtx);
        CurrentActiveThread = After;
        CurrentActiveThread_mtx.unlock();

        afterThread = thread(afterSleep);
        afterThread.detach();
      } else if (watchdogNextStep == GoingSleep) {
        waitMutex(&sleep_mtx);
        sleepJob = GoingSleep;
        sleep_mtx.unlock();

        waitMutex(&CurrentActiveThread_mtx);
        CurrentActiveThread = GoingSleep;
        CurrentActiveThread_mtx.unlock();

        goingThread = thread(goSleep);
        goingThread.detach();
      } else {
        log("Its impossible. you will never see this log");
        exit(EXIT_FAILURE);
      }
      watchdogNextStep = Nothing;
    }
    std::this_thread::sleep_for(timespan);
  }

  prepareThread.join();
  afterThread.join();
  goingThread.join();
}

/*
// some bad code:
// to be sure, always to avoid crashes
    if (prepareThread.joinable() == true) {
      log("Fallback joining in watchdog: prepare thread");
      //prepareThread.join();
      //log("fallback joined: prepare thread");
      //waitMutex(&CurrentActiveThread_mtx);
      //CurrentActiveThread = Nothing;
      //CurrentActiveThread_mtx.unlock();
    }
    if (goingThread.joinable() == true) {
      log("Fallback joining in watchdog: going thread");
      //goingThread.join();
      //log("fallback joined: going thread");
      //waitMutex(&CurrentActiveThread_mtx);
      //CurrentActiveThread = Nothing;
      //CurrentActiveThread_mtx.unlock();
    }
    if (afterThread.joinable() == true) {
      log("Fallback joining in watchdog: after thread");
      //afterThread.join();
      //log("fallback joined: after thread");
      //waitMutex(&CurrentActiveThread_mtx);
      //CurrentActiveThread = Nothing;
      CurrentActiveThread_mtx.unlock();
    }
*/