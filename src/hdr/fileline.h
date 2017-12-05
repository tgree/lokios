#ifndef __LOKIOS_FILELINE_H
#define __LOKIOS_FILELINE_H

#define _LINESTR(l) #l
#define LINESTR(l) _LINESTR(l)
#define FILELINESTR __FILE__ ":" LINESTR(__LINE__)

#endif /* __LOKIOS_FILELINE_H */
