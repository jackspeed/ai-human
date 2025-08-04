#pragma once

//#define MFCC_OFFSET  6436
#define MFCC_OFFSET  6400
//##define MFCC_OFFSET  0
#define MFCC_DEFRMS  0.1f
#define MFCC_FPS    25
#define MFCC_RATE   16000
//#define MFCC_WAVCHUNK  960000
#define MFCC_WAVCHUNK  560000
//#define MFCC_WAVCHUNK  512

//#define MFCC_MELBASE  6001
#define MFCC_MELBASE  3501
#define MFCC_MELCHUNK  80
//#define MFCC_MELCHUNK  20

//#define MFCC_BNFBASE  1499
#define MFCC_BNFBASE  874
#define MFCC_BNFCHUNK  256
//input==== NodeArg(name='speech', type='tensor(float)', shape=['B', 'T', 80])
//input==== NodeArg(name='speech_lengths', type='tensor(int32)', shape=['B'])
//output==== NodeArg(name='encoder_out', type='tensor(float)', shape=['B', 'T_OUT', 'Addencoder_out_dim_2'])
#define STREAM_BASE_MINOFF 10 
#define STREAM_BASE_MINBLOCK 20
#define STREAM_BASE_MAXBLOCK 50
#define STREAM_BASE_TICK 40
#define STREAM_BASE_PCM 1280
#define STREAM_BASE_SAMP 640
#define STREAM_BASE_BNF 256
#define STREAM_CNT_BNF 20
#define STREAM_OFF_BNF 20
#define STREAM_ALL_BNF 20480
#define STREAM_BASE_MEL 80
#define STREAM_BASE_CNT 1500
//#define STREAM_BASE_CNT 050
#define STREAM_MFCC_FILL 10
//#define STREAM_MFCC_FILL 5

