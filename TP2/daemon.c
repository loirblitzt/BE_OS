
/**
 * @file daemon.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Thu Dec 28 12:35:34 2017
 * @brief Daemon entry point.
 *
 * Daemon entry point.
 */


/* 
		Vincent Loibl
j'ai rajouté des petits commentaires pour rendre ca plus compréhensible :
	_un petit conseil commencez à lire les functions de bas de fichier
	_puis remonter petit a petit, fonction par fonction

*/



#include "config.h"
#include "logging.h"
#include "list.h"
#include "event.h"

#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Globals, because sometimes everyone feels lazy. */
list   events = nil;
FILE *logfile = NULL;

/*
   Lock file management.
*/


/* Returns 0 in case lock file does not exist or PID can't be read. */
static int lock_file_exists( void ) {
  FILE *fp = fopen( LOCK_DAEMON, "rt" );

  if ( !fp ) {
    return 0;
  }
  else {
    int pid = 0;

    if ( 1 != fscanf( fp, "%d", &pid ) ) {
      ABORT( "Can't read PID.\n" );
    }
    fclose( fp );

    return pid;
  }
}

/* Create lock file, store PID inside. */
static int create_lock_file( void ) {
  FILE *fp = fopen( LOCK_DAEMON, "w" );

  if ( !fp ) {
    ABORT( LOCK_DAEMON );
  }

  fprintf( fp, "%d\n", getpid() );

  fclose( fp );

  return 0;
}


/*
   Upon call to exit...
*/

void delete_lock_file( void ) {
  LOG( logfile, "* Deleting lock file.\n" );
  unlink( LOCK_DAEMON );
  return;
}

static void delete_event_list( void ) {
  LOG( logfile, "* Deleting event list.\n" );
  delete( events, del_event );
}

static void  close_log_file( void ) {
  LOG( logfile, "* Closing log file.\n" );
  fclose( logfile );
}


/* Find next alarm. */
static void set_next_alarm( void ) {

  if ( empty( events ) ) {
    LOG( logfile, "No next event.\n" );
  }
  else {
    event   ev = car( events );
    int   when = event_date( ev ) - time( NULL );

    LOG( logfile, "Next event in %d sec.\n", when );
    alarm( when );
  }
}


/*
   Signal handlers.
 */

/*
	meme chose que en dessous blablabla
*/
static void sigalrm_handler( int signum ) {

  /* TODO: Reload signal handler? */

  if ( !empty( events ) ) {
    event   ev;
    char   buff[1+STRLEN];

    ev = pop( events );
    snprintf( buff, STRLEN, "Executing: %s.\n", event_command( ev ) );
    LOG( logfile, buff );
    set_next_alarm();
    system( event_command( ev ) );
  }

  return;
  UNUSED( signum );
}

/*
	Ici on intercepte le signal SIGINT 
	on configure des truc mais le plsu important cest la ligne de signal() quil ne faut pas oublier 
	cf explixcation ci dessous
*/
static void sigint_handler( int signum ) {

  /* TODO: Reload signal handler? */
  signal(SIGINT,sigint_handler);

  delete( events, del_event );

  events = read_events( CONF_DAEMON );

  LOG( logfile, "Events loaded.\n" );

  set_next_alarm();

  return;
  UNUSED( signum );
}

/*
	On soccupe du signal SIGUSR1 ici :
	on affiche différents trucs mais SURTOUT
	on reappelle la fonction signal() 
	en effet quand un signal est intercepté par un handler l'os supprime la fonction des fonctions a appeller pour intercepter un signal,
	cela permet de eventuellemnt changer de fonction d'appel de handler
	Ainsi il ne faut pas oublier de rappeler la fonction signal() pour reenregistrer notre fonction
*/
static void sigusr1_handler( int signum ) {
  int     n = length( events );

  /* TODO: Reload signal handler? */
  signal(SIGUSR1,sigusr1_handler);

  printf( "\nRunning with PID %d.\n", getpid() );

  if ( 0 == n ) {
    printf( "* No next event (reload updated config file).\n" );
  }
  else {
    printf( "* %d event%s remaining:\n\n", n, n > 1 ? "s" : "" );
    visit( events, print_event );
  }

  UNUSED( signum );
}

//on gere le signal SIGTERM (signal de terminaison)
/*
		cest le plus simple handler :
		on dit juste au programme de se terminer avec exit()
		qui comme expliqué si dessous , appelle les fonctions enregistrées
		dans une pile (First In Last Out) grace a la fonction atexit( nomDuneFocntion) ligne 
		ainsi on suprrime bien le lockfile quand on recoit un brutal signal SIGTERM
*/
static void sigterm_handler( int signum ) {

  /* TODO: Reload signal handler? NOT HERE YOU BASTARD*/

  LOG( logfile, "Exiting.\n" );

  exit( EXIT_SUCCESS );

  UNUSED( signum );
}

