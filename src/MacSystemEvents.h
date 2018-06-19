#ifndef SRC_MACSYSTEMEVENTS_H
#define SRC_MACSYSTEMEVENTS_H

void *registerSystemHandler(void *instance, void (*trigger)(const int, void *));
void unregisterSystemHandler(void *instance);

// If NSTerminateLater has been used it is necessary to instruct when it is okay to terminate.
void readyToTerminate();

#endif // SRC_MACSYSTEMEVENTS_H
