/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GenAVB alsa application
 @details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <sched.h>
#include <signal.h>

#include <genavb/genavb.h>
#include <genavb/srp.h>

#include "../common/alsa.h"
#include "../common/msrp.h"

/* Application main modes */
#define MODE_AVDECC	0 /* the application relies on avdecc indication from avb stack*/
#define MODE_LISTENER	1 /* acting as media files server if avdecc is not used*/
#define MODE_TALKER	2 /* acting as media files server if avdecc is not used*/

#define PROCESS_PRIORITY		60  /* RT_FIFO priority to be used for the process */

#define BATCH_SIZE	512

static int signal_terminate = 0;


/* application main context */
struct avb_app {
	unsigned int mode;
	unsigned int config;
	int media_fd;
	struct avb_stream_params stream_params;
	unsigned int stream_batch_size;
	unsigned int stream_flags;
	struct avb_control_handle *ctrl_h;
};

struct avb_app app;


struct avb_stream_params default_stream_params = {
	.direction = AVTP_DIRECTION_LISTENER,
	.subtype = AVTP_SUBTYPE_61883_IIDC,
	.stream_class = SR_CLASS_A,
	.clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM,
	.flags = AVB_STREAM_FLAGS_MCR,
	.format.u.s = {
		.v = 0,
		.subtype = AVTP_SUBTYPE_61883_IIDC,
		.subtype_u.iec61883 = {
			.sf = IEC_61883_SF_61883,
			.fmt = IEC_61883_CIP_FMT_6,
			.r = 0,
			.format_u.iec61883_6 = {
				.fdf_u.fdf = {
					.evt = IEC_61883_6_FDF_EVT_AM824,
					.sfc = IEC_61883_6_FDF_SFC_48000,
				},
				.dbs = 2,
				.b = 0,
				.nb = 1,
				.rsvd = 0,
				.label_iec_60958_cnt = 0,
				.label_mbla_cnt = 2,
				.label_midi_cnt = 0,
				.label_smptecnt = 0,
			},
		},
	},
	.port = 0,
	.stream_id = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 },
	.dst_mac = { 0x91, 0xE0, 0xF0, 0x00, 0xeb, 0x15 }
};

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-m <mode>             application mode: avdecc(default), listener\n"
		"\t-h                    print this help text\n");

}


static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}


static int apply_config(struct avb_stream_params *stream_params)
{
	memcpy(&app.stream_params, stream_params, sizeof(struct avb_stream_params));

	app.stream_batch_size = BATCH_SIZE;

	app.stream_flags = AVTP_NONBLOCK;

	/* display the whole configuration */
	printf("stream ID: %02x%02x%02x%02x%02x%02x%02x%02x\n", stream_params->stream_id[0],stream_params->stream_id[1],stream_params->stream_id[2],stream_params->stream_id[3],
								stream_params->stream_id[4],stream_params->stream_id[5],stream_params->stream_id[6],stream_params->stream_id[7]);

	printf("mode: ");
	if (app.mode == MODE_LISTENER)
		printf("LISTENER\n");
	else if (app.mode == MODE_TALKER)
		printf("TALKER\n");
	else
		printf("AVDECC %s\n", (app.stream_params.direction == AVTP_DIRECTION_LISTENER)? "LISTENER":"TALKER");

	return 0;
}

