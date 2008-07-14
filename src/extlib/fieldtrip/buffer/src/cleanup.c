/*
 * Copyright (C) 2008, Robert Oostenveld
 * F.C. Donders Centre for Cognitive Neuroimaging, Radboud University Nijmegen,
 * Kapittelweg 29, 6525 EN Nijmegen, The Netherlands
 *
 * $Log: cleanup.c,v $
 * Revision 1.5  2008/07/09 10:38:33  roboos
 * added verbose flag for printing
 *
 * Revision 1.4  2008/05/22 10:04:26  roboos
 * cleaned up local includes and ensure that socket_includes.h is used for closesocket on unix
 *
 * Revision 1.3  2008/05/22 09:49:32  roboos
 * some small changes by Jurgen Mellinger related to compatibility with Borland
 * moved linux and socket specific includes and defines into seperate header files
 *
 * Revision 1.2  2008/03/27 11:15:04  thohar
 * replaced close with closesocket
 *
 * Revision 1.1  2008/03/13 14:52:57  roboos
 * new code, used in forced cleanup ot the threads
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "message.h"
#include "buffer.h"
#include "socket_includes.h"

/* this is used for debugging */
int verbose = 0;

void cleanup_socket(void *arg) {
	int s = *((int *)arg);
	if (verbose) fprintf(stderr, "cleanup_socket()\n");
	if (s>0)
		closesocket(s);
	return;
}

void cleanup_message(void *arg) {
	message_t *message = (message_t *)arg;
	if (verbose) fprintf(stderr, "cleanup_message()\n");
	if (message) {
		FREE(message->def);
		FREE(message->buf);
		FREE(message);
	}
	return;
}

void cleanup_header(void *arg) {
	header_t *header = (header_t *)arg;
	if (verbose) fprintf(stderr, "cleanup_header()\n");
	if (header) {
		FREE(header->def);
		FREE(header->buf);
		FREE(header);
	}
	return;
}

void cleanup_data(void *arg) {
	data_t *data = (data_t *)arg;
	if (verbose) fprintf(stderr, "cleanup_data()\n");
	if (data) {
		FREE(data->def);
		FREE(data->buf);
		FREE(data);
	}
	return;
}

void cleanup_event(void *arg) {
	event_t *event = (event_t *)arg;
	if (verbose) fprintf(stderr, "cleanup_event()\n");
	if (event) {
		FREE(event->def);
		FREE(event->buf);
		FREE(event);
	}
	return;
}

void cleanup_property(void *arg) {
	property_t *property = (property_t *)arg;
	if (verbose) fprintf(stderr, "cleanup_property()\n");
	if (property) {
		FREE(property->def);
		FREE(property->buf);
		FREE(property);
	}
	return;
}

