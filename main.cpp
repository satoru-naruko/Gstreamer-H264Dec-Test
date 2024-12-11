#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>

#include <iostream>
#include <sstream>

int basic_tutorial_1(int argc, char* argv[]);
int h264_decode_sample(int argc, char* argv[]);
int srt_recieve(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    //basic_tutorial_1(argc, argv);
    //h264_decode_sample(argc, argv);
	srt_recieve(argc, argv);
}

int basic_tutorial_1(int argc, char* argv[])
{
    GstElement* pipeline;
    GstBus* bus;
    GstMessage* msg;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Build the pipeline */
    pipeline = gst_parse_launch("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GstMessageType::GST_MESSAGE_ERROR | GstMessageType::GST_MESSAGE_EOS));

    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;

}

static GstFlowReturn on_new_sample(GstAppSink* sink, gpointer user_data) {
    GstSample* sample;
    GstBuffer* buffer;
    GstMapInfo map;

    // �V�����T���v�����擾
    sample = gst_app_sink_pull_sample(sink);
    if (!sample) {
        g_printerr("Failed to pull sample\n");
        return GST_FLOW_ERROR;
    }

    // �T���v������o�b�t�@���擾
    buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        g_printerr("Failed to get buffer\n");
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // �o�b�t�@���}�b�s���O���ăf�[�^�ɃA�N�Z�X
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        // �t�@�C���ɏ�������
        //FILE* file;
        //errno_t err = fopen_s(&file, "output.rgb", "ab"); // �ǋL���[�h�ŊJ��
        //if (err == 0 && file) {
        //    fwrite(map.data, 1, map.size, file);
        //    fclose(file);
        //} else {
        //    g_printerr("Failed to open file\n");
        //}
        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
    GMainLoop* loop = (GMainLoop*)data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar* debug;
        GError* error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

int h264_decode_sample(int argc, char* argv[])
{
    GstElement* pipeline, * source, * parser, * decoder, * convert, * sink;
    GMainLoop* loop;

    gst_init(&argc, &argv);

    const char* filepath = "sample.h264";

    // �p�C�v���C�����쐬
    pipeline = gst_pipeline_new("video-pipeline");
    source = gst_element_factory_make("filesrc", "source");
    parser = gst_element_factory_make("h264parse", "parser");
    decoder = gst_element_factory_make("avdec_h264", "decoder");
    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("appsink", "sink");

    if (!pipeline || !source || !parser || !decoder || !convert || !sink) {
        g_printerr("Failed to create elements\n");
        return -1;
    }

    // �t�@�C���\�[�X�ݒ�
    g_object_set(source, "location", filepath, NULL);

    // appsink �̐ݒ�
    GstCaps* caps = gst_caps_from_string("video/x-raw,format=RGB");
    g_object_set(sink, "emit-signals", TRUE, "caps", caps, NULL);
    gst_caps_unref(caps);

    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), NULL);

    // �p�C�v���C���ɃG�������g��ǉ�
    gst_bin_add_many(GST_BIN(pipeline), source, parser, decoder, convert, sink, NULL);
    if (!gst_element_link_many(source, parser, decoder, convert, sink, NULL)) {
        g_printerr("Failed to link elements\n");
        return -1;
    }

    // ���C�����[�v�̍쐬
    loop = g_main_loop_new(NULL, FALSE);

    // �o�X�̎擾�ƃR�[���o�b�N�̐ݒ�
    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    // �Đ�
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // ���C�����[�v�̎��s
    g_main_loop_run(loop);

    // �I������
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}

int srt_recieve(int argc, char* argv[]) {
    GstElement* pipeline, * srtsrc, * tsdemux, * h264parse, * avdec_h264, * videoconvert, * autovideosink;
    GstBus* bus;
    GMainLoop* loop;  // ���C�����[�v�̍쐬

    // GStreamer�̏�����
    gst_init(&argc, &argv);

    // ���C�����[�v�̍쐬
    loop = g_main_loop_new(NULL, FALSE);

    // �p�C�v���C���̗v�f���쐬
    pipeline = gst_pipeline_new("srt-pipeline");
    srtsrc = gst_element_factory_make("srtsrc", "srt-source");
    tsdemux = gst_element_factory_make("tsdemux", "tsdemux");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    avdec_h264 = gst_element_factory_make("avdec_h264", "avdec_h264");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    autovideosink = gst_element_factory_make("autovideosink", "autovideosink");

    // �e�v�f���쐬����Ă��邱�Ƃ��m�F
    if (!pipeline || !srtsrc || !tsdemux || !h264parse || !avdec_h264 || !videoconvert || !autovideosink) {
        g_printerr("Not all elements could be created. Exiting.\n");
        return -1;
    }

    // SRT �\�[�X�� URI ��ݒ�
    g_object_set(srtsrc, "uri", "srt://192.168.11.15:12345", NULL);

    // autovideosink �� sync=false ��ݒ�
    g_object_set(autovideosink, "sync", FALSE, NULL);

    // �p�C�v���C����ڑ�
    gst_bin_add_many(GST_BIN(pipeline), srtsrc, tsdemux, h264parse, avdec_h264, videoconvert, autovideosink, NULL);
    if (gst_element_link_many(srtsrc, tsdemux, h264parse, avdec_h264, videoconvert, autovideosink, NULL) != TRUE) {
        g_printerr("Elements could not be linked. Exiting.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // �p�C�v���C�������s��Ԃɐݒ�
    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to set pipeline to PLAYING state. Exiting.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // �o�X�̍쐬
    bus = gst_element_get_bus(pipeline);

    // �o�X�Ƀ��b�Z�[�W�̃E�H�b�`��ǉ�
    gst_bus_add_watch(bus, bus_call, loop);

    // �o�X���g���I�������N���[���A�b�v
    gst_object_unref(bus);

    // ���C�����[�v�����s
    g_main_loop_run(loop);

    // ���C�����[�v�I����ɃN���[���A�b�v
    g_main_loop_unref(loop);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}