/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AECP common code
 @details Handles AECP stack
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdarg.h>
#include <getopt.h>

#include "genavb/aem.h"
#include "genavb/helpers.h"

#include "aem_manager.h"

#define DESC_MIN 0
#define DESC_MAX 0xffff

static struct aem_desc_handler desc_handler[AEM_NUM_DESC_TYPES];

static void print_level(int level, const char *format, ...)
{
	va_list ap;
	int i;

	for (i = 0; i < level; i++)
		printf("\t");

	va_start(ap, format);

	vprintf(format, ap);

	va_end(ap);
}


static void default_desc_fixup(struct aem_desc_hdr *aem_desc)
{
}

static int default_desc_check(struct aem_desc_hdr *aem_desc)
{
	return 0;
}

static void default_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
}

static void default_desc_update_name(struct aem_desc_hdr *aem_desc, char *name)
{
}

static int entity_desc_check(struct aem_desc_hdr *aem_desc)
{
	struct entity_descriptor *entity;
	unsigned short len;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, &len);

	if (ntohs(entity->current_configuration) > ntohs(entity->configurations_count))
		return -1;

	return 0;
}

static void entity_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct entity_descriptor *entity;
	char string[128];
	unsigned short len;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, &len);

	print_level(level, "Entity id: %016"PRIx64"\n", ntohll(entity->entity_id));
	h_strncpy(string, (char *)entity->entity_name, 64);
	print_level(level, " name:     %s\n", string);
	print_level(level, " sources:  %u\n", ntohs(entity->talker_stream_sources));
	print_level(level, " sinks:    %u\n", ntohs(entity->listener_stream_sinks));

	desc_handler[AEM_DESC_TYPE_CONFIGURATION].print(aem_desc, level + 1, DESC_MIN, DESC_MAX);
}

static void entity_desc_update_name(struct aem_desc_hdr *aem_desc, char *name)
{
	struct entity_descriptor *entity;
	unsigned short len;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, &len);
	if (!entity)
		return;

	/* Clear destination */
	memset((char *)entity->entity_name, 0, 64);

	len = strlen(name);
	if (len > 64)
		len = 64;

	memcpy((char *)entity->entity_name, name, len);
}

static int configuration_desc_check(struct aem_desc_hdr *aem_desc)
{
	struct configuration_descriptor *configuration;
	int desc_counts_count, desc_num, desc_type, desc_max;
	int cfg, cfg_max;
	int rc = 0;

	cfg_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONFIGURATION);

	for (cfg = 0; cfg < cfg_max; cfg++) {
		configuration = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONFIGURATION, cfg, NULL);

		for (desc_counts_count = 0; desc_counts_count < ntohs(configuration->descriptor_counts_count); desc_counts_count++) {
			desc_type = ntohs(configuration->descriptors_counts[2 * desc_counts_count]);
			desc_num = ntohs(configuration->descriptors_counts[2 * desc_counts_count + 1]);

			if (desc_type >= AEM_NUM_DESC_TYPES) {
				printf("aem(%p) configuration(%u) error: unsupported descriptor type(%u) in descriptors_count\n", aem_desc, cfg, desc_type);
				rc = -1;
				goto out;
			}

			desc_max = aem_get_descriptor_max(aem_desc, desc_type);

			if (desc_num > desc_max) {
				printf("aem(%p) configuration(%u) error: descriptor count(%u) exceeds maximum(%u) for descriptor type(%u)\n", aem_desc, cfg, desc_num, desc_max, desc_type);
				rc = -1;
				goto out;
			}
		}
	}

out:
	return rc;
}

static void configuration_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct configuration_descriptor *configuration;
	char string[128];
	unsigned short len;
	int cfg, cfg_max;
	int i, type;

	cfg_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONFIGURATION);

	if (cfg_max > max)
		cfg_max = max;

	for (cfg = min; cfg < cfg_max; cfg++) {
		configuration = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONFIGURATION, cfg, &len);

		print_level(level, "Configuration: %d\n", cfg);
		h_strncpy(string, (char *)configuration->object_name, 64);
		print_level(level, " name:         %s\n", string);
		print_level(level, " descriptors:  %d\n", ntohs(configuration->descriptor_counts_count));

		for (i = 0; i < ntohs(configuration->descriptor_counts_count); i++) {
			type = ntohs(configuration->descriptors_counts[2 * i]);

			desc_handler[type].print(aem_desc, level + 1, DESC_MIN, DESC_MAX);
		}
	}
}

