#ifndef SRC_MACSYSTEMEVENTS_H
#define SRC_MACSYSTEMEVENTS_H

void *registerSystemHandler(void *instance, void (*trigger)(const int, void *));
void unregisterSystemHandler(void *instance);

#endif // SRC_MACSYSTEMEVENTS_H
