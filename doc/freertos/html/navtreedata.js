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
      [ "AVB", "index.html#autotoc_md65", null ],
      [ "TSN", "index.html#autotoc_md66", null ]
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
    [ "Timer API usage", "timer_usage.html", [
      [ "PPS support", "timer_usage.html#pps", null ]
    ] ],
    [ "Scheduled Traffic API usage", "scheduled_traffic_usage.html", null ],
    [ "Frame Preemption API usage", "frame_preemption_usage.html", null ],
    [ "Reference", "modules.html", "modules" ]
  ] ]
];

var NAVTREEINDEX =
[
"clock_usage.html",
"group__aem.html#gade22f6ab6f1e5948679bdb96bc327198",
"group__control.html#a18b2e586476f7c5242b5e9fffc9b492f",
"group__control.html#ggab94beb0e7670da522ffb95ca2580f77ea5e4070a2b3b53f4e692a32af531bb0e6",
"group__iec__61883__6.html#gga0fa2e9a5cf842c6f3b46d128e6b9c939abdfbfbab13eb24f9ecae1e02c3108fba",
"group__socket.html#a975bfd49137cb34ef651cdb98c84995d"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';