/*
  Daemon entry point.
 */
//ici cest le vrai boulot du deamon qui est produit
static void main_daemon( int argc, char *argv[] ) {

  /* TODO: Register functions to be called on exit(3) !in reverse order! */*
	 /*
	  alors pas forcement le plus simple en premier mais ici on dit à la fonction exit() 
	  dappeler les fonctions close_log_file , delete_lock_file , delete_event_list quand elle sera appellé
	  ainsi quand on termine le programme avec un 
	  exit( EXIT_FAILURE );
	  ou encore exit( EXIT_SUCESS);
	  le lockfile est supprimé et tout est propre :)
	  d'ailleur vous remarquerez que dans la fonction si dessus : sigterm_handler()
	  qui est appelé quand on envoie un SIGTERM au deamon (grace à la ligne 207 ci dessous)
	*/
  atexit(close_log_file);
  atexit(delete_lock_file);
  atexit(delete_event_list);
  /* TODO: Register signal handlers. */
  /*
	LES LIGNES LES PLUS IMPORTANTES DU BE :
	ici on dit à l'os quelles fonctions appeler quand on recoit tel ou tel message
	tout ca grace a la fonction signal cf "google" "ecosia" "duckduckgo" ou "man signal"
	exemple :
	quand on recoit SIGTERM , la focntion sigterm_handler() va etre appelé
	meme chose pour les autres signaux
	je vous invite donc à regarder les fonctions "handler" ci dessus 
	NB : handler cest proche de HAND (main en anglais :p ) (symbole de l'action toussa toussa)
	handle == gérer	comme agir, faire quelque chose. 
	pareil en allemand (zu handeln )
	bref en gros cest une fonction qui gère l'action la procédure suite à un certain evenement (un signal recu par exemple)
  */
  signal(SIGTERM,sigterm_handler);
  signal(SIGINT,sigint_handler);
  signal(SIGUSR1,sigusr1_handler);

  /* Create log and lock files. */
  //bon la on creer le logfile pour dire a tout le monde que est bien en vie et que on a un pid
  logfile = open_log_file( LOG_DAEMON );

  create_lock_file();

  /* Start logging. */
  LOG( logfile, "Up and running.\n" );

  /* TODO: Force loading of config file (send SIGINT to self). */
  // sigint ici sert à que le daemon lise le fichier de config et ainsi puisse savoir ce quil doit faire
 // du coup au demarage on envoie a soit meme SIGINT
  kill(getpid(),SIGINT);
  /* TODO: Wait for signals. */
  while(1){
	//ici wait() dit à l'os que en fait on est faineant et que tout ce que l'on fait cest d'attendre des signaux 
	// on a pas besoin de puissance de calcul on ne gaspille pas de ressource cpu ect
    wait();
  }

  return;
  UNUSED( argc );
  UNUSED( argv );
}

/* Create daemon. */
static int daemonize( void (*exec_daemon)(int,char*[]), int argc, char *argv[] ) {

  /* TODO: Use double fork and setsid to launch daemon entry point. */
  /*ici on fait deux fork mais un seul garde lappel a main_deamon*/
  pid_t mypid = fork();
  if(mypid == 0){
    //ici cest le premier fils
    setsid(); //on detache le programme du clavier et de toute action exterieure ;) comme ca on ne pourra que communiquer avec le deamon avec des signaux

	//on cree un deuzieme fils (on etait pas obligé mais dans des deamon plsu complexe cest comme ca que cest fait et cest devenu une convention
    mypid = fork();
    if(mypid == 0){
      //ici cest le deuxieme fils et le deamon final , on lance la fonction qui fait le le vrai boulot du deamon
      main_daemon(argc, argv);
    }
    else if (mypid == -1){
      /*message d'erreur plus rien ne va plus*/
      fprintf(stderr, "failure to launch the fork syscall");
      return EXIT_FAILURE;
    }
  }
  else if (mypid == -1){
    /*mesage d'erreur */
    fprintf(stderr, "failure to launch the fork syscall");
    return EXIT_FAILURE;

  }

  return EXIT_SUCCESS;
}