static void audio_unit_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct audio_unit_descriptor *audio_unit;
	char string[128];
	unsigned short len;
	int i, i_max;
	int _min, _max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_AUDIO_UNIT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		audio_unit = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_AUDIO_UNIT, i, &len);

		print_level(level, "Audio Unit: %u\n", i);
		h_strncpy(string, (void *)audio_unit->object_name, 64);
		print_level(level, " name:            %s\n", string);
		print_level(level, " stream input port:    %u\n", ntohs(audio_unit->number_of_stream_input_ports));
		print_level(level, " stream output port:   %u\n", ntohs(audio_unit->number_of_stream_output_ports));
		print_level(level, " external input port:  %u\n", ntohs(audio_unit->number_of_external_input_ports));
		print_level(level, " external output port: %u\n", ntohs(audio_unit->number_of_external_output_ports));
		print_level(level, " internal input port:  %u\n", ntohs(audio_unit->number_of_internal_input_ports));
		print_level(level, " internal output port: %u\n", ntohs(audio_unit->number_of_internal_output_ports));

		_min = ntohs(audio_unit->base_stream_input_port);
		_max = _min + ntohs(audio_unit->number_of_stream_input_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_INPUT].print(aem_desc, level + 1, _min, _max);

		_min = ntohs(audio_unit->base_stream_output_port);
		_max = _min + ntohs(audio_unit->number_of_stream_output_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_OUTPUT].print(aem_desc, level + 1, _min, _max);
	}
}

static void video_unit_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct video_unit_descriptor *video_unit;
	char string[128];
	unsigned short len;
	int i, i_max;
	int _min, _max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_VIDEO_UNIT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		video_unit = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_VIDEO_UNIT, i, &len);

		print_level(level, "Video Unit: %u\n", i);
		h_strncpy(string, (void *)video_unit->object_name, 64);
		print_level(level, " name:            %s\n", string);
		print_level(level, " stream input port:    %u\n", ntohs(video_unit->number_of_stream_input_ports));
		print_level(level, " stream output port:   %u\n", ntohs(video_unit->number_of_stream_output_ports));
		print_level(level, " external input port:  %u\n", ntohs(video_unit->number_of_external_input_ports));
		print_level(level, " external output port: %u\n", ntohs(video_unit->number_of_external_output_ports));
		print_level(level, " internal input port:  %u\n", ntohs(video_unit->number_of_internal_input_ports));
		print_level(level, " internal output port: %u\n", ntohs(video_unit->number_of_internal_output_ports));

		_min = ntohs(video_unit->base_stream_input_port);
		_max = _min + ntohs(video_unit->number_of_stream_input_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_INPUT].print(aem_desc, level + 1, _min, _max);

		_min = ntohs(video_unit->base_stream_output_port);
		_max = _min + ntohs(video_unit->number_of_stream_output_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_OUTPUT].print(aem_desc, level + 1, _min, _max);
	}
}


static void stream_input_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct stream_descriptor *stream;
	char string[128];
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_INPUT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_INPUT, i, &len);

		print_level(level, "Stream Input: %u\n", i);
		h_strncpy(string, (void *)stream->object_name, 64);
		print_level(level, " name:    %s\n", string);
	}
}

static void stream_input_desc_update_name(struct aem_desc_hdr *aem_desc, char *names)
{
	struct stream_descriptor *stream;
	char *string;
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_INPUT);

	string = strtok(names, ";");
	for (i = 0; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_INPUT, i, &len);

		if (!string)
			break;

		/* Clear destination */
		memset((char *)stream->object_name, 0, 64);

		len = strlen(string);
		if (len > 64)
			len = 64;

		memcpy((void *)stream->object_name, string, len);

		string = strtok(NULL, ";");
	}
}


static void stream_output_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct stream_descriptor *stream;
	char string[128];
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT, i, &len);

		print_level(level, "Stream Output: %u\n", i);
		h_strncpy(string, (void *)stream->object_name, 64);
		print_level(level, " name:    %s\n", string);
	}
}

static void stream_output_desc_update_name(struct aem_desc_hdr *aem_desc, char *names)
{
	struct stream_descriptor *stream;
	char *string;
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT);

	string = strtok(names, ";");
	for (i = 0; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT, i, &len);

		if (!string)
			break;

		/* Clear destination */
		memset((char *)stream->object_name, 0, 64);

		len = strlen(string);
		if (len > 64)
			len = 64;

		memcpy((void *)stream->object_name, string, len);

		string = strtok(NULL, ";");
	}
}

static void control_desc_print(struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct control_descriptor *control;
	char string[128];
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONTROL);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		control = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONTROL, i, &len);

		print_level(level, "Control: %u\n", i);
		h_strncpy(string, (void *)control->object_name, 64);
		print_level(level, " name:    %s\n", string);
	}
}

static struct aem_desc_handler desc_handler[AEM_NUM_DESC_TYPES] = {
	[AEM_DESC_TYPE_ENTITY] = {
		.fixup = aem_entity_desc_fixup,
		.check = entity_desc_check,
		.print = entity_desc_print,
		.update_name = entity_desc_update_name,
	},

