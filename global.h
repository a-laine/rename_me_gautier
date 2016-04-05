/* 
 * File:   global.h
 * Author: pehladik
 *
 * Created on 12 janvier 2012, 10:11
 */

#ifndef GLOBAL_H
#define	GLOBAL_H

#include "includes.h"

/* @descripteurs des tâches */
extern RT_TASK tcommunicate;
extern RT_TASK tconnect;
extern RT_TASK tmove;
extern RT_TASK tsend;
extern RT_TASK twatchdog;
extern RT_TASK tbattery;
extern RT_TASK tcam;
extern RT_TASK tposition;
extern RT_TASK tarena;

/* @descripteurs des mutex */
extern RT_MUTEX mutexEtat;
extern RT_MUTEX mutexMove;

/* @descripteurs des sempahore */
extern RT_SEM semConnecterRobot;

/* @descripteurs des files de messages */
extern RT_QUEUE queueMsgGUI;

/* @variables partagées */
extern int etatCommMoniteur;
extern int etatCommRobot;
extern DServer *serveur;
extern DRobot *robot;
extern DMovement *move;

typedef struct Etat_communication_t {
	int robot;
	int moniteur;
	int cpt;
} Etat_communication_t;
extern Etat_communication_t *etat_communication;

/* @constantes */
extern int MSG_QUEUE_SIZE;
extern int PRIORITY_TCOMMUNICATE;
extern int PRIORITY_TCONNECT;
extern int PRIORITY_TMOVE;
extern int PRIORITY_TSEND;
extern int PRIORITY_TWATCHDOG;
extern int PRIORITY_TBATTERY;
extern int PRIORITY_TCAM;
extern int PRIORITY_TPOSITION;
extern int PRIORITY_TARENA;

#endif	/* GLOBAL_H */

