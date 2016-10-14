TEMPLATE = subdirs
SUBDIRS = daemon gui

#On windows moolticute_pageant is a wrapper that emulates Pageant from Putty
#It allows windows user to use Putty tools with Moolticute as key storage
win32: SUBDIRS += src\daemon_pageant

daemon.file = daemon.pro
gui.file = gui.pro