	[AEM_DESC_TYPE_CONFIGURATION] = {
		.fixup = aem_configuration_desc_fixup,
		.check = configuration_desc_check,
		.print = configuration_desc_print,
	},

	[AEM_DESC_TYPE_AUDIO_UNIT] = {
		.print = audio_unit_desc_print,
	},

	[AEM_DESC_TYPE_VIDEO_UNIT] = {
		.print = video_unit_desc_print,
	},

	[AEM_DESC_TYPE_STREAM_INPUT] = {
		.print = stream_input_desc_print,
		.update_name = stream_input_desc_update_name,
	},

	[AEM_DESC_TYPE_STREAM_OUTPUT] = {
		.print = stream_output_desc_print,
		.update_name = stream_output_desc_update_name,
	},

	[AEM_DESC_TYPE_VIDEO_CLUSTER] = {
		.fixup = aem_video_cluster_desc_fixup,
	},

	[AEM_DESC_TYPE_CONTROL] = {
		.print = control_desc_print,
	},

};


static void aem_entity_fixup(struct aem_desc_hdr *aem_desc)
{
	int i;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++)
		desc_handler[i].fixup(aem_desc);
}

static int aem_entity_check(struct aem_desc_hdr *aem_desc)
{
	int i;
	int rc = 0;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		rc = desc_handler[i].check(aem_desc);
		if (rc < 0)
			goto out;
	}

out:
	return rc;
}

static int aem_entity_dump_to_file(const char *name, struct aem_desc_hdr *aem_desc, unsigned int overwrite)
{
	int i;
	int fd;

	if (overwrite)
		fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	else
		fd = open(name, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	if (fd < 0) {
		printf("open(%s) failed: %s\n", name, strerror(errno));
		goto err;
	}

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		if (write(fd, &aem_desc[i].total, sizeof(avb_u16)) != sizeof(avb_u16))
			goto err_write;

		if (write(fd, &aem_desc[i].size, sizeof(avb_u16)) != sizeof(avb_u16))
			goto err_write;
	}

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		int size = aem_desc[i].size * aem_desc[i].total;

		if (write(fd, aem_desc[i].ptr, size) != size)
			goto err_write;
	}

	close(fd);

	return 0;

err_write:
	close(fd);
err:
	return -1;
}


static void __aem_entity_print(struct aem_desc_hdr *aem_desc)
{
	desc_handler[AEM_DESC_TYPE_ENTITY].print(aem_desc, 0, 0, 0xffff);
}

/* Return value: < 0 when error, 0 otherwise. */
static int aem_entity_print(const char *filename)
{
	struct aem_desc_hdr *aem_desc;

	aem_desc = aem_entity_load_from_file(filename);
	if (!aem_desc)
		return -1;

	__aem_entity_print(aem_desc);
	return 0;
}

int aem_entity_create(const char *name, void (*entity_init)(struct aem_desc_hdr *aem_desc))
{
	struct aem_desc_hdr aem_desc[AEM_NUM_DESC_TYPES] = {{0, }};
	int rc = 0;

	entity_init(aem_desc);

	aem_entity_fixup(aem_desc);

	if (aem_entity_check(aem_desc) < 0) {
		printf("aem_desc(%p) failed to create entity %s\n", aem_desc, name);
		rc = -1;
		goto err;
	}

	__aem_entity_print(aem_desc);

	aem_entity_dump_to_file(name, aem_desc, 0);

err:
	return rc;
}

/* Return value: < 0 when error, 0 otherwise. */
static int aem_entity_update_name(const char *filename, int type, char *names)
{
	struct aem_desc_hdr *aem_desc;

	aem_desc = aem_entity_load_from_file(filename);
	if (!aem_desc)
		return -1;

	if (!desc_handler[type].update_name)
		return -1;

	desc_handler[type].update_name(aem_desc, names);

	__aem_entity_print(aem_desc);

	aem_entity_dump_to_file(filename, aem_desc, 1);
	return 0;
}

extern void listener_audio_single_init(struct aem_desc_hdr *aem_desc);
extern void listener_audio_single_milan_init(struct aem_desc_hdr *aem_desc);
extern void talker_audio_single_init(struct aem_desc_hdr *aem_desc);
extern void talker_audio_single_milan_init(struct aem_desc_hdr *aem_desc);
extern void listener_talker_audio_single_init(struct aem_desc_hdr *aem_desc);
extern void listener_talker_audio_single_milan_init(struct aem_desc_hdr *aem_desc);
extern void listener_video_single_init(struct aem_desc_hdr *aem_desc);
extern void listener_video_multi_init(struct aem_desc_hdr *aem_desc);
extern void talker_video_single_init(struct aem_desc_hdr *aem_desc);
extern void talker_video_multi_init(struct aem_desc_hdr *aem_desc);
extern void talker_audio_video_init(struct aem_desc_hdr *aem_desc);
extern void talker_listener_audio_multi_init(struct aem_desc_hdr *aem_desc);
extern void talker_listener_audio_multi_aaf_init(struct aem_desc_hdr *aem_desc);
extern void talker_listener_audio_multi_format_init(struct aem_desc_hdr *aem_desc);
extern void controller_init(struct aem_desc_hdr *aem_desc);
extern void avnu_certification_init(struct aem_desc_hdr *aem_desc);


