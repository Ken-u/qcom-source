#ifndef AENC_SVR_H
#define AENC_SVR_H

/* Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#ifdef _ANDROID_
#define LOG_TAG "QC_AMRENC"
#endif
#include "qc_omx_msg.h"


typedef void (*process_message_func)(void* client_data, unsigned char id);

/**
 @brief audio decoder command server structure

  This structure maintains the command
  server context

 */
struct aenc_cmd_svr
{
    pthread_t thr;
    int pipe_in;
    int pipe_out;
    int dead;
    process_message_func process_msg_cb;
    void *client_data;
};

struct aenc_cmd_svr *aenc_svr_start(process_message_func cb,
                                    void* client_data);

/*struct aenc_cmd_cln *aenc_cln_start(process_message_func cb,
                                    void* client_data);                */
/**
 @brief This function stop command server

 @param svr handle to command server
 @return none
 */
void aenc_svr_stop(struct aenc_cmd_svr *svr);

void aenc_cln_stop(struct aenc_cmd_svr *cln);
/**
 @brief This function post message in the command server

 @param svr handle to command server
 @return none
 */
void aenc_svr_post_msg(struct aenc_cmd_svr *svr, unsigned char id);
void aenc_output_post_msg(struct aenc_cmd_svr *cln, unsigned char id);

#ifdef __cplusplus
}
#endif

#endif /* AENC_SVR */
