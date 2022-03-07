/*
@licstart  The following is the entire license notice for the
JavaScript code in this file.

Copyright (C) 1997-2019 by Dimitri van Heesch

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

@licend  The above is the entire license notice
for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "GenAVB/TSN", "index.html", [
    [ "Introduction", "index.html", [
      [ "AVB", "index.html#autotoc_md137", null ],
      [ "TSN", "index.html#autotoc_md138", null ]
    ] ],
    [ "Initialization API usage", "init_usage.html", null ],
    [ "Streaming API usage", "streaming_usage.html", [
      [ "Stream creation", "streaming_usage.html#stream_creation", null ],
      [ "Flow control", "streaming_usage.html#flow_control", null ],
      [ "Receiving data (listener stream)", "streaming_usage.html#rx_data", null ],
      [ "Sending data (talker stream)", "streaming_usage.html#tx_data", null ]
    ] ],
    [ "Stream formats", "stream_formats.html", [
      [ "Supported stream formats", "stream_formats.html#str_formats_0", null ],
      [ "IEC 61883-4", "stream_formats.html#str_formats_1", null ],
      [ "IEC 61883-6 AM824", "stream_formats.html#str_formats_2", null ],
      [ "IEC 61883-6 FLOAT32", "stream_formats.html#str_formats_3", null ],
      [ "IEC 61883-6 INT32", "stream_formats.html#str_formats_4", null ],
      [ "AAF PCM", "stream_formats.html#str_formats_5", null ],
      [ "AAF AES3", "stream_formats.html#str_formats_6", null ],
      [ "CVF MJPEG", "stream_formats.html#str_formats_7", null ],
      [ "CVF H264", "stream_formats.html#str_formats_8", null ],
      [ "CRF AUDIO SAMPLE", "stream_formats.html#str_formats_9", null ],
      [ "ACF TSCF", "stream_formats.html#str_formats_11", null ],
      [ "ACF NTSCF", "stream_formats.html#str_formats_12", null ]
    ] ],
    [ "Control API usage", "control_usage.html", [
      [ "Introduction", "control_usage.html#autotoc_md32", null ],
      [ "Restrictions", "control_usage.html#autotoc_md34", null ],
      [ "Clock Domain control API", "control_usage.html#autotoc_md36", null ],
      [ "MSRP control API", "control_usage.html#autotoc_md43", null ],
      [ "GPTP control API", "control_usage.html#autotoc_md47", null ],
      [ "AVDECC control API", "control_usage.html#autotoc_md49", [
        [ "Exchanges on an @ref GENAVB_CTRL_AVDECC_MEDIA_STACK channel", "control_usage.html#autotoc_md51", null ],
        [ "ACMP exchanges on an @ref GENAVB_CTRL_AVDECC_CONTROLLER channel", "control_usage.html#autotoc_md53", null ],
        [ "ADP exchanges on an @ref GENAVB_CTRL_AVDECC_CONTROLLER channel", "control_usage.html#autotoc_md55", null ],
        [ "AECP exchanges", "control_usage.html#autotoc_md57", [
          [ "Static mode", "streaming_usage.html#autotoc_md0", null ],
          [ "AVDECC mode", "streaming_usage.html#autotoc_md1", null ],
          [ "Audio", "streaming_usage.html#autotoc_md2", [
            [ "61883-6", "streaming_usage.html#autotoc_md3", null ],
            [ "AAF", "streaming_usage.html#autotoc_md4", null ]
          ] ],
          [ "Video", "streaming_usage.html#autotoc_md5", [
            [ "61883-4", "streaming_usage.html#autotoc_md6", null ],
            [ "CVF/MJPEG", "streaming_usage.html#autotoc_md7", null ],
            [ "CVF/H264", "streaming_usage.html#autotoc_md8", null ]
          ] ],
          [ "Control", "streaming_usage.html#autotoc_md9", [
            [ "ACF/NTSCF", "streaming_usage.html#autotoc_md10", null ],
            [ "ACF/TSCF", "streaming_usage.html#autotoc_md11", null ]
          ] ],
          [ "Audio", "streaming_usage.html#autotoc_md12", [
            [ "61883-6", "streaming_usage.html#autotoc_md13", null ],
            [ "AAF", "streaming_usage.html#autotoc_md14", null ]
          ] ],
          [ "Video", "streaming_usage.html#autotoc_md15", [
            [ "61883-4", "streaming_usage.html#autotoc_md16", null ],
            [ "CVF/H264", "streaming_usage.html#autotoc_md17", null ]
          ] ],
          [ "Control", "streaming_usage.html#autotoc_md18", [
            [ "ACF/NTSCF", "streaming_usage.html#autotoc_md19", null ],
            [ "ACF/TSCF", "streaming_usage.html#autotoc_md20", null ]
          ] ],
          [ "Message types properties", "control_usage.html#autotoc_md33", null ],
          [ "Clock domain", "control_usage.html#autotoc_md37", null ],
          [ "Setting the clock domain source", "control_usage.html#autotoc_md38", null ],
          [ "Source types", "control_usage.html#autotoc_md39", null ],
          [ "Clock source types summary", "control_usage.html#autotoc_md40", null ],
          [ "Indications", "control_usage.html#autotoc_md41", null ],
          [ "Talker streams", "control_usage.html#autotoc_md44", null ],
          [ "Listener streams", "control_usage.html#autotoc_md45", null ],
          [ "AEM model support", "control_usage.html#autotoc_md58", null ],
          [ "AECP exchanges on an @ref GENAVB_CTRL_AVDECC_CONTROLLER channel", "control_usage.html#autotoc_md59", null ],
          [ "AECP exchanges on an @ref GENAVB_CTRL_AVDECC_CONTROLLED channel", "control_usage.html#autotoc_md60", [
            [ "Handling of SET_CONTROL command", "control_usage.html#autotoc_md61", null ],
            [ "Handling of START_STREAMING and STOP_STREAMING commands", "control_usage.html#autotoc_md62", null ],
            [ "Responses from the application", "control_usage.html#autotoc_md63", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "Socket API usage", "socket_usage.html", [
      [ "Socket transmit", "socket_usage.html#sock_tx", null ],
      [ "Socket receive", "socket_usage.html#sock_rx", null ],
      [ "Flow control (non-blocking mode)", "socket_usage.html#flow_control_sock", null ],
      [ "Network address", "socket_usage.html#net_addr", null ]
    ] ],
    [ "Clock API usage", "clock_usage.html", null ],
    [ "gPTP usage and configuration", "fgptp_usage.html", [
      [ "Description", "fgptp_usage.html#autotoc_md65", null ],
      [ "Usage", "fgptp_usage.html#autotoc_md66", null ],
      [ "Configuration files (/etc/genavb/fgptp.cfg[-N])", "fgptp_usage.html#autotoc_md67", null ]
    ] ],
    [ "GenAVB Configuration", "genavb_config.html", [
      [ "Main Configuration", "genavb_config.html#autotoc_md101", null ],
      [ "Application profile parameters", "genavb_config.html#autotoc_md106", null ],
      [ "GenAVB stack profile parameters", "genavb_config.html#autotoc_md113", null ],
      [ "Defining an AVDECC entity description", "genavb_config.html#autotoc_md123", null ],
      [ "AVB Startup Options", "genavb_config.html#autotoc_md126", null ],
      [ "Endpoint, entity and application constraints", "genavb_config.html#autotoc_md128", null ]
    ] ],
    [ "Linux platform specific", "platform_linux.html", [
      [ "Media clock driver", "platform_linux.html#autotoc_md129", [
        [ "DMA based recovery and generation", "platform_linux.html#autotoc_md130", [
          [ "[FGPTP_GENERAL]", "fgptp_usage.html#autotoc_md68", [
            [ "profile", "fgptp_usage.html#autotoc_md69", null ],
            [ "grand master ID", "fgptp_usage.html#autotoc_md70", null ],
            [ "gPTP domain number assignment", "fgptp_usage.html#autotoc_md71", null ],
            [ "log output level", "fgptp_usage.html#autotoc_md72", null ],
            [ "reverse sync feature control", "fgptp_usage.html#autotoc_md73", null ],
            [ "reverse sync feature interval", "fgptp_usage.html#autotoc_md74", null ],
            [ "Neighbor propagation delay threshold", "fgptp_usage.html#autotoc_md75", null ],
            [ "Statistics output interval", "fgptp_usage.html#autotoc_md76", null ]
          ] ],
          [ "[FGPTP_GM_PARAMS]", "fgptp_usage.html#autotoc_md77", [
            [ "grandmaster capable setting", "fgptp_usage.html#autotoc_md78", null ],
            [ "grandmaster priority1 value", "fgptp_usage.html#autotoc_md79", null ],
            [ "grandmaster priority2 value", "fgptp_usage.html#autotoc_md80", null ],
            [ "grandmaster clock class value", "fgptp_usage.html#autotoc_md81", null ],
            [ "grandmaster clock accuracy value", "fgptp_usage.html#autotoc_md82", null ],
            [ "grandmaster variance value", "fgptp_usage.html#autotoc_md83", null ]
          ] ],
          [ "[FGPTP_AUTOMOTIVE_PARAMS]", "fgptp_usage.html#autotoc_md84", [
            [ "pdelay mode", "fgptp_usage.html#autotoc_md85", null ],
            [ "static pdelay value", "fgptp_usage.html#autotoc_md86", null ],
            [ "static pdelay sensitivity", "fgptp_usage.html#autotoc_md87", null ],
            [ "nvram file name", "fgptp_usage.html#autotoc_md88", null ]
          ] ],
          [ "[FGPTP_PORTn]", "fgptp_usage.html#autotoc_md89", null ],
          [ "port role", "fgptp_usage.html#autotoc_md90", null ],
          [ "ptp port enabled", "fgptp_usage.html#autotoc_md91", null ],
          [ "Rx timestamping compensation delay", "fgptp_usage.html#autotoc_md92", null ],
          [ "Tx timestamping compensation delay", "fgptp_usage.html#autotoc_md93", [
            [ "initial pdelay request interval value", "fgptp_usage.html#autotoc_md94", null ],
            [ "initial sync interval value", "fgptp_usage.html#autotoc_md95", null ],
            [ "initial announce interval value", "fgptp_usage.html#autotoc_md96", null ],
            [ "operational pdelay request interval value", "fgptp_usage.html#autotoc_md97", null ],
            [ "operational sync interval value", "fgptp_usage.html#autotoc_md98", null ]
          ] ],
          [ "Pdelay Mechanism", "fgptp_usage.html#autotoc_md99", null ],
          [ "Configuration profile", "genavb_config.html#autotoc_md102", null ],
          [ "Auto start", "genavb_config.html#autotoc_md103", null ],
          [ "Setting system clock to be gPTP-based", "genavb_config.html#autotoc_md104", null ],
          [ "Fast connect mode", "genavb_config.html#autotoc_md107", null ],
          [ "Defining a media application", "genavb_config.html#autotoc_md108", null ],
          [ "Defining a controller application", "genavb_config.html#autotoc_md109", null ],
          [ "Custom controller parameters", "genavb_config.html#autotoc_md110", null ],
          [ "Defining a control application", "genavb_config.html#autotoc_md111", null ],
          [ "Section [AVB_GENERAL]", "genavb_config.html#autotoc_md114", null ],
          [ "Section [AVB_AVDECC]", "genavb_config.html#autotoc_md115", null ],
          [ "Section [AVB_AVDECC_ENTITY_1]", "genavb_config.html#autotoc_md116", null ],
          [ "Section [AVB_AVDECC_ENTITY_2]", "genavb_config.html#autotoc_md117", null ],
          [ "Custom AEM parameters", "genavb_config.html#autotoc_md118", [
            [ "Association ID", "genavb_config.html#autotoc_md119", null ],
            [ "Entity ID", "genavb_config.html#autotoc_md120", null ],
            [ "Entity model ID", "genavb_config.html#autotoc_md121", null ]
          ] ],
          [ "AEM configuration file format", "genavb_config.html#autotoc_md124", null ],
          [ "AVB DMA node", "platform_linux.html#autotoc_md131", null ],
          [ "CS2000 node", "platform_linux.html#autotoc_md132", null ],
          [ "MLE14570", "platform_linux.html#autotoc_md133", null ]
        ] ],
        [ "Internal audio PLL recovery", "platform_linux.html#autotoc_md134", [
          [ "AVB internal recovery node", "platform_linux.html#autotoc_md135", null ],
          [ "AVB HW timer node", "platform_linux.html#autotoc_md136", null ]
        ] ]
      ] ]
    ] ],
    [ "Reference", "modules.html", "modules" ]
  ] ]
];

var NAVTREEINDEX =
[
"clock_usage.html",
"group__aem.html#a63b4b597840f92816e511c30ef4dfb66",
"group__avtp.html#a58df9332aea7ceb4545458e45e3888f0",
"group__control.html#gga30dc1d7d1686cf41e7fa27982a533116ae983156e18782690a97976e59ecb9324",
"group__iec__61883.html",
"socket_usage.html#flow_control_sock"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';