const char *filename = "listener_talker_audio_single.aem";

void print_usage (void)
{
	printf("\nUsage:\n aem-manager [options]\n");
	printf("\nOptions:\n"
		"\t-h                    prints this help text\n"
		"\t-c                    create binary file for each defined entity\n"
		"\t-f filename           name of the entity binary file to use (use with p/e/i/o options)\n"
		"\t-p                    print entity to stdout\n"
		"\t-e name               modifies the entity name in the binary file\n"
		"\t-i name1;name2        modifies the input stream names in the binary file\n"
		"\t-o name1;name2        modifies the output stream names in the binary file\n\n"
		"Examples:         ./aem-manager -c\n"
		"                  ./aem-manager -f talker_listener_audio_multi.aem -p\n"
		"                  ./aem-manager -f talker_listener_audio_multi.aem -e \"New entity name\"\n");
}

int main(int argc, char *argv[])
{
	int option, i, nb_options = 0;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		if (!desc_handler[i].print)
			desc_handler[i].print = default_desc_print;

		if (!desc_handler[i].fixup)
			desc_handler[i].fixup = default_desc_fixup;

		if (!desc_handler[i].check)
			desc_handler[i].check = default_desc_check;

		if (!desc_handler[i].update_name)
			desc_handler[i].update_name = default_desc_update_name;
	}

	while ((option = getopt(argc, argv,"f:pce:i:o:h")) != -1) {

		nb_options += 1;

		switch (option) {
		case 'f':
			filename = optarg;
			break;

		case 'p':
			if (aem_entity_print(filename) < 0)
				goto err;

			break;

		case 'e':
			if (aem_entity_update_name(filename, AEM_DESC_TYPE_ENTITY, optarg) < 0)
				goto err;

			break;

		case 'i':
			if (aem_entity_update_name(filename, AEM_DESC_TYPE_STREAM_INPUT, optarg) < 0)
				goto err;

			break;

		case 'o':
			if (aem_entity_update_name(filename, AEM_DESC_TYPE_STREAM_OUTPUT, optarg) < 0)
				goto err;

			break;

		case 'c':
			if ((aem_entity_create("listener_audio_single.aem", listener_audio_single_init) < 0)
			    || (aem_entity_create("listener_audio_single_milan.aem", listener_audio_single_milan_init) < 0)
			    || (aem_entity_create("talker_audio_single.aem", talker_audio_single_init) < 0)
			    || (aem_entity_create("talker_audio_single_milan.aem", talker_audio_single_milan_init) < 0)
			    || (aem_entity_create("listener_talker_audio_single.aem", listener_talker_audio_single_init) < 0)
			    || (aem_entity_create("listener_talker_audio_single_milan.aem", listener_talker_audio_single_milan_init) < 0)
			    || (aem_entity_create("listener_video_single.aem", listener_video_single_init) < 0)
			    || (aem_entity_create("listener_video_multi.aem", listener_video_multi_init) < 0)
			    || (aem_entity_create("talker_video_single.aem", talker_video_single_init) < 0)
			    || (aem_entity_create("talker_video_multi.aem", talker_video_multi_init) < 0)
			    || (aem_entity_create("talker_audio_video.aem", talker_audio_video_init) < 0)
			    || (aem_entity_create("talker_listener_audio_multi.aem", talker_listener_audio_multi_init) < 0)
			    || (aem_entity_create("talker_listener_audio_multi_aaf.aem", talker_listener_audio_multi_aaf_init) < 0)
			    || (aem_entity_create("talker_listener_audio_multi_format.aem", talker_listener_audio_multi_format_init) < 0)
			    || (aem_entity_create("controller.aem", controller_init) < 0)
			    || (aem_entity_create("avnu_certification.aem", avnu_certification_init) < 0)) {

				fprintf(stderr, "%s: entity generation failed\n", argv[0]);
				goto err;
			}

			break;

		case 'h':
		default:
			print_usage();
			break;
		}
	}

	if (!nb_options)
		print_usage();

	return 0;

err:
	fprintf(stderr, "%s: error!\n", argv[0]);
	return 1;
}
