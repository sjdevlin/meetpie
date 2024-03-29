//
//  meetpie.h
//  
//
//  Created by Stephen Devlin on 01/02/2021.
//

#ifndef meetpie_h
#define meetpie_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <math.h>
#include <time.h>

#define INPORT 9000
#define MAXLINE 1024
#define MAXPART 8
#define MAXSILENCE 500
#define NUMCHANNELS 3
#define ANGLESPREAD 15
#define MINTURNSILENCE 30
#define MINENERGY 0.2
#define MINTALKTIME 3

// to do :
// define talker as the highest average energy over last 5 secs
// interrupter is the other target if their energy is over 0.8
// sucessful interrurpt is if they then take over as the strongest

// function protos

// structs
typedef struct odas_data{
    double x;
    double y;
    double activity;
    int frequency;
 } odas_data;

typedef struct participant_data{
    int participant_angle;
    int participant_is_talking;
    int participant_silent_time;
    int participant_total_talk_time;
    int participant_num_turns;
    float participant_frequency;
 } participant_data;

typedef struct meeting{
    int participant_number[360];
    int num_participants;
    int total_silence;
    int total_meeting_time;
    int last_talker;
    int num_talking;
 } meeting;

void process_sound_data(meeting *, participant_data *, odas_data *);
void initialise_meeting_data(meeting *, participant_data *, odas_data *);
void write_to_file(char * ) ;


#endif /* meetpie_h */
