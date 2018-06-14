#ifndef SRC_MACSYSTEMEVENTS_H
#define SRC_MACSYSTEMEVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

void *registerSystemHandler(void *instance, void (*trigger)(const int, void *));
void unregisterSystemHandler(void *instance);

#ifdef __cplusplus
}
#endif

#endif // SRC_MACSYSTEMEVENTS_H
