#ifndef MAC_UTILS_H
#define MAC_UTILS_H


namespace utils {
namespace mac {

void hideDockIcon(bool hide);
void orderFrontRegardless(unsigned long long win_id, bool force = false);
void setAutoStartup(bool en);

} // namespace mac
} // namespace utils
#endif /* MAC_UTILS_H */
