#ifndef XENO_MOVIE_PLAYER_H
#define XENO_MOVIE_PLAYER_H

/* Native setup/ReadS failed or the selected movie ran out before its endpoint.
 * Failure survives stop and release and clears on the next successful setup. */
int MoviePlayerStartFailed(void);

#endif
