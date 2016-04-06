#include "fonctions.h"

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size);

void envoyer(void * arg) {
    DMessage *msg;
    int err;

    while (1) {
        rt_printf("tenvoyer : Attente d'un message\n");
        if ((err = rt_queue_read(&queueMsgGUI, &msg, sizeof (DMessage), TM_INFINITE)) >= 0) {
            rt_printf("tenvoyer : envoi d'un message au moniteur\n");
            server->send(server, msg);
            msg->free(msg);
        } else {
            rt_printf("Error msg queue write: %s\n", strerror(-err));
        }
    }
}

/* Author : Gautier Delorme
 * State : done
 */
void connecter(void * arg) {
    int status;
    DMessage *message;

    rt_printf("tconnect : Debut de l'exécution de tconnect\n");

    while (1) {
        rt_printf("tconnect : Attente du sémarphore semConnecterRobot\n");
        rt_sem_p(&semConnecterRobot, TM_INFINITE);
        rt_printf("tconnect : Ouverture de la communication avec le robot\n");
		rt_mutex_acquire(&mutexRobot, TM_INFINITE);
        status = robot->open_device(robot);
		rt_mutex_release(&mutexRobot);

        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        etat_communication->robot = status;
        rt_mutex_release(&mutexEtat);

        if (status == STATUS_OK) {
			rt_mutex_acquire(&mutexRobot, TM_INFINITE);
            status = robot->start_insecurely(robot);
			rt_mutex_release(&mutexRobot);
            if (status == STATUS_OK){
                rt_printf("tconnect : Robot démarrer\n");
            }
        }

        message = d_new_message();
        message->put_state(message, status);

        rt_printf("tconnecter : Envoi message\n");
        message->print(message, 100);

        if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
            message->free(message);
        }
    }
}

void communiquer(void *arg) {
    DMessage *msg = d_new_message();
    int size = 1;
    int num_msg = 0;

    /* Connexion */
    rt_printf("tcommunicate : Début de l'exécution du Serveur\n");
    server->open(server, "8000"); //ouvrir la connexion, DServer *serveur;
    rt_print("tcommunicate : Connexion\n");
    
    /* Etat */
    rt_mutex_acquire(&mutexEtat, TM_INFINITE);
    etat_communication = 0;
    rt_mutex_release(&mutexEtat);

    while (size > 0) {
      rt_printf("tcommunicate : Attente d'un message\n");
      size = server->receive(server, msg);
      num_msg++;
      
      if (size > 0) {
	switch (msg->get_type(msg)) {
	
	  /* Type::Action */
	case MESSAGE_TYPE_ACTION:
	  rt_printf("tcommunicate : Le message %d reçu est une action\n",
		    num_msg);
	  DAction *action = d_new_action();
	  action->from_message(action, msg);
	  switch (action->get_order(action)) {
	    /* Type::Action::Connect */
	  case ACTION_CONNECT_ROBOT:
	    rt_printf("tcommunicate : Action 'connecter robot'\n");
	    rt_sem_v(&semConnecterRobot);
	    break;
	    /* Type::Action::FindArena */
	  case ACTION_FIND_ARENA:
	    rt_printf("tcommunicate : Action 'demander acquisition'\n");
	    rt_sem_v(&semAcquArene);
	    break;
	    /* Type::Action::ArenaFound */
	    rt_printf("tcommunicate : Action 'valider arene'\n");
	    rt_sem_v(&semValidArene);
	    rt_mutex_acquire(&mutexEtat, TM_INFINITE);
	    //areneValidee = 0;
	    rt_mutex_release(&mutexEtat);
	    break;
	  }
	  break;
	  
	  /* Type::Mouvement */
	case MESSAGE_TYPE_MOVEMENT:
	  rt_printf("tcommunicate : Le message reçu %d est un mouvement\n",
		    num_msg);
	  rt_mutex_acquire(&mutexMove, TM_INFINITE);
	  mvt->from_message(mvt, msg);
	  mvt->print(mvt);
	  rt_mutex_release(&mutexMove);
	  break;
      
    /* Message Mouvement */

    /* Message ordre */
	}
      }
    }
    
}