static int handle_avdecc_event(struct avb_control_handle *ctrl_h, unsigned int *msg_type)
{
	union avb_media_stack_msg msg;
	unsigned int msg_len = sizeof(union avb_media_stack_msg);
	int rc;

	rc = avb_control_receive(ctrl_h, msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto receive_error;

	switch (*msg_type) {
	case AVB_MSG_MEDIA_STACK_CONNECT:
		printf("AVB_MSG_MEDIA_STACK_CONNECT\n");
		apply_config(&msg.media_stack_connect.stream_params);
		break;

	case AVB_MSG_MEDIA_STACK_DISCONNECT:
		printf("AVB_MSG_MEDIA_STACK_DISCONNECT\n");
		break;

	default:
		break;
	}

receive_error:
	return rc;
}


static int run_listener(struct avb_stream_handle *stream_h, int stream_fd, unsigned int batch_size, void *alsa_h)
{
	struct pollfd poll_fds[2];
	int rc = 0;
	int ctrl_rx_fd = -1, ready, i, n, nfds;
	unsigned int event_type;

	/*
	* setup ALSA to receive samples
	*/

	alsa_h = alsa_tx_init((void*)&app.stream_params.stream_id, &app.stream_params.format, batch_size);
	if (!alsa_h) {
		printf("Couldn't initialize alsa tx, aborting...\n");
		rc = -1;
		goto err_alsa_init;
	}

	printf("Starting listener loop, non-blocking mode\n");
	/*
	* listen to read event from the stack
	*/

	poll_fds[0].fd = stream_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[0].revents = 0;

	nfds = 1;

	if (app.ctrl_h) {
		ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
		poll_fds[1].fd = ctrl_rx_fd;
		poll_fds[1].events = POLLIN;
		poll_fds[1].revents = 0;

		nfds++;
	} else {
		poll_fds[1].fd = -1;
		poll_fds[1].events = 0;
		poll_fds[1].revents = 0;
	}

	while (1) {
		if ((ready = poll(poll_fds, nfds, -1)) == -1) {
			if (errno == EINTR) {
				if (signal_terminate) {
					printf("processing terminate signal\n");
					signal_terminate = 0;
					rc = -1;
					goto exit;
				}
			} else {
				printf("poll(%d) failed while processing listener\n", stream_fd);
				rc = -1;
				goto exit;
			}
		}

		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[i].revents & POLLIN) {
					if (poll_fds[i].fd == ctrl_rx_fd) {
						n++;

						/*
						* read control event from avdecc
						*/
						if (handle_avdecc_event(app.ctrl_h, &event_type) == AVB_SUCCESS) {
							if (event_type == AVB_MSG_MEDIA_STACK_DISCONNECT) {
								rc = 0;
								goto exit; /* disconnected, stop stream processing */
							}
						}
					} else if (poll_fds[i].fd == stream_fd) {
						n++;

						/*
						* send samples to alsa
						*/
						if (alsa_tx(alsa_h, stream_h, &app.stream_params) < 0) {
							printf("Error writing data to alsa...\n");
							rc = -1;
						}
					}
				}
			}
		}
	}

exit:
	alsa_tx_exit(alsa_h);

err_alsa_init:
	return rc;

}

void signal_terminate_handler (int signal_num)
{
	signal_terminate = 1;
}


