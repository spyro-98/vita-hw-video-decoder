#ifndef VITA_HW_VIDEO_DECODER_DECODER_RUNTIME_H
#define VITA_HW_VIDEO_DECODER_DECODER_RUNTIME_H

/* Loads the bundled compatibility runtime once for the process. The runtime
 * must remain resident until every player and SceAvPlayer instance is closed. */
int vita_hw_video_decoder_runtime_prepare(void);

#endif