/* Cosmetics. */
static void print_usage( FILE *fp, char *progname ) {

  fprintf( fp, "\n%s  - A toy cron daemon.\n\n", progname );
  fprintf( fp, "  Usage: %s [--]command\n\n", progname );
  fprintf( fp, "  Available commands:\n\n" );
  fprintf( fp, "    help\tPrint this help.\n" );
  fprintf( fp, "    start\tStart %s daemon.\n", progname );
  fprintf( fp, "    stop\tStop %s daemon.\n", progname );
  fprintf( fp, "    restart\tRestart %s daemon.\n", progname );
  fprintf( fp, "    reload\tReload %s config file.\n", progname );
  fprintf( fp, "    status\tPrint %s daemon status.\n", progname );
  fprintf( fp, "\n" );
}


/*
	le main première fonction a regarder
*/
/* Main entry point, check args */
int main ( int argc, char *argv[] ) {

  int        pid = -1;
  char *progname = argv[0];

  /* Remove garbage at start of argv[0] */
  while( !isalpha( *(progname++) ) ); progname--;

  /*on verifie bien qu'il y a 1 arguments donné à notre programme*/
  if ( 2 != argc ) {
    print_usage( stderr, progname );
    exit( EXIT_FAILURE );
  }

  /* If non-zero, this is the PID of our daemon. */
  /*esceque notre demon run deja , dans ce cas le lockfile existe*/
  pid = lock_file_exists();

  /* TODO: Handle 'restart' command (first, because of strstr and 'start' command).
     Hint: Report failure on exiting if daemon is not running,
     otherwise terminate it and launch new one.
   */

  /* ici ca commence a etre interessant 
	on regarde le premier argument donné à notre programme
	si l'argument est "restart" 
	on envoie le signal sigterm (avec la fonction kill)
	ce qui provoque la terminaison du programme dont le pid est ecris dans le lockfile aka LE DEMON
	puis on relance le demon avec la fonction daemonize() , perso je l'ai fait retourner un truc du coup je l'ai placé dans un exit mais ca on sen balek

	ca peut etre vu comme une combinaison de l'argument "start" et "stop" cf plus bas
  */
  if ( strstr( argv[1], "restart" ) ) {
    if ( !pid ) {
      fprintf( stderr, "No %s daemon running.\n", progname );
      exit( EXIT_FAILURE );
    }
    else {
      kill(pid,SIGTERM);
      exit(daemonize(NULL , argc , argv));
    }
  }

  /* TODO: Handle 'start' command.
     Hint: If daemon is already running, report failure on exiting.
     Otherwise, launch daemon entry point.
   */
  /*
	start provoque le lancement du demon , bien sur si et seulment si le deamon nest pas deja en train de fonctionner(si le lockfile existe)
  */
  if ( strstr( argv[1], "start" ) ) {
    if ( pid ) {
      fprintf( stderr, "%s running, not launching daemon.\n", progname );
      exit( EXIT_FAILURE );
    }
    else {
      /*ici on demare le boulot*/
      exit(daemonize(NULL , argc , argv));
    }
  }

  /* TODO: Handle 'stop' command.
     Hint: If daemon is not running, report failure on exiting.
     Otherwise, terminate it.
   */
  /*
	stop envoie le signal SIGTERM qui provoque l'arret forcé du programme par l'OS
  */
  if ( strstr( argv[1], "stop" ) ) {
    if ( !pid ) {
      fprintf( stderr, "No %s daemon running.\n", progname );
      exit( EXIT_FAILURE );
    }
    else {
      kill(pid, SIGTERM);
      exit(EXIT_FAILURE);
    }
  }

  /* TODO: Handle 'status' command.
     Hint: If daemon is not running, report failure on exiting.
     Otherwise, send SIGUSR1 signal.
   */
  /*
	ici le signal SIGUSR1 est envoyé au deamon, tjr avec la fonction kill
	cf plus haut pour comment on traite le signal dans la partie deamon
  */
  if ( strstr( argv[1], "status" ) ) {
    if ( !pid ) {
      fprintf( stderr, "No %s daemon running.\n", progname );
      exit( EXIT_FAILURE );
    }
    else {
      kill(pid,SIGUSR1);
      exit(EXIT_SUCCESS);
    }
  }

  /* TODO: Handle 'reload' command.
     Hint: If daemon is not running, report failure on exiting.
     Otherwise, reload config file.
   */

/*
	pareil mais cette fois cest SIGINT qui est envoyé au daemon,
*/
  if ( strstr( argv[1], "reload" ) ) {
    if ( !pid ) {
      fprintf( stderr, "No %s daemon running.\n", progname );
      exit( EXIT_FAILURE );
    }
    else {
      kill(pid,SIGINT);
      exit(EXIT_SUCCESS);
    }
  }

  if ( strstr( argv[1], "help" ) ) {
    print_usage( stdout, progname );
    exit( EXIT_SUCCESS );
  }

  /* No known command, report failure on exiting. */
  print_usage( stderr, progname );
  exit( EXIT_FAILURE );
}