void deplacer(void *arg) {
    int status = 1;
    int gauche;
    int droite;
    DMessage *message;

    rt_printf("tmove : Debut de l'éxecution de periodique à 1s\n");
    rt_task_set_periodic(NULL, TM_NOW, 1000000000);

    while (1) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        rt_printf("tmove : Activation périodique\n");

        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etat_communication->robot;
        rt_mutex_release(&mutexEtat);

        if (status == STATUS_OK) {
            rt_mutex_acquire(&mutexMove, TM_INFINITE);
            switch (mvt->get_direction(mvt)) {
                case DIRECTION_FORWARD:
                    gauche = MOTEUR_ARRIERE_LENT;
                    droite = MOTEUR_ARRIERE_LENT;
                    break;
                case DIRECTION_LEFT:
                    gauche = MOTEUR_ARRIERE_LENT;
                    droite = MOTEUR_AVANT_LENT;
                    break;
                case DIRECTION_RIGHT:
                    gauche = MOTEUR_AVANT_LENT;
                    droite = MOTEUR_ARRIERE_LENT;
                    break;
                case DIRECTION_STOP:
                    gauche = MOTEUR_STOP;
                    droite = MOTEUR_STOP;
                    break;
                case DIRECTION_STRAIGHT:
                    gauche = MOTEUR_AVANT_LENT;
                    droite = MOTEUR_AVANT_LENT;
                    break;
            }
            rt_mutex_release(&mutexMove);

            status = robot->set_motors(robot, gauche, droite);

            if (status != STATUS_OK) {
                rt_mutex_acquire(&mutexEtat, TM_INFINITE);
                etat_communication->robot = status;
                rt_mutex_release(&mutexEtat);

                message = d_new_message();
                message->put_state(message, status);

                rt_printf("tmove : Envoi message\n");
                if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                    message->free(message);
                }
            }
        }
    }
}

/* Author : Gautier Delorme
 * State : done
 */
int test_robot_state(RobotStatus res) {
	int status;
	rt_mutex_acquire(&mutexEtat, TM_INFINITE);
	if (res != STATUS_OK) {
        etat_communication->cpt++;
	}
	if (etat_communication->cpt < 3) {
		status = 1;
	} else {
		status = 0;
	}
	rt_mutex_release(&mutexEtat);
	return status;
}

/* Author : Gautier Delorme
 * State : done
 */
void watchdog(void *arg)
{
	int status = 1;
	RobotStatus res;
    DMessage *message;

    rt_printf("twatchdog : Debut de l'exécution de twatchdog\n");

    while (1) {
        rt_printf("twatchdog : Attente du sémarphore semWatchdog\n");
        rt_sem_p(&semWatchdog, TM_INFINITE);
        rt_printf("twatchdog : Ouverture de la communication avec le robot\n");
		sleep(1);
		while (status == 1) {
			rt_mutex_acquire(&mutexRobot, TM_INFINITE);
            res = robot->reload_wdt(robot);
			rt_mutex_release(&mutexRobot);
			status = test_robot_state(res);
		}
        message = d_new_message();
        message->put_state(message, status);

        rt_printf("twatchdog : Envoi message\n");
        message->print(message, 100);

        if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
            message->free(message);
        }
    }
}

void position(void *arg)
{
	
}

/* Author : Aurélien Lainé
 * State : En cours
 */
void batteries(void *arg)
{
	int status;
	int vbat;
	DBattery* batterie = d_new_battery();
	DMessage *message;
	
	while(2)
	{
		rt_task_wait_period(NULL);
		
		// verifier la communication
		rt_mutex_acquire(&mutexEtat, TM_INFINITE);
		status = etat_communication->robot;
		rt_mutex_release(&mutexEtat);
		if(status == 1)
		{
			// TODO GERER LES ERREURS
		}
		
		rt_mutex_acquire(&mutexRobot, TM_INFINITE);
        status = robot->get_vbat(robot, &vbat);
		rt_mutex_release(&mutexRobot);
		batterie->set_level(batterie, vbat);
		message = d_new_message();
		message->put_battery_level(message, batterie);
		
		rt_printf("tbatteries : Envoi message\n");
		message->print(message, 100);
    }
}

void arene(void *arg)
{
	
}

void webcam(void *arg)
{
	
}

void mission(void *arg)
{
	
}

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size) {
    void *msg;
    int err;

    msg = rt_queue_alloc(msgQueue, size);
    memcpy(msg, &data, size);

    if ((err = rt_queue_send(msgQueue, msg, sizeof (DMessage), Q_NORMAL)) < 0) {
        rt_printf("Error msg queue send: %s\n", strerror(-err));
    }
    rt_queue_free(&queueMsgGUI, msg);

    return err;
}
