#include <sys/time.h>

/* Defines Packet Types */

/* struct INITHELLO { */
/*     char type; // type=1 */
/*     char need_file; */
/* }; */

/* struct PAUSE { */
/*     char type; // type=3 */
/*     struct timeval time_stamp; */
/* }; */

/* struct PLAY { */
/*     char type; // type=3 */
/*     struct timeval time_stamp; */
/* }; */

/* struct USERLISTACK { */
/*     char type; // type=5 */
/* }; */

/* struct PAUSEACK { */
/*     char type; // type=6 */
/* }; */

/* struct PLAYACK { */
/*     char type; // type=7 */
/* }; */

struct VIDPKT {
    char type; // type=8
    char filename[128];
}__attribute__((packed));  ;
