#include "includes.h"
#include "global.h"
#include "fonctions.h"

/**
 * \fn void initStruct(void)
 * \brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void initStruct(void);

/**
 * \fn void startTasks(void)
 * \brief Démarrage des tâches
 */
void startTasks(void);

/**
 * \fn void deleteTasks(void)
 * \brief Arrêt des tâches
 */
void deleteTasks(void);

int main(int argc, char**argv) {
    printf("#################################\n");
    printf("#      DE STIJL PROJECT         #\n");
    printf("#################################\n");

    //signal(SIGTERM, catch_signal);
    //signal(SIGINT, catch_signal);

    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT | MCL_FUTURE);
    /* For printing, please use rt_print_auto_init() and rt_printf () in rtdk.h
     * (The Real-Time printing library). rt_printf() is the same as printf()
     * except that it does not leave the primary mode, meaning that it is a
     * cheaper, faster version of printf() that avoids the use of system calls
     * and locks, instead using a local ring buffer per real-time thread along
     * with a process-based non-RT thread that periodically forwards the
     * contents to the output stream. main() must call rt_print_auto_init(1)
     * before any calls to rt_printf(). If you forget this part, you won't see
     * anything printed.
     */
    rt_print_auto_init(1);
    initStruct();
    startTasks();
    pause();
    deleteTasks();

    return 0;
}

void initStruct(void) {
    int err;
    /* Creation des mutex */
    if (err = rt_mutex_create(&mutexEtat, NULL)) {
        rt_printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutexMove, NULL)) {
        rt_printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }

    /* Creation du semaphore */
    if (err = rt_sem_create(&semConnecterRobot, NULL, 0, S_FIFO)) {
        rt_printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }

    /* Creation des taches */
    if (err = rt_task_create(&tcommunicate, NULL, 0, PRIORITY_TCOMMUNICATE, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&tconnect, NULL, 0, PRIORITY_TCONNECT, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&tmove, NULL, 0, PRIORITY_TMOVE, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&tsend, NULL, 0, PRIORITY_TSEND, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&twatchdog, NULL, 0, PRIORITY_TWATCHDOG, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&tbattery, NULL, 0, PRIORITY_TBATTERY, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&tcam, NULL, 0, PRIORITY_TCAM, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&tposition, NULL, 0, PRIORITY_TPOSITION, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }    
    if (err = rt_task_create(&tarena, NULL, 0, PRIORITY_TARENA, 0)) {
        rt_printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }

    /* Creation des files de messages */
    if (err = rt_queue_create(&queueMsgGUI, "toto", MSG_QUEUE_SIZE*sizeof(DMessage), MSG_QUEUE_SIZE, Q_FIFO)){
        rt_printf("Error msg queue create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }

    /* Creation des structures globales du projet */
    robot = d_new_robot();
    move = d_new_movement();
    serveur = d_new_server();
}

void startTasks() {
    int err;
    if (err = rt_task_start(&tcommunicate, &communiquer, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tconnect, &connecter, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tmove, &deplacer, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tsend, &envoyer, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&twatchdog, &watchdog, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tbattery, &batteries, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tcam, &webcam, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tposition, &position, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&tarena, &arene, NULL)) {
        rt_printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
}

void deleteTasks() {
  rt_task_delete(&tcommunicate);
  rt_task_delete(&tconnect);
  rt_task_delete(&tmove);    
  rt_task_delete(&tsend);
  rt_task_delete(&twatchdog);
  rt_task_delete(&tcam);    
  rt_task_delete(&tarena);
  rt_task_delete(&tposition);
  rt_task_delete(&tbattery);
}
