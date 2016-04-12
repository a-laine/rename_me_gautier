#include "fonctions.h"

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size);

/* Author : Gautier Delorme
 * State : done
 */
void envoyer(void * arg) {
	DMessage *msg;
	int err;

	while (1) {
		rt_task_wait_period(NULL);
		if ((err = rt_queue_read(&queueMsgGUI, &msg, sizeof (DMessage), TM_INFINITE)) >= 0) {
			rt_mutex_acquire(&mutexServer, TM_INFINITE);
			server->send(server, msg);
			msg->free(msg);
			rt_mutex_release(&mutexServer);
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
		rt_printf("tconnect : Attente du sémaphore semConnecterRobot\n");
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
			status = robot->start_insecurely(robot); //met le robot en attente des ordres
			rt_mutex_release(&mutexRobot);
			if (status == STATUS_OK){
				rt_sem_v(&semWatchdog);
				rt_sem_v(&semBattery);
				rt_sem_v(&semWebcam);
				rt_printf("tconnect : semWebcam free\n");
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

/* Author : Nabil
 * State : quasi-complet, reste à finir la partie mission
 */
void communiquer(void *arg) {
	DMessage *msg = d_new_message();
	int size = 1;
	int num_msg = 0;
	
	while (1) { //première boucle : permet de relancer le serveur en cas de perte de connexion

		/* Ouverture de la connexion */
		// on est censé attendre la connexion de la part d'un socket a priori
		rt_printf("tcommunicate : Début de l'exécution du Serveur\n");
		server->open(server, "8000");
		rt_printf("tcommunicate : Connexion\n");

		/* Etat ? */
		rt_mutex_acquire(&mutexEtat, TM_INFINITE);
		rt_mutex_release(&mutexEtat);

		while (1) { //deuxième boucle : permet de recevoir un message après l'autre
			
			/* Réception du message */
			rt_printf("tcommunicate : Attente d'un message\n");
			size = server->receive(server, msg);
			num_msg++;
			
			if (size <= 0) {
				server->close(server); //en cas de perte de communication, fermer le serveur
				break;
			}

			switch (msg->get_type(msg)) {

				/* Type::Action */
				case MESSAGE_TYPE_ACTION:
					rt_printf("tcommunicate : Le message %d reçu est une action\n", num_msg);
					DAction *action = d_new_action();
					action->from_message(action, msg);
					switch (action->get_order(action)) {
						/* Type Action-Connect */
						case ACTION_CONNECT_ROBOT:
							rt_printf("tcommunicate : Action 'connecter robot'\n");
							rt_sem_v(&semConnecterRobot); //lance la connexion superviseur-robot (=> Connecter)
							break;
						/* Type Action-FindArena */
						case ACTION_FIND_ARENA:
							rt_printf("tcommunicate : Action 'demander acquisition'\n");
							rt_sem_v(&semAcquArene); //lance l'acquisition de l'arene
							rt_mutex_acquire(&mutexValidArene, TM_INFINITE);
							areneValidee = 1;
							rt_mutex_release(&mutexValidArene);
							rt_sem_v(&semValidArene); //lance la validation de l'arene
							break;
						/* Type Action-ArenaFound */
						case ACTION_ARENA_IS_FOUND:
							rt_printf("tcommunicate : Action 'valider arene'\n");
							rt_mutex_acquire(&mutexValidArene, TM_INFINITE);
							areneValidee = 0;
							rt_mutex_release(&mutexValidArene);
							rt_sem_v(&semValidArene); //lance la validation de l'arene
							break;
						/* Type Action-ArenaFailed */
						case ACTION_ARENA_FAILED:
							rt_printf("tcommunicate : Action 'annuler arene'\n");
							rt_mutex_acquire(&mutexValidArene, TM_INFINITE);
							areneValidee = 2;
							rt_mutex_release(&mutexValidArene);
							rt_sem_v(&semValidArene); //lance la validation de l'arene
							break;
						case ACTION_COMPUTE_CONTINUOUSLY_POSITION:
							rt_printf("tcommunicate : Action ' start compute position'\n");
							rt_sem_v(&semPosition);
							break;
						case ACTION_STOP_COMPUTE_POSITION:
							rt_printf("tcommunicate : Action ' stop compute position'\n");
							rt_sem_p(&semPosition, TM_INFINITE);
							break;
					}
					break;

				/* Type::Mission */
				case MESSAGE_TYPE_MISSION:
					rt_printf("tcommunicate : Le message reçu %d est une mission\n", num_msg);
					rt_mutex_acquire(&mutexPositionVoulue, TM_INFINITE);
					// A Compléter avec les missions
					rt_mutex_release(&mutexPositionVoulue);
					rt_sem_v(&semMission);
					break;

				/* Type::Mouvement */
				case MESSAGE_TYPE_MOVEMENT:
					rt_printf("tcommunicate : Le message reçu %d est un mouvement\n", num_msg);
					rt_mutex_acquire(&mutexMove, TM_INFINITE);
					mvt->from_message(mvt, msg);
					mvt->print(mvt);
					rt_mutex_release(&mutexMove);
					break;
			}
		}
	}
}

/* Author : 
 * State : done
 */
void deplacer(void *arg) {
	int status = 1;
	int gauche;
	int droite;
	DMessage *message;

	rt_printf("tmove : Debut de l'éxecution de periodique à 1s\n");
	rt_task_wait_period(NULL);
		rt_printf("tmove : Activation périodique\n");
	while (1) {
		/* Attente de l'activation périodique */
		rt_task_wait_period(NULL);


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
			
			//rt_mutex_acquire(&mutexRobot, TM_INFINITE);
			status = robot->set_motors(robot, gauche, droite);
			//rt_mutex_release(&mutexRobot);

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
rt_printf("twatchdog : Attente du sémaphore semWatchdog\n");
		rt_sem_p(&semWatchdog, TM_INFINITE);
		rt_printf("twatchdog : Ouverture de la communication avec le robot\n");
	while (1) {
		rt_task_wait_period(NULL);
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



/* Author : Aurélien Lainé
 * State : en cours
 */
void position(void *arg)
{
	int status;
	int tmpAreneValidee;
	DPosition* tmpPositionRobot;
	DMessage *message;
	rt_printf("tposition : Attente du sémarphore semPosition\n");
    
    while(1)
    {
    	rt_sem_p(&semPosition, TM_INFINITE);
    	rt_task_wait_period(NULL);
		rt_mutex_acquire(&mutexImage, TM_INFINITE);
			rt_mutex_acquire(&mutexArene, TM_INFINITE);
				tmpPositionRobot = image->compute_robot_position(image,0);
				if (tmpPositionRobot) {
					rt_mutex_acquire(&mutexPositionRobot, TM_INFINITE);
						d_position_free(positionRobot);
						positionRobot = tmpPositionRobot;
						message = d_new_message();
						message->put_position(message, positionRobot);
						rt_printf("tposition : Envoi message\n");
						if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
							message->free(message);
						}
					rt_mutex_release(&mutexPositionRobot);
				}
			rt_mutex_release(&mutexArene);
		rt_mutex_release(&mutexImage);
		rt_sem_v(&semPosition);	
	}
}

/* Author : Aurélien Lainé
 * State : done
 */
void batteries(void *arg)
{
	int status;
	int vbat;
	DBattery* batterie = d_new_battery();
	DMessage *message;
	rt_printf("tbattery : Attente du sémarphore semBattery\n");
	rt_sem_p(&semBattery, TM_INFINITE);

	while(2) {
		do {
			rt_task_wait_period(NULL);
			rt_mutex_acquire(&mutexEtat, TM_INFINITE);
			status = etat_communication->robot;
			rt_mutex_release(&mutexEtat);
		} while(status == 1);

		rt_mutex_acquire(&mutexRobot, TM_INFINITE);
		status = robot->get_vbat(robot, &vbat);
		rt_mutex_release(&mutexRobot);

		batterie->set_level(batterie, vbat);
		message = d_new_message();
		message->put_battery_level(message, batterie);

		rt_printf("tbatteries : Envoi message\n");
		if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
			message->free(message);
		}
	}
}

/* Author : Nabil Sellam
 * State : reste à choisir une des deux versions
 */
void arene(void *arg)
{
	/*Choisir une des deux versions*/

	/* VERSION 1 :*/
	int valide; // reçoit la valeur de areneValidee

	while(1) {
		rt_printf("tArene : Attente du sémarphore semAcquArene\n");

			rt_sem_p(&semAcquArene, TM_INFINITE); //libéré par Communiquer() sur ACTION_FIND_ARENA
			rt_sem_p(&semValidArene, TM_INFINITE);
			rt_mutex_acquire(&mutexImage, TM_INFINITE); //acquisition des mutex image et arene pour computer
			rt_mutex_acquire(&mutexArene, TM_INFINITE);
			if(arena)
				arena->free(arena);

			arena = image->compute_arena_position(image);
			rt_mutex_release(&mutexArene);
			rt_mutex_release(&mutexImage);

			// Demander la validation de l'arene
			rt_printf("tArene : Attente du sémaphore semValidArene\n");
			rt_sem_p(&semValidArene, TM_INFINITE);

			rt_mutex_acquire(&mutexValidArene, TM_INFINITE);
			valide = areneValidee;
			rt_mutex_release(&mutexValidArene);

			if(valide == 2){ //Annulation de la demande
				rt_mutex_acquire(&mutexArene, TM_INFINITE);
				if(arena) {
					arena->free(arena); //on free l'arène (tant pis)
					arena = 0;
				}
				rt_mutex_release(&mutexArene);
			}
	} 
}

/* Author : Aurélien Lainé
 * State : done
 */
void webcam(void *arg)
{
	int status, tmpAreneValidee;
	DMessage *message;
	DCamera* camera = d_new_camera();
	DJpegimage* jpgImage = d_new_jpegimage();
	
	printf("tWebcam : Attente du sémarphore semWebcam\n");
    rt_sem_p(&semWebcam, TM_INFINITE);
    camera->open(camera);

    while(1)
    {
    	rt_task_wait_period(NULL);

    	rt_mutex_acquire(&mutexImage, TM_INFINITE);

		camera->get_frame(camera, image); //acquisition d'une DImage

		// dessiner l'arene
		rt_mutex_acquire(&mutexArene, TM_INFINITE);
		d_imageshop_draw_arena(image, arena);
		rt_mutex_release(&mutexArene);
		
		// dessiner la position du robot
		rt_mutex_acquire(&mutexPositionRobot, TM_INFINITE);
    	d_imageshop_draw_position(image, positionRobot);
    	rt_mutex_release(&mutexPositionRobot);
    	
    	do {
			rt_mutex_acquire(&mutexEtat, TM_INFINITE);
			status = etat_communication->robot;
			rt_mutex_release(&mutexEtat);
		} while(status == 1);
		
		jpgImage->compress(jpgImage, image);
		rt_mutex_release(&mutexImage);
		
		message = d_new_message();
		message->put_jpeg_image(message, jpgImage);
		
		//rt_printf("twebcam : Envoi message\n");
		if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
            message->free(message);
        }
	}
}

/* Author : Aurélien Lainé
 * State : started
 */
void mission(void *arg)
{
	while(1) {
	rt_printf("tMission : Attente du sémarphore semMission\n");
	rt_sem_p(&semMission, TM_INFINITE);

		while(0) {// while( pas arrivé ) 
			// se deplacer à la position voulue
			rt_mutex_acquire(&mutexPositionVoulue, TM_INFINITE);
			//positionVoulue
			rt_mutex_release(&mutexPositionVoulue);
			rt_mutex_acquire(&mutexMove, TM_INFINITE);
			//mvt->set(mvt, );
			rt_mutex_release(&mutexMove);
		}
	}
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


