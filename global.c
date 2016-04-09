/*
 * File:   global.h
 * Author: pehladik
 *
 * Created on 21 avril 2011, 12:14
 */

#include "global.h"

RT_TASK tcommunicate;
RT_TASK tconnect;
RT_TASK tmove;
RT_TASK tsend;
RT_TASK twatchdog;
RT_TASK tbattery;
RT_TASK tcam;
RT_TASK tposition;
RT_TASK tarena;
RT_TASK tmission;

RT_MUTEX mutexEtat;
RT_MUTEX mutexMove;
RT_MUTEX mutexRobot;
RT_MUTEX mutexServer;
RT_MUTEX mutexArene;
RT_MUTEX mutexImage;
RT_MUTEX mutexPositionRobot;
RT_MUTEX mutexPositionVoulue;
RT_MUTEX mutexValidArene;

RT_SEM semConnecterRobot;
RT_SEM semWatchdog;
RT_SEM semPosition;
RT_SEM semAcquArene;
RT_SEM semValidArene;
RT_SEM semBattery;
RT_SEM semWebcam;
RT_SEM semMission;

RT_QUEUE queueMsgGUI;

int areneValidee;
DRobot *robot;
DMovement *mvt;
DServer *server;
DImage* image;
DArena* arena;
DPosition* positionRobot;
DPosition* positionVoulue;
Etat_communication_t *etat_communication;

int MSG_QUEUE_SIZE = 10;

int PRIORITY_TCONNECT = 10;
int PRIORITY_TWATCHDOG = 9;
int PRIORITY_TCOMMUNICATE = 8;
int PRIORITY_TSEND = 7;
int PRIORITY_TMOVE = 6;
int PRIORITY_TMISSION = 5;
int PRIORITY_TARENA = 4;
int PRIORITY_TPOSITION = 3;
int PRIORITY_TCAM = 2;
int PRIORITY_TBATTERY = 1;