int main(int argc, char *argv[])
{
	struct avb_handle *avb_h;
	struct avb_stream_handle* stream_h;
	unsigned int avb_flags;
	int stream_fd, option;
	unsigned int event_type;
	int ctrl_rx_fd;
	struct pollfd ctrl_poll;
	int rc = 0;
	void *alsa_h;
	struct sched_param param = {
			.sched_priority = PROCESS_PRIORITY,
		};
	struct sigaction action;

	/*
	 * Increase process priority to match the AVTP thread priority
	 */
	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler(), %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}

	setlinebuf(stdout);

	printf("NXP's GenAVB ALSA reference application\n");


	/*
	* retrieve user's configuration parameters
	*/

	app.mode = MODE_AVDECC; app.config = 0; /*default is AVDECC, NON-BLOCKING, SINGLE BUFFER */

	while ((option = getopt(argc, argv,"m:h")) != -1) {
		switch (option) {
		case 'm':
			if (!strcasecmp(optarg, "listener"))
				app.mode = MODE_LISTENER;
			else if(!strcasecmp(optarg, "avdecc"))
				app.mode = MODE_AVDECC;
			else {
				usage();
				goto exit;
			}
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

	/*
	* set signals handler
	*/
	action.sa_handler = signal_terminate_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		printf("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGTERM, &action, NULL) < 0) /* Termination signal */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGQUIT, &action, NULL) < 0) /* Quit from keyboard */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGINT, &action, NULL) < 0) /* Interrupt from keyboard */
		printf("sigaction(): %s\n", strerror(errno));

	/*
	* Setup the avb stack
	*/

	set_avb_config(&avb_flags);

	rc = avb_init(&avb_h, avb_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	if (app.mode == MODE_AVDECC) {
		/*
		* Open control channel for AVDECC events.
		*/
		rc = avb_control_open(avb_h, &app.ctrl_h, AVB_CTRL_AVDECC_MEDIA_STACK);
		if (rc != AVB_SUCCESS) {
			printf("avb_control_open() failed: %s\n", avb_strerror(rc));
			goto error_control_open;
		}

		ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
		ctrl_poll.fd = ctrl_rx_fd;
		ctrl_poll.events = POLLIN;
	} else {
		rc = msrp_init(avb_h);
		if (rc < 0)
			goto err_msrp_init;
	}

wait_new_stream:
	if (app.mode == MODE_AVDECC) {

		printf("\nwait for new stream...\n");

		/*
		* Listen to AVDECC events to get stream parameters
 		*/
		while (1) {
			if (poll(&ctrl_poll, 1, -1) == -1) {
				printf("poll(%d) failed on waiting for connect\n", ctrl_poll.fd);
				rc = -1;
				goto error_ctrl_poll;
			}

			if (ctrl_poll.revents & POLLIN) {
				/*
				* read control event from avdecc
				*/
				if (handle_avdecc_event(app.ctrl_h, &event_type) == AVB_SUCCESS) {
					if (event_type == AVB_MSG_MEDIA_STACK_CONNECT)
						break; /* connected, start stream processing */
				}
			}
		}
	}else {
		/*
		* no avdecc, static configuration used
		*/

		if (app.mode == MODE_TALKER)
			default_stream_params.direction = AVTP_DIRECTION_TALKER;
		else
			default_stream_params.direction = AVTP_DIRECTION_LISTENER;

		apply_config(&default_stream_params);

		if (app.mode == MODE_TALKER) {
			rc = msrp_talker_register(&default_stream_params);
			if (rc != AVB_SUCCESS) {
				printf("msrp_talker_register error, rc = %d\n", rc);
				goto err_msrp;
			}
		} else if (app.mode == MODE_LISTENER) {
			rc = msrp_listener_register(&default_stream_params);
			if (rc != AVB_SUCCESS) {
				printf("msrp_listener_register error, rc = %d\n", rc);
				goto err_msrp;
			}
		}
	}

	/*
	* setup the stream
	*/

	rc = avb_stream_create (avb_h, &stream_h, &app.stream_params, &app.stream_batch_size, app.stream_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", app.stream_batch_size);


	/*
	* retrieve the file descriptor associated to the stream
	*/

	stream_fd = avb_stream_fd(stream_h);
	if (stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	/*
	* run listener/talker main processing function
	*/

	if (app.stream_params.direction & AVTP_DIRECTION_TALKER) {
		printf("Talker direction not supported yet.\n");
		rc = -1;
	}
	else
		rc = run_listener(stream_h, stream_fd,  app.stream_batch_size, alsa_h);

	if (rc < 0) {
		printf("Loop function exited with error code %d\n", rc);
	} else {
		printf("Loop function exited upon avdecc disconnect\n");

		avb_stream_destroy(stream_h);

		goto wait_new_stream;
	}

	/*
	* destroy the stream, disconnect for avb stack
	*/
error_stream_fd:
	avb_stream_destroy(stream_h);

error_stream_create:
	if (app.mode == MODE_TALKER)
		msrp_talker_deregister(&default_stream_params);
	else if (app.mode == MODE_LISTENER)
		msrp_listener_deregister(&default_stream_params);

err_msrp:
error_ctrl_poll:
	if (app.ctrl_h)
		avb_control_close(app.ctrl_h);

error_control_open:
	if (app.mode != MODE_AVDECC)
		msrp_exit();
err_msrp_init:
	avb_exit(avb_h);

error_avb_init:
exit:
	return rc;

